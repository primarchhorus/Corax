#pragma once

#include "vulkan_common.h"

namespace Vulkan {

    struct Instance;
    struct Window;
    struct Device;

    struct SwapChain {
        SwapChain();
        ~SwapChain();
        SwapChain(const SwapChain &) = delete;
        SwapChain &operator=(const SwapChain &) = delete;
        SwapChain(SwapChain &&other) noexcept;
        SwapChain &operator=(SwapChain &&other) noexcept;

        void create(const Device& device, const Window& window, const Instance& instance);
        void destroy(const Device& device);

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const Device& device);
        VkPresentModeKHR chooseSwapPresentMode(const Device& device);
        VkExtent2D chooseSwapExtent(const Device& device, const Window& window);
        void createSwapChain(const Device& device, const Window& window, const Instance& instance);
        void createImageViews(const Device& device);

        VkSwapchainKHR swap_chain{};
        std::vector<VkImage> images{};
        std::vector<VkImageView> image_views{};
        std::vector<VkFramebuffer> frame_buffers{};
        VkFormat image_format{};
        VkExtent2D extent{};

    };
} // namespace Vulkan
