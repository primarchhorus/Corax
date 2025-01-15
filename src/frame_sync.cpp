#include "frame_sync.h"
#include "swap_chain.h"
#include "vulkan_utils.h"
#include "device.h"

namespace Vulkan {
    FrameSync::FrameSync(Device& device) : device_ref(device)
    {
        command_pool.createPool(device);
        create();
    }

    FrameSync::~FrameSync()
    {
        try
        {
            destroy();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    FrameSync::FrameSync(FrameSync&& other) noexcept
        : image_available_semaphores(std::move(other.image_available_semaphores)), render_finished_semaphores(std::move(other.render_finished_semaphores)), in_flight_fences(std::move(other.in_flight_fences)), device_ref(other.device_ref) {

    }

    FrameSync& FrameSync::operator=(FrameSync&& other) noexcept {
        if (this != &other) {
            image_available_semaphores = std::move(other.image_available_semaphores);
            render_finished_semaphores = std::move(other.render_finished_semaphores); 
            in_flight_fences = std::move(other.in_flight_fences);
        }
        return *this;
    }

    void FrameSync::create() 
    {
        image_available_semaphores.resize(FRAMES_IN_FLIGHT);
        render_finished_semaphores.resize(FRAMES_IN_FLIGHT);
        in_flight_fences.resize(FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            vkCheck(vkCreateSemaphore(device_ref.logical_handle, &semaphore_info, nullptr, &image_available_semaphores[i]));
            vkCheck(vkCreateSemaphore(device_ref.logical_handle, &semaphore_info, nullptr, &render_finished_semaphores[i]));
            vkCheck(vkCreateFence(device_ref.logical_handle, &fence_info, nullptr, &in_flight_fences[i]));
        }

        command_buffers.resize(FRAMES_IN_FLIGHT);
        command_pool.createBuffer(device_ref, command_buffers);
    }
        
    void FrameSync::advanceFrame()
    {
        current_frame = (current_frame + 1) & (FRAMES_IN_FLIGHT - 1);;
    }

    void FrameSync::destroy() {
        for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device_ref.logical_handle, image_available_semaphores[i], nullptr);
            vkDestroySemaphore(device_ref.logical_handle, render_finished_semaphores[i], nullptr);
            vkDestroyFence(device_ref.logical_handle, in_flight_fences[i], nullptr);
        }
        command_pool.destroy(device_ref);
    }
}