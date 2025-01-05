#include "swap_chain.h"
#include "window.h"
#include "vulkan_utils.h"
#include "device.h"

namespace Vulkan {
    SwapChain::SwapChain()
    {
    }

    SwapChain::SwapChain(SwapChain&& other) noexcept
        : swap_chain(other.swap_chain) {
        other.swap_chain = nullptr;
    }

    SwapChain& SwapChain::operator=(SwapChain&& other) noexcept {
        if (this != &other) {
            swap_chain = other.swap_chain;
        }
        return *this;
    }

    SwapChain::~SwapChain()
    {
    }

    void SwapChain::destroy(const Device& device)
    {
        vkDestroySwapchainKHR(device.logical_handle, swap_chain, nullptr);
        for (auto view : image_views)
        {
            vkDestroyImageView(device.logical_handle, view, nullptr);
        }
    }

    void SwapChain::init(Device& device, Window& window)
    {
        createSwapChain(device, window);
        createImageViews(device);
    }

    VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const Device& device)
    {
        for (const auto& available_format : device.suitability.formats) {
            if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return available_format;
            }
        }
        return device.suitability.formats[0];
    }

    VkPresentModeKHR SwapChain::chooseSwapPresentMode(const Device& device)
    {
        for (const auto& available_present_mode : device.suitability.present_modes) {
            if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return available_present_mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::chooseSwapExtent(const Device& device, Window& window)
    {
      if (device.suitability.capabilities.currentExtent.width !=
          std::numeric_limits<uint32_t>::max())
      {
        return device.suitability.capabilities.currentExtent;
      }
      else
      {
        int width, height;
        window.getWindowFramebufferSize(width, height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                   static_cast<uint32_t>(height)};

        actualExtent.width =
            std::clamp(actualExtent.width,
                       device.suitability.capabilities.minImageExtent.width,
                       device.suitability.capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height,
                       device.suitability.capabilities.minImageExtent.height,
                       device.suitability.capabilities.maxImageExtent.height);

        return actualExtent;
      }
    }

    void SwapChain::createSwapChain(Device& device, Window& window) {
        VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(device);
        VkPresentModeKHR present_mode = chooseSwapPresentMode(device);
        extent = chooseSwapExtent(device, window);
        uint32_t image_count = device.suitability.capabilities.minImageCount + 1;
        if (device.suitability.capabilities.maxImageCount > 0 && image_count > device.suitability.capabilities.maxImageCount) {
            image_count = device.suitability.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swap_chain_info{};
        swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swap_chain_info.surface = window.surface;
        swap_chain_info.minImageCount = image_count;
        swap_chain_info.imageFormat = surface_format.format;
        swap_chain_info.imageColorSpace = surface_format.colorSpace;
        swap_chain_info.imageExtent = extent;
        swap_chain_info.imageArrayLayers = 1;
        swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        image_format = surface_format.format;

        uint32_t fam_indexes[] = {device.suitability.queue_fam_indexes["draw"], device.suitability.queue_fam_indexes["present"]};

        if (device.suitability.queue_fam_indexes["draw"] != device.suitability.queue_fam_indexes["present"]) {
            swap_chain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swap_chain_info.queueFamilyIndexCount = 2;
            swap_chain_info.pQueueFamilyIndices = fam_indexes;
        } else {
            swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swap_chain_info.queueFamilyIndexCount = 0;
            swap_chain_info.pQueueFamilyIndices = nullptr;
        }

        swap_chain_info.preTransform = device.suitability.capabilities.currentTransform;
        swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swap_chain_info.presentMode = present_mode;
        swap_chain_info.clipped = VK_TRUE;
        swap_chain_info.oldSwapchain = VK_NULL_HANDLE;

        vkCheck(vkCreateSwapchainKHR(device.logical_handle, &swap_chain_info, nullptr, &swap_chain));

        vkGetSwapchainImagesKHR(device.logical_handle, swap_chain, &image_count, nullptr);
        images.resize(image_count);
        vkGetSwapchainImagesKHR(device.logical_handle, swap_chain, &image_count, images.data());

    }

    void SwapChain::createImageViews(const Device& device)
    {
        image_views.resize(images.size());
        for (size_t i = 0; i < images.size(); i++) {
            VkImageViewCreateInfo image_view_info{};
            image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_info.image = images[i];
            image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_info.format = image_format;
            image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_info.subresourceRange.baseMipLevel = 0;
            image_view_info.subresourceRange.levelCount = 1;
            image_view_info.subresourceRange.baseArrayLayer = 0;
            image_view_info.subresourceRange.layerCount = 1;

            vkCheck(vkCreateImageView(device.logical_handle, &image_view_info, nullptr, &image_views[i]));
        }
    }
}
