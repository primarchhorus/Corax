#include "device.h"
#include "swap_chain.h"
#include "vulkan_common.h"
#include "window.h"
#include "instance.h"
#include "command_pool.h"
#include "frame_sync.h"
#include "dynamic_rendering.h"
#include "pipeline_builder.h"

class CoraxRenderer {
public:
    void run() 
    {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    void initVulkan() 
    {
        Vulkan::loadDynamicRenderingFunctions(device.logical_handle);
        createPipelineObjects();
    }

    void createPipelineObjects() 
    {
        pipeline_builder.buildPipeLines(swap_chain);
    }

    void updateRenderingInfo(uint32_t index)
    {

        color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        color_attachment_info.imageView = swap_chain.image_views[index];
        color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_info.clearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

        area.extent = swap_chain.extent;
        area.offset = {0,0};

        render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        render_info.renderArea = area;
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = 1;
        render_info.pColorAttachments = &color_attachment_info;

        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.dstAccessMask = 0;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        image_memory_barrier.image = swap_chain.images[index];
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;
    }

    void recordCommands(VkCommandBuffer buffer, uint32_t index)
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        vkCheck(vkBeginCommandBuffer(buffer, &begin_info));

        updateRenderingInfo(index);

        vkCmdPipelineBarrier(
            buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // srcStageMask
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
            0,
            0,
            nullptr,
            0,
            nullptr,
            1, // imageMemoryBarrierCount
            &image_memory_barrier // pImageMemoryBarriers
        );

        Vulkan::vkCmdBeginRenderingKHR(buffer, &render_info);

        // main_pipeline = pipelines.getPipeline(config);
        main_pipeline = pipeline_builder.getPipeLine(pipeline_builder.configs.triangle);
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline->handle);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swap_chain.extent.width);
        viewport.height = static_cast<float>(swap_chain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swap_chain.extent;
        vkCmdSetScissor(buffer, 0, 1, &scissor);

        vkCmdDraw(buffer, 3, 1, 0, 0);

        Vulkan::vkCmdEndRenderingKHR(buffer);

        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        image_memory_barrier.image = swap_chain.images[index];
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;


        vkCmdPipelineBarrier(
            buffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // dstStageMask
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,                    // imageMemoryBarrierCount
            &image_memory_barrier // pImageMemoryBarriers
        );

        vkCheck(vkEndCommandBuffer(buffer));
    }

    void drawFrame()
    {
        vkWaitForFences(device.logical_handle, 1, &frame_sync.in_flight_fences[frame_sync.current_frame], VK_TRUE, UINT64_MAX);
        
        uint32_t index;
        VkResult result = vkAcquireNextImageKHR(device.logical_handle, swap_chain.swap_chain, UINT64_MAX, frame_sync.image_available_semaphores[frame_sync.current_frame], VK_NULL_HANDLE, &index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            std::cout << "THE PROBLEM: vkAcquireNextImageKHR" << std::endl;
            return;
        } 
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        vkResetFences(device.logical_handle, 1, &frame_sync.in_flight_fences[frame_sync.current_frame]);
        vkResetCommandBuffer(frame_sync.command_buffers[frame_sync.current_frame], 0);
        recordCommands(frame_sync.command_buffers[frame_sync.current_frame], index);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[] = {frame_sync.image_available_semaphores[frame_sync.current_frame]};
        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores  = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &frame_sync.command_buffers[frame_sync.current_frame];

        VkSemaphore signal_semaphores[] = {frame_sync.render_finished_semaphores[frame_sync.current_frame]};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        vkQueueSubmit(device.graphics_queue, 1, &submit_info, frame_sync.in_flight_fences[frame_sync.current_frame]);

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;

        VkSwapchainKHR swap_chains[] = {swap_chain.swap_chain};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swap_chains;
        present_info.pImageIndices = &index;

        result = vkQueuePresentKHR(device.present_queue, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || glfw_window.window_resized) {
            glfw_window.window_resized = false;
            recreateSwapChain();
        } 
        else if (result != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        frame_sync.advanceFrame();
    }

    void mainLoop() 
    {
        while (!glfw_window.closeCheck()) {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(device.logical_handle);
    }

    void cleanup() 
    {
    }

    void recreateSwapChain()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(glfw_window.handle, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(glfw_window.handle, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(device.logical_handle);
        swap_chain.destroy();
        swap_chain.createSwapChain(glfw_window, instance);
        swap_chain.createImageViews();
    }

    Vulkan::Window glfw_window{800, 600, "vulkan"};
    Vulkan::Instance instance{glfw_window};
    Vulkan::Device device{instance};
    Vulkan::SwapChain swap_chain{device, glfw_window, instance};
    Vulkan::ShaderModule vertex_module;
    Vulkan::ShaderModule frag_module;
    Vulkan::PipeLine* main_pipeline;
    Vulkan::FrameSync frame_sync{device};
    Vulkan::PipeLineConfig config{};
    Vulkan::PipeLineBuilder pipeline_builder{device};
    bool swap_chain_resized{false};
    VkRenderingAttachmentInfoKHR color_attachment_info{};
    VkImageMemoryBarrier image_memory_barrier{};
    VkRect2D area{};
    VkRenderingInfo render_info{};
};

int main() {
    CoraxRenderer app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
