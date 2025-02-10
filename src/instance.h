#pragma once

#include "vulkan_common.h"

namespace Vulkan {
  struct Window;
  struct Instance {
    Instance();
    ~Instance();
    Instance(const Instance &) = delete;
    Instance &operator=(const Instance &) = delete;
    Instance(Instance &&other) noexcept;
    Instance &operator=(Instance &&other) noexcept;

    void create();
    void destroy();
    void createSurface(const Window& window);

    const std::vector<const char *> validation_layers = {
        "VK_LAYER_KHRONOS_validation"};
    const bool enable_validation{true};
    VkInstance handle;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;
  };
} // namespace Vulkan
