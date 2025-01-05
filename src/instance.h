#pragma once

#include "vulkan_common.h"

namespace Vulkan {

struct Instance {
  Instance();
  ~Instance();
  Instance(const Instance &) = delete;
  Instance &operator=(const Instance &) = delete;
  Instance(Instance &&other) noexcept;
  Instance &operator=(Instance &&other) noexcept;

  void init();
  void destroy();

  const std::vector<const char *> validation_layers = {
      "VK_LAYER_KHRONOS_validation", "VK_KHR_portability_subset"};
  const bool enable_validation{true};
  VkInstance handle;
  VkDebugUtilsMessengerEXT debug_messenger;
};
} // namespace Vulkan
