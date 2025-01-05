#pragma once

#include "vulkan_common.h"

namespace Vulkan {

class Instance;
class Window;
class Device;

struct SwapChain {
  SwapChain();
  ~SwapChain();
  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;
  SwapChain(SwapChain &&other) noexcept;
  SwapChain &operator=(SwapChain &&other) noexcept;

  void init(Device& device, Window& window);
  void initSurface(const Window& window);
  void destroy(const Device& device);

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const Device& device);
  VkPresentModeKHR chooseSwapPresentMode(const Device& device);
  VkExtent2D chooseSwapExtent(const Device& device, Window& window);
  void createSwapChain(Device& device, Window& window);
  void createImageViews(const Device& device);

  VkSwapchainKHR swap_chain;
  std::vector<VkImage> images;
  std::vector<VkImageView> image_views;
  VkFormat image_format;
  VkExtent2D extent;

};
} // namespace Vulkan

// Dont give any references to the constructors, dont rely on the destructors, call explicity destroy on all objects and pass device or instance to the functions that need them
