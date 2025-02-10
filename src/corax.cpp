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

        Descriptors::addLayoutBinding(
            global_layout, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
        Descriptors::buildLayout(
            global_layout, device,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        Descriptors::addLayoutBinding(
            scene_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
        Descriptors::buildLayout(
            scene_layout, device,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        mesh_vertex.filename =
            "C:/Users/bgarner/Documents/repos/Corax/build/shaders/"
            "shader.vert.spv";
        Pipeline::loadShader(mesh_vertex);
        Pipeline::createShaderModule(device, mesh_vertex);
        mesh_fragment.filename =
            "C:/Users/bgarner/Documents/repos/Corax/build/shaders/"
            "shader.frag.spv";
        Pipeline::loadShader(mesh_fragment);
        Pipeline::createShaderModule(device, mesh_fragment);

        basic_mesh_pipeline_config.name = "triangle_pipeline";
        VkPipelineShaderStageCreateInfo vertex_info{};
        vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_info.module = mesh_vertex.module;
        vertex_info.pName = "main";
        basic_mesh_pipeline_config.vertex_stages = vertex_info;

        VkPipelineShaderStageCreateInfo frag_info{};
        frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_info.module = mesh_fragment.module;
        frag_info.pName = "main";
        basic_mesh_pipeline_config.fragment_stages = frag_info;
        basic_mesh_pipeline_config.format = swap_chain.image_format;
        basic_mesh_pipeline_config.extent = swap_chain.extent;

        VkDescriptorSetLayout layouts[] = {global_layout.layout_handle};
        basic_mesh_pipeline_config.descriptor_set_layout = layouts;
        basic_mesh_pipeline_config.num_descriptor_sets = 1;

        auto mesh_pipeline =
            Pipeline::createPipelineObject(device, basic_mesh_pipeline_config);
        Pipeline::addPipelineToCache(basic_mesh_pipeline_config, pipeline_cache,
                                     std::move(mesh_pipeline));

        MaterialOperation::buildPipelines(device, swap_chain,
                                          material_operations, pipeline_cache,
                                          scene_layout);

        VkSamplerCreateInfo sampl = {.sType =
                                         VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

        sampl.magFilter = VK_FILTER_NEAREST;
        sampl.minFilter = VK_FILTER_NEAREST;

        vkCreateSampler(device.logical_handle, &sampl, nullptr,
                        &_defaultSamplerNearest);

        sampl.magFilter = VK_FILTER_LINEAR;
        sampl.minFilter = VK_FILTER_LINEAR;
        vkCreateSampler(device.logical_handle, &sampl, nullptr,
                        &_defaultSamplerLinear);

        // resources.loadAsset(
        //     device, "C:/Users/bgarner/Documents/repos/Corax/assets/structure.glb",
        //     allocator, transfer_pool);

        auto mesh_resources = ResourceManagement::loadAssets(
            device,
            "C:/Users/bgarner/Documents/repos/Corax/assets/AntiqueCamera.glb",
            allocator, transfer_pool);
        main_deletion_queue.pushDeleter([=, this]() {
            for (auto m : mesh_resources) {
                MeshOperations::destroyMesh(allocator, m->mesh_buffers);
            }
        });
        uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        _whiteImage =
            Texture::upload(device, allocator, transfer_pool, (void*)&white,
                            VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
        _greyImage = Texture::upload(
            device, allocator, transfer_pool, (void*)&grey, VkExtent3D{1, 1, 1},
            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
        _blackImage =
            Texture::upload(device, allocator, transfer_pool, (void*)&black,
                            VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_USAGE_SAMPLED_BIT);

        //checkerboard image
        uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        std::array<uint32_t, 16 * 16> pixels;  //for 16x16 checkerboard texture
        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 16; y++) {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : white;
            }
        }
        _errorCheckerboardImage =
            Texture::upload(device, allocator, transfer_pool, pixels.data(),
                            VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_USAGE_SAMPLED_BIT);

        MaterialOperation::MaterialResources material_resources;
        //default the material textures
        material_resources.color_image = _whiteImage;
        material_resources.color_sampler = _defaultSamplerLinear;
        material_resources.metal_rough_image = _whiteImage;
        material_resources.metal_rough_sampler = _defaultSamplerLinear;

        // set the uniform buffer for the material data

        AllocatedBuffer materialConstants = Buffer::allocateBuffer(
            allocator, sizeof(MaterialOperation::MaterialResources),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        // write the buffer
        MaterialOperation::MaterialConstants* sceneUniformData =
            static_cast<MaterialOperation::MaterialConstants*>(
                materialConstants.info.pMappedData);
        sceneUniformData->color_factors = glm::vec4{1, 1, 1, 1};
        sceneUniformData->metal_rough_factors = glm::vec4{1, 0.5, 0, 0};
        // My idea here is to give the structs constructors and destructors
        // But assert on the destructo function not being set so that any objects that 
        // need explicit resource clean up always have that setup
        main_deletion_queue.pushDeleter([=, this]() {
            Buffer::destroyBuffer(allocator, materialConstants);
        });

        material_resources.data_buffer = materialConstants.buffer;
        material_resources.data_buffer_offset = 0;

        default_data = MaterialOperation::writeMaterial(
            device, MaterialOperation::MaterialPass::MAINCOLOUR,
            material_resources, global_descriptor_allocator,
            material_operations, pipeline_cache);

        // seems like double handling since it loops for the load anyway, might be able to combine with the load asset?
        for (auto& m : mesh_resources) {
            std::shared_ptr<MaterialOperation::MeshNode> newNode =
                std::make_shared<MaterialOperation::MeshNode>();
            newNode->mesh = m;
            newNode->localTransform = glm::mat4{1.f};
            newNode->worldTransform = glm::mat4{1.f};

            for (auto& s : newNode->mesh->surfaces) {
                s.material =
                    std::make_shared<MaterialOperation::MaterialInstance>(
                        default_data);
            }

            loaded_nodes[m->name] = std::move(newNode);
        }
    }

    void CoraxRenderer::updateScene(float delta_time) {
        main_draw_context.opaque_surfaces.clear();
        Camera::updatePosition(fps_camera, delta_time);

        for (auto& n: loaded_nodes)
        {
            n.second->Draw(glm::mat4{1.f}, main_draw_context);
        }

        static float angle = 0.0f;
        angle += delta_time * glm::radians(45.0f);

        scene_data.model =
            glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

        scene_data.view = Camera::getViewMatrix(fps_camera);
        scene_data.projection = glm::perspective(
            glm::radians(70.f), (float)swap_chain.extent.width / (float)swap_chain.extent.height,
            10000.f, 0.1f);
        scene_data.projection[1][1] *= -1;
        scene_data.view_projection = scene_data.projection * scene_data.view;

        scene_data.ambient_color = glm::vec4(.1f);
        scene_data.sunlight_color = glm::vec4(1.f);
        scene_data.sunlight_direction = glm::vec4(0, 1, 0.5, 1.f);
    }

    void CoraxRenderer::createPipelineObjects() {
        // pipeline_builder.buildPipeLines(device, swap_chain);
    }

    void CoraxRenderer::updateRenderingInfo() {

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.pNext = nullptr;
        depthAttachment.imageView = depth_image.imageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil.depth = 0.0f;
        depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;

        color_attachment_info.sType =
            VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        color_attachment_info.imageView = swap_chain.image_views[current_index];
        color_attachment_info.imageLayout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_info.clearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

        area.extent = swap_chain.extent;
        area.offset = {0, 0};

        render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        render_info.pNext = nullptr;
        render_info.renderArea = area;
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = 1;
        render_info.pColorAttachments = &color_attachment_info;
        // render_info.pDepthAttachment = &depthAttachment;
    }

    void CoraxRenderer::beginFrame(float delta_time) {

        updateScene(delta_time);
        vkWaitForFences(device.logical_handle, 1,
                        &frame_sync.frames[last_frame_index].in_flight_fence,
                        VK_TRUE, UINT64_MAX);
        frame_sync.frames[last_frame_index].deletion.flush();
        FrameResources& frame = frame_sync.frames[last_frame_index];
        Descriptors::clearPools(
            frame_sync.frames[last_frame_index].frame_descriptor_allocator,
            device);

        VkResult result = vkAcquireNextImageKHR(
            device.logical_handle, swap_chain.swap_chain, UINT64_MAX,
            frame_sync.frames[last_frame_index].image_available_semaphore,
            VK_NULL_HANDLE, &current_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        vkResetFences(device.logical_handle, 1,
                      &frame_sync.frames[last_frame_index].in_flight_fence);
        vkResetCommandPool(device.logical_handle,
                           frame_sync.frames[last_frame_index].command_pool, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        vkCheck(vkBeginCommandBuffer(
            frame_sync.frames[last_frame_index].command_buffer, &begin_info));

        updateRenderingInfo();

        Transition::image(frame_sync.frames[last_frame_index].command_buffer,
                          swap_chain.images[current_index],
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        Transition::image(frame_sync.frames[last_frame_index].command_buffer,
                          depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0,
                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        vkCmdBeginRenderingKHR(
            frame_sync.frames[last_frame_index].command_buffer, &render_info);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swap_chain.extent.width);
        viewport.height = static_cast<float>(swap_chain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(frame_sync.frames[last_frame_index].command_buffer, 0,
                         1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swap_chain.extent;
        vkCmdSetScissor(frame_sync.frames[last_frame_index].command_buffer, 0,
                        1, &scissor);


        AllocatedBuffer gpuSceneDataBuffer = Buffer::allocateBuffer(
            allocator, sizeof(Scene), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        frame_sync.frames[last_frame_index].deletion.pushDeleter([=, this]() {
            Buffer::destroyBuffer(allocator, gpuSceneDataBuffer);
        });

        Scene* sceneUniformData = (Scene*)gpuSceneDataBuffer.info.pMappedData;
        *sceneUniformData = scene_data;
        VkDescriptorSet globalDescriptor = Descriptors::allocate(
            frame_sync.frames[last_frame_index].frame_descriptor_allocator,
            device, scene_layout.layout_handle);
        assert(globalDescriptor != VK_NULL_HANDLE);
        {
            Descriptors::writeBuffer(descriptor_write, 0,
                                     gpuSceneDataBuffer.buffer, sizeof(Scene),
                                     0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            Descriptors::updateSet(descriptor_write, device, globalDescriptor);
        }

        for (const MaterialOperation::RenderObject& draw :
             main_draw_context.opaque_surfaces) {

            vkCmdBindPipeline(
                frame_sync.frames[last_frame_index].command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                draw.material->pipeline->handle);
            vkCmdBindDescriptorSets(
                frame_sync.frames[last_frame_index].command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                draw.material->pipeline->layout_handle, 0, 1, &globalDescriptor,
                0, nullptr);
            vkCmdBindDescriptorSets(
                frame_sync.frames[last_frame_index].command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                draw.material->pipeline->layout_handle, 1, 1,
                &draw.material->material_set, 0, nullptr);

            vkCmdBindIndexBuffer(
                frame_sync.frames[last_frame_index].command_buffer,
                draw.index_buffer, 0, VK_INDEX_TYPE_UINT32);

            GPUDrawPushConstants pushConstants;
            pushConstants.vertexBuffer = draw.vertex_buffer_address;
            pushConstants.worldMatrix = draw.transform;
            vkCmdPushConstants(
                frame_sync.frames[last_frame_index].command_buffer,
                draw.material->pipeline->layout_handle,
                VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants),
                &pushConstants);

            vkCmdDrawIndexed(frame_sync.frames[last_frame_index].command_buffer,
                             draw.index_ount, 1, draw.first_index, 0, 0);
        }

        Vulkan::vkCmdEndRenderingKHR(
            frame_sync.frames[last_frame_index].command_buffer);

        Transition::image(frame_sync.frames[last_frame_index].command_buffer,
                          swap_chain.images[current_index],
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);

        endFrame(frame);
    }

    void CoraxRenderer::endFrame(FrameResources& frame) {
        vkCheck(vkEndCommandBuffer(
            frame_sync.frames[last_frame_index].command_buffer));

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[] = {
            frame_sync.frames[last_frame_index].image_available_semaphore};
        VkPipelineStageFlags wait_stages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers =
            &frame_sync.frames[last_frame_index].command_buffer;

        VkSemaphore signal_semaphores[] = {
            frame_sync.frames[last_frame_index].render_finished_semaphore};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        vkQueueSubmit(device.graphics_queue, 1, &submit_info,
                      frame_sync.frames[last_frame_index].in_flight_fence);

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;

        VkSwapchainKHR swap_chains[] = {swap_chain.swap_chain};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swap_chains;
        present_info.pImageIndices = &current_index;

        VkResult result =
            vkQueuePresentKHR(device.present_queue, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            glfw_window.window_resized) {
            glfw_window.window_resized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        last_frame_index = frame_sync.advanceFrame();
    }

    void CoraxRenderer::processInputKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        Camera::Type* fps_camera_context = static_cast<Camera::Type*>(glfwGetWindowUserPointer(window));
        Camera::updateVelocityFromEvent(*fps_camera_context, key, scancode, action, mods);
        
    }

    void CoraxRenderer::processInputMouseEvent(GLFWwindow* window, double xpos, double ypos)
    {
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
        main_deletion_queue.flush();
        vkDeviceWaitIdle(device.logical_handle);
        vkDestroySampler(device.logical_handle, _defaultSamplerNearest,
                         nullptr);
        vkDestroySampler(device.logical_handle, _defaultSamplerLinear, nullptr);
        Texture::destroy(device, allocator, _whiteImage);
        Texture::destroy(device, allocator, _greyImage);
        Texture::destroy(device, allocator, _blackImage);
        Texture::destroy(device, allocator, _errorCheckerboardImage);
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
        vkDestroyImageView(device.logical_handle, depth_image.imageView,
                           nullptr);
        vkFreeMemory(device.logical_handle, depth_image.memory, nullptr);
        CommandPool::destroyPool(device, transfer_pool);
        MemoryAllocator::destroyAllocator(allocator);
        swap_chain.destroy(device);
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

    uint32_t CoraxRenderer::findMemoryType(uint32_t typeFilter,
                                           VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(device.physical_handle,
                                            &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) ==
                    properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void CoraxRenderer::initDepthImage() {
        VkExtent3D depth_image_extent = {swap_chain.extent.width,
                                         swap_chain.extent.height, 1};
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

        vkCreateImage(device.logical_handle, &dimg_info, nullptr,
                      &depth_image.image);

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device.logical_handle, depth_image.image,
                                     &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits,
            VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        if (vkAllocateMemory(device.logical_handle, &allocInfo, nullptr,
                             &depth_image.memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device.logical_handle, depth_image.image,
                          depth_image.memory, 0);

        VkImageViewCreateInfo dview_info{};
        dview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        dview_info.format = depth_image.imageFormat;
        dview_info.image = depth_image.image;
        dview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        dview_info.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT;  // Set aspectMask
        dview_info.subresourceRange.baseMipLevel = 0;
        dview_info.subresourceRange.levelCount = 1;
        dview_info.subresourceRange.baseArrayLayer = 0;
        dview_info.subresourceRange.layerCount = 1;

        vkCheck(vkCreateImageView(device.logical_handle, &dview_info, nullptr,
                                  &depth_image.imageView));
    }

};  // namespace Vulkan
