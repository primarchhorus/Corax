#pragma once

#include "vulkan_common.h"
#include "command_pool.h"

namespace Vulkan {

    struct Device;
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;

    struct FrameSync {
        FrameSync(Device& device);
        ~FrameSync();
        FrameSync(const FrameSync &) = delete;
        FrameSync &operator=(const FrameSync &) = delete;
        FrameSync(FrameSync &&other) noexcept;
        FrameSync &operator=(FrameSync &&other) noexcept;

        void create();
        void destroy();
        void advanceFrame();
        
        Device& device_ref;
        CommandPool command_pool;
        std::vector<VkSemaphore> image_available_semaphores;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<VkFence> in_flight_fences;
        std::vector<VkCommandBuffer> command_buffers;
        uint32_t current_frame{0};
    };
}