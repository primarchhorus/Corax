#pragma once

#include "vulkan_common.h"

#include <array>

namespace Vulkan {

    struct Device;
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;

    struct FrameResources {
        VkCommandPool command_pool{VK_NULL_HANDLE};
        VkCommandBuffer command_buffer{VK_NULL_HANDLE};

        VkSemaphore image_available_semaphore{VK_NULL_HANDLE};
        VkSemaphore render_finished_semaphore{VK_NULL_HANDLE};
        VkFence in_flight_fence{VK_NULL_HANDLE};

        DescriptorAllocation frame_descriptor_allocator{.pool_size_ratios = { 
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
        }, .num_pools = 1, .sets_per_pool = 1000};

        DeletionQueue deletion;
    };

    struct FrameSync {
        FrameSync();
        ~FrameSync();
        FrameSync(const FrameSync &) = delete;
        FrameSync &operator=(const FrameSync &) = delete;
        FrameSync(FrameSync &&other) noexcept;
        FrameSync &operator=(FrameSync &&other) noexcept;

        void create(const Device& device);
        void destroy(const Device& device);
        uint64_t advanceFrame();
        
        uint32_t current_frame{0};
        std::array<FrameResources, FRAMES_IN_FLIGHT> frames;
    };
}