#include "frame_sync.h"
#include "swap_chain.h"
#include "vulkan_utils.h"
#include "device.h"
#include "vulkan_operations.h"

namespace Vulkan {
    FrameSync::FrameSync()
    {  
    }

    FrameSync::~FrameSync()
    {
    }

    FrameSync::FrameSync(FrameSync&& other) noexcept
    {
        *this = std::move(other);
    }

    FrameSync& FrameSync::operator=(FrameSync&& other) noexcept {
        if (this != &other) 
        {
            std::swap(frames, other.frames);
            std::swap(current_frame, other.current_frame);
        }
        return *this;
    }

    void FrameSync::create(const Device& device) 
    {
        for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        {

            VkCommandPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.queueFamilyIndex = device.suitability.queue_fam_indexes.at("draw");
            pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vkCheck(vkCreateCommandPool(device.logical_handle, &pool_info, nullptr, 
                                        &frames[i].command_pool));

            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandPool = frames[i].command_pool;
            alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandBufferCount = 1;
            vkCheck(vkAllocateCommandBuffers(device.logical_handle, &alloc_info, 
                                            &frames[i].command_buffer));

            VkSemaphoreCreateInfo sem_info{};
            sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            vkCheck(vkCreateSemaphore(device.logical_handle, &sem_info, nullptr, 
                                    &frames[i].image_available_semaphore));
            vkCheck(vkCreateSemaphore(device.logical_handle, &sem_info, nullptr, 
                                    &frames[i].render_finished_semaphore));

            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            vkCheck(vkCreateFence(device.logical_handle, &fence_info, nullptr, 
                                &frames[i].in_flight_fence));

            Descriptors::initPool(frames[i].frame_descriptor_allocator, device);
        }
    }
        
    uint64_t FrameSync::advanceFrame()
    {
        current_frame++;
        return (current_frame % FRAMES_IN_FLIGHT);
    }

    // Maybe add a dynamic clear here, clean out any frame data from the not in use frames

    void FrameSync::destroy(const Device& device) {

        vkDeviceWaitIdle(device.logical_handle);  // ensure no GPU operations are pending

        for (auto& frame : frames)
        {
            frame.deletion.flush();

            if (frame.command_pool)
            {
                // Free command buffers automatically with command pool destruction
                vkDestroyCommandPool(device.logical_handle, frame.command_pool, nullptr);
                frame.command_pool = VK_NULL_HANDLE;
                frame.command_buffer = VK_NULL_HANDLE;
            }
            if (frame.image_available_semaphore)
            {
                vkDestroySemaphore(device.logical_handle, frame.image_available_semaphore, nullptr);
                frame.image_available_semaphore = VK_NULL_HANDLE;
            }
            if (frame.render_finished_semaphore)
            {
                vkDestroySemaphore(device.logical_handle, frame.render_finished_semaphore, nullptr);
                frame.render_finished_semaphore = VK_NULL_HANDLE;
            }
            if (frame.in_flight_fence)
            {
                vkDestroyFence(device.logical_handle, frame.in_flight_fence, nullptr);
                frame.in_flight_fence = VK_NULL_HANDLE;
            }

            Descriptors::destroyPools(frame.frame_descriptor_allocator, device);
        }
    }
}