#include "corax.h"
#include "vulkan_operations.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/projection.hpp>
#include <glm/gtx/transform.hpp>

#include <array>
#include <chrono>

namespace Vulkan {

    void CoraxRenderer::run() {
        init();
        mainLoop();
        destroy();
    }

    void CoraxRenderer::init() {
        glfw_window.create(1200, 1000, "vulkan");
        instance.create();
        instance.createSurface(glfw_window);
        device.create(instance);

        transfer_pool = CommandPool::createPool(device);
        allocator = MemoryAllocator::createAllocator(instance, device);
        swap_chain.create(device, glfw_window, instance);
        initDepthImage();
        frame_sync.create(device);

        Descriptors::initPool(global_descriptor_allocator, device);

        Vulkan::loadDynamicRenderingFunctions(device.logical_handle);

        Descriptors::addLayoutBinding(global_layout, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
        Descriptors::buildLayout(global_layout, device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        Descriptors::addLayoutBinding(scene_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                      (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
        Descriptors::buildLayout(scene_layout, device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);


        MaterialOperation::buildPipelines(device, swap_chain, material_operations, pipeline_cache, scene_layout);

        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

        sampl.magFilter = VK_FILTER_NEAREST;
        sampl.minFilter = VK_FILTER_NEAREST;

        vkCreateSampler(device.logical_handle, &sampl, nullptr, &default_sampler_nearest);

        sampl.magFilter = VK_FILTER_LINEAR;
        sampl.minFilter = VK_FILTER_LINEAR;
        vkCreateSampler(device.logical_handle, &sampl, nullptr, &default_linear_sampler);


        uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        default_white_image = Texture::upload(device, allocator, transfer_pool, (void*)&white, VkExtent3D{1, 1, 1},
                                      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
        default_grey_image = Texture::upload(device, allocator, transfer_pool, (void*)&grey, VkExtent3D{1, 1, 1},
                                     VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
        default_black_image = Texture::upload(device, allocator, transfer_pool, (void*)&black, VkExtent3D{1, 1, 1},
                                      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        std::array<uint32_t, 16 * 16> pixels;
        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 16; y++) {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : white;
            }
        }
        error_checkerboard_image =
            Texture::upload(device, allocator, transfer_pool, pixels.data(), VkExtent3D{16, 16, 1},
                            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        MaterialOperation::MaterialResources material_resources;
        material_resources.color_image = default_white_image;
        material_resources.color_sampler = default_linear_sampler;
        material_resources.metal_rough_image = default_white_image;
        material_resources.metal_rough_sampler = default_linear_sampler;

        auto scene_resources = ResourceManagement::loadGLTF(
            device, "C:/Users/bgarner/Documents/repos/Corax/third-party/glTF-Sample-Assets/Models/DamagedHelmet/glTF-Binary/DamagedHelmet.glb", allocator, transfer_pool,
            error_checkerboard_image, material_resources, material_operations, pipeline_cache);
        loaded_scenes["structure"] = scene_resources.value();

        AllocatedBuffer materialConstants =
            Buffer::allocateBuffer(allocator, sizeof(MaterialOperation::MaterialResources),
                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        MaterialOperation::MaterialConstants* sceneUniformData =
            static_cast<MaterialOperation::MaterialConstants*>(materialConstants.info.pMappedData);
        sceneUniformData->color_factors = glm::vec4{1, 1, 1, 1};
        sceneUniformData->metal_factors = 1.0f;
        // My idea here is to give the structs constructors and destructors
        // But assert on the destructo function not being set so that any objects that
        // need explicit resource clean up always have that setup
        main_deletion_queue.pushDeleter([=, this]() { Buffer::destroyBuffer(allocator, materialConstants); });

        material_resources.data_buffer = materialConstants.buffer;
        material_resources.data_buffer_offset = 0;
    }

    void CoraxRenderer::updateScene(float delta_time) {
        main_draw_context.opaque_surfaces.clear();
        main_draw_context.transparent_surfaces.clear();
        Camera::updatePosition(fps_camera, delta_time);


        if (loaded_scenes["structure"] != nullptr)
        {
            loaded_scenes["structure"]->Draw(glm::mat4{ 1.f }, main_draw_context);
        }
        else{
            std::cout << "null structure" << std::endl;
        }

        static float angle = 0.0f;
        angle += delta_time * glm::radians(45.0f);

        scene_data.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

        scene_data.view = Camera::getViewMatrix(fps_camera);
        scene_data.projection = glm::perspective(
            glm::radians(70.f), (float)swap_chain.extent.width / (float)swap_chain.extent.height, 0.1f, 10000.f);
        scene_data.projection[1][1] *= -1;
        scene_data.view_projection = scene_data.projection * scene_data.view;

        scene_data.sunlight_color = glm::vec4(2.0f, 2.0f, 2.0f, 2.0f);
        scene_data.sunlight_direction = glm::vec4(0, 1, 0, 1.0f);
        scene_data.ambient_color = glm::vec4(5.0f, 5.0f, 5.0f, 5.0f);
        scene_data.camera_position = glm::vec4(Camera::getPosition(fps_camera));
        scene_data.light_position = glm::vec4(1.0f, 1.0f, 5.0f, 10.0f);
    }

    void CoraxRenderer::updateRenderingInfo() {

        assert(depth_image.imageView != VK_NULL_HANDLE);

        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.pNext = nullptr;
        depth_attachment.imageView = depth_image.imageView;
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.clearValue.depthStencil.depth = 1.0f;

        color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_info.pNext = nullptr;
        color_attachment_info.imageView = swap_chain.image_views[current_index];
        color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_info.clearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

        area.extent = swap_chain.extent;
        area.offset = {0, 0};

        render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        render_info.pNext = nullptr;
        render_info.renderArea = area;
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = 1;
        render_info.pColorAttachments = &color_attachment_info;
        render_info.pDepthAttachment = &depth_attachment;
    }

    void CoraxRenderer::beginFrame(float delta_time) {

        updateScene(delta_time);
        vkWaitForFences(device.logical_handle, 1, &frame_sync.frames[last_frame_index].in_flight_fence, VK_TRUE,
                        UINT64_MAX);
        frame_sync.frames[last_frame_index].deletion.flush();
        FrameResources& frame = frame_sync.frames[last_frame_index];
        Descriptors::clearPools(frame_sync.frames[last_frame_index].frame_descriptor_allocator, device);

        VkResult result = vkAcquireNextImageKHR(device.logical_handle, swap_chain.swap_chain, UINT64_MAX,
                                                frame_sync.frames[last_frame_index].image_available_semaphore,
                                                VK_NULL_HANDLE, &current_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        vkResetFences(device.logical_handle, 1, &frame_sync.frames[last_frame_index].in_flight_fence);
        vkResetCommandPool(device.logical_handle, frame_sync.frames[last_frame_index].command_pool, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        vkCheck(vkBeginCommandBuffer(frame_sync.frames[last_frame_index].command_buffer, &begin_info));

        updateRenderingInfo();
        Transition::image(frame_sync.frames[last_frame_index].command_buffer, swap_chain.images[current_index],
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        Transition::image(frame_sync.frames[last_frame_index].command_buffer, depth_image.image,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0,
                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        vkCmdBeginRenderingKHR(frame_sync.frames[last_frame_index].command_buffer, &render_info);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swap_chain.extent.width);
        viewport.height = static_cast<float>(swap_chain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(frame_sync.frames[last_frame_index].command_buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swap_chain.extent;
        vkCmdSetScissor(frame_sync.frames[last_frame_index].command_buffer, 0, 1, &scissor);

        AllocatedBuffer gpuSceneDataBuffer = Buffer::allocateBuffer(
            allocator, sizeof(Scene), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        frame_sync.frames[last_frame_index].deletion.pushDeleter(
            [=, this]() { Buffer::destroyBuffer(allocator, gpuSceneDataBuffer); });

        Scene* sceneUniformData = (Scene*)gpuSceneDataBuffer.info.pMappedData;
        *sceneUniformData = scene_data;
        VkDescriptorSet globalDescriptor = Descriptors::allocate(
            frame_sync.frames[last_frame_index].frame_descriptor_allocator, device, scene_layout.layout_handle);
        assert(globalDescriptor != VK_NULL_HANDLE);
        {
            Descriptors::writeBuffer(descriptor_write, 0, gpuSceneDataBuffer.buffer, sizeof(Scene), 0,
                                     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            Descriptors::updateSet(descriptor_write, device, globalDescriptor);
        }

        auto draw = [&](const MaterialOperation::RenderObject& draw) {
            vkCmdBindPipeline(frame_sync.frames[last_frame_index].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              draw.material->pipeline->handle);
            vkCmdBindDescriptorSets(frame_sync.frames[last_frame_index].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    draw.material->pipeline->layout_handle, 0, 1, &globalDescriptor, 0, nullptr);
            vkCmdBindDescriptorSets(frame_sync.frames[last_frame_index].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    draw.material->pipeline->layout_handle, 1, 1, &draw.material->material_set, 0,
                                    nullptr);

            vkCmdBindIndexBuffer(frame_sync.frames[last_frame_index].command_buffer, draw.index_buffer, 0,
                                 VK_INDEX_TYPE_UINT32);

            GPUDrawPushConstants pushConstants;
            pushConstants.vertexBuffer = draw.vertex_buffer_address;
            pushConstants.worldMatrix = draw.transform;
            vkCmdPushConstants(frame_sync.frames[last_frame_index].command_buffer,
                               draw.material->pipeline->layout_handle, VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(GPUDrawPushConstants), &pushConstants);

            vkCmdDrawIndexed(frame_sync.frames[last_frame_index].command_buffer, draw.index_ount, 1, draw.first_index,
                             0, 0);
        };

        for (const MaterialOperation::RenderObject& render_obj : main_draw_context.opaque_surfaces) {
            draw(render_obj);
        }

        for (const MaterialOperation::RenderObject& render_obj : main_draw_context.transparent_surfaces) {
            draw(render_obj);
        }

        Vulkan::vkCmdEndRenderingKHR(frame_sync.frames[last_frame_index].command_buffer);

        Transition::image(frame_sync.frames[last_frame_index].command_buffer, swap_chain.images[current_index],
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);

        endFrame(frame);
    }

    void CoraxRenderer::endFrame(FrameResources& frame) {
        vkCheck(vkEndCommandBuffer(frame_sync.frames[last_frame_index].command_buffer));

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[] = {frame_sync.frames[last_frame_index].image_available_semaphore};
        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &frame_sync.frames[last_frame_index].command_buffer;

        VkSemaphore signal_semaphores[] = {frame_sync.frames[last_frame_index].render_finished_semaphore};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        vkQueueSubmit(device.graphics_queue, 1, &submit_info, frame_sync.frames[last_frame_index].in_flight_fence);

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;

        VkSwapchainKHR swap_chains[] = {swap_chain.swap_chain};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swap_chains;
        present_info.pImageIndices = &current_index;

        VkResult result = vkQueuePresentKHR(device.present_queue, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || glfw_window.window_resized) {
            glfw_window.window_resized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        last_frame_index = frame_sync.advanceFrame();
    }

    void CoraxRenderer::processInputKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
        Camera::Type* fps_camera_context = static_cast<Camera::Type*>(glfwGetWindowUserPointer(window));
        Camera::updateVelocityFromEvent(*fps_camera_context, key, scancode, action, mods);
    }

    void CoraxRenderer::processInputMouseEvent(GLFWwindow* window, double xpos, double ypos) {
        Camera::Type* fps_camera_context = static_cast<Camera::Type*>(glfwGetWindowUserPointer(window));
        Camera::updatePitchYawFromEvent(*fps_camera_context, xpos, ypos);
    }

    void CoraxRenderer::mainLoop() {
        auto lastTime = std::chrono::high_resolution_clock::now();
        glfwSetWindowUserPointer(glfw_window.handle, &fps_camera);
        glfwSetInputMode(glfw_window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetKeyCallback(glfw_window.handle, CoraxRenderer::processInputKeyEvent);
        glfwSetCursorPosCallback(glfw_window.handle, processInputMouseEvent);
        while (!glfw_window.closeCheck()) {
            glfwPollEvents();
            auto currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            beginFrame(deltaTime.count());
        }
        vkDeviceWaitIdle(device.logical_handle);
    }

    void CoraxRenderer::destroy() {

        // In this experiment i am finding manual destruction a pain, however i dont want to use OOP, so
        // The destruction queue is one way, another i am considering is using a function callback set to run on destruction.
        

        vkDeviceWaitIdle(device.logical_handle);
        main_deletion_queue.flush();
        for (auto& n : loaded_scenes) {
            n.second->onDestroy();
        }
        vkDestroySampler(device.logical_handle, default_sampler_nearest, nullptr);
        vkDestroySampler(device.logical_handle, default_linear_sampler, nullptr);
        Texture::destroy(device, allocator, default_white_image);
        Texture::destroy(device, allocator, default_grey_image);
        Texture::destroy(device, allocator, default_black_image);
        Texture::destroy(device, allocator, error_checkerboard_image);
        Descriptors::destroyLayout(device, global_layout);
        Descriptors::clearLayoutBindings(global_layout);
        Descriptors::destroyLayout(device, scene_layout);
        Descriptors::clearLayoutBindings(scene_layout);
        Descriptors::destroyPools(global_descriptor_allocator, device);
        MaterialOperation::destroyResources(device, material_operations);
        Pipeline::destroyShaderModule(device, mesh_vertex);
        Pipeline::destroyShaderModule(device, mesh_fragment);
        Pipeline::clearCache(device, pipeline_cache);
        frame_sync.destroy(device);
        vkDestroyImage(device.logical_handle, depth_image.image, nullptr);
        vkDestroyImageView(device.logical_handle, depth_image.imageView, nullptr);
        CommandPool::destroyPool(device, transfer_pool);
        swap_chain.destroy(device);
        MemoryAllocator::destroyAllocator(allocator);
        device.destroy();
        glfw_window.destroy();
        instance.destroy();
    }

    void CoraxRenderer::recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(glfw_window.handle, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(glfw_window.handle, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(device.logical_handle);
        swap_chain.destroy(device);
        swap_chain.create(device, glfw_window, instance);
    }

    uint32_t CoraxRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(device.physical_handle, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void CoraxRenderer::initDepthImage() {
        VkExtent3D depth_image_extent = {swap_chain.extent.width, swap_chain.extent.height, 1};
        depth_image.imageFormat = VK_FORMAT_D32_SFLOAT;
        depth_image.imageExtent = depth_image_extent;
        VkImageUsageFlags depthImageUsages{};
        depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkImageCreateInfo dimg_info{};
        dimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        dimg_info.imageType = VK_IMAGE_TYPE_2D;
        dimg_info.format = depth_image.imageFormat;
        dimg_info.usage = depthImageUsages;
        dimg_info.extent = depth_image_extent;
        dimg_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        dimg_info.samples = VK_SAMPLE_COUNT_1_BIT;
        dimg_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        dimg_info.extent.depth = 1;
        dimg_info.mipLevels = 1;
        dimg_info.arrayLayers = 1;
        dimg_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        dimg_info.samples = VK_SAMPLE_COUNT_1_BIT;
        dimg_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateImage(device.logical_handle, &dimg_info, nullptr, &depth_image.image);

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device.logical_handle, depth_image.image, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits, VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        if (vkAllocateMemory(device.logical_handle, &allocInfo, nullptr, &depth_image.memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device.logical_handle, depth_image.image, depth_image.memory, 0);

        VkImageViewCreateInfo dview_info{};
        dview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        dview_info.format = depth_image.imageFormat;
        dview_info.image = depth_image.image;
        dview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        dview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;  // Set aspectMask
        dview_info.subresourceRange.baseMipLevel = 0;
        dview_info.subresourceRange.levelCount = 1;
        dview_info.subresourceRange.baseArrayLayer = 0;
        dview_info.subresourceRange.layerCount = 1;

        vkCheck(vkCreateImageView(device.logical_handle, &dview_info, nullptr, &depth_image.imageView));
    }

};  // namespace Vulkan
