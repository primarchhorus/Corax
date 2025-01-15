#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {
    extern PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
    extern PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;

    void loadDynamicRenderingFunctions(VkDevice device);
}