#pragma once

#include "vulkan_common.h"

namespace Vulkan {

class Instance;
class Window;
class Device;

struct SwapChain {
  SwapChain(Device& device, Window& window, const Instance& instance);
  ~SwapChain();
  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;
  SwapChain(SwapChain &&other) noexcept;
  SwapChain &operator=(SwapChain &&other) noexcept;

  void init(Window& window, const Instance& instance);
  void destroy();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat();
  VkPresentModeKHR chooseSwapPresentMode();
  VkExtent2D chooseSwapExtent(Window& window);
  void createSwapChain(Window& window, const Instance& instance);
  void createImageViews();
  void createFrameBuffers();

  VkSwapchainKHR swap_chain;
  std::vector<VkImage> images;
  std::vector<VkImageView> image_views;
  std::vector<VkFramebuffer> frame_buffers;
  VkFormat image_format;
  VkExtent2D extent;
  Device& device_ref;

};
} // namespace Vulkan

// Dont give any references to the constructors, dont rely on the destructors, call explicity destroy on all objects and pass device or instance to the functions that need them
