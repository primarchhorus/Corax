#include "dynamic_rendering.h"
#include <stdexcept>

namespace Vulkan {
    PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
    PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;

    void loadDynamicRenderingFunctions(VkDevice device) {
        vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR");
        vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR");

        if (!vkCmdBeginRenderingKHR || !vkCmdEndRenderingKHR) {
            throw std::runtime_error("Failed to load dynamic rendering functions");
        }
    }
}