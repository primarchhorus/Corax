#pragma once

#include "vulkan_common.h"
#include <vulkan/vulkan_core.h>

#include <map>

namespace Vulkan {

class Instance;
class Window;

struct DeviceSuitability {
    std::map<std::string, uint32_t> queue_fam_indexes;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t queue_fam_total{0};
    VkBool32 properties_suitable{false};
    VkBool32 features_suitable{false};
    VkBool32 extension_suitable{false};
    VkBool32 queue_fam_draw_suitable{false};
    VkBool32 queue_fam_present_suitable{false};
    uint32_t queue_fam_draw_index{0};

    bool result() {
        bool result{true};
        result = result && (properties_suitable && true);
        result = result && (features_suitable | true);
        result = result && (queue_fam_draw_suitable | true);
        result = result && (queue_fam_present_suitable && true);
        result = result && (extension_suitable && true);
        result = result && (!formats.empty() && !present_modes.empty());
        return result;
    }
};

struct Device {
    Device(const Instance& instance);
    ~Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

    void init(const Instance& instance);
    void initLogical(const Instance& instance);
    void destroy();
    void initPhysical(const Instance& instance);
    void checkDeviceSuitability(const VkPhysicalDevice& device, DeviceSuitability& suitability, const Instance& instance);
    void checkDeviceExtensions(const VkPhysicalDevice& device, const Instance& instance);
    void querySwapChainSupport(const VkPhysicalDevice& device, const Instance& instance);
    void initQueues();

    VkPhysicalDevice physical_handle{VK_NULL_HANDLE};
    VkDevice logical_handle{VK_NULL_HANDLE};
    DeviceSuitability suitability;
    const std::vector<const char*> required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, // Dynamic Rendering Dependency.
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, // Dynamic Rendering Dependency.
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };

    VkQueue graphics_queue;
    VkQueue present_queue;
    //TODO add the othere here over time


};
}
