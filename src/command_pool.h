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

        void createPool(Device& device);
        void createBuffer(Device& device, std::vector<VkCommandBuffer>& buffer);
        void destroy(Device& device);

        VkCommandPool handle;
    };
}
