#pragma once

#include "vulkan_common.h"
#include "vulkan_utils.h"

namespace Vulkan {
    struct Device;
    struct CommandPool {
        CommandPool();
        ~CommandPool();
        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;
        CommandPool(CommandPool&& other) noexcept;
        CommandPool& operator=(CommandPool&& other) noexcept;

        void createPool(const Device& device);
        void createBuffer(const Device& device, std::vector<VkCommandBuffer>& buffer);
        void freeBuffers(const Device& device, std::vector<VkCommandBuffer>& buffer, uint32_t buffer_count);
        void destroy(const Device& device);

        VkCommandPool handle;
    };
}
