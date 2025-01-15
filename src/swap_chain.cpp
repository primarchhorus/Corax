#include "swap_chain.h"
#include "window.h"
#include "instance.h"
#include "vulkan_utils.h"
#include "device.h"

#include <algorithm>

namespace Vulkan {
    SwapChain::SwapChain(Device& device, Window& window, const Instance& instance) : device_ref(device)
    {
        init(window, instance);
    }

    SwapChain::SwapChain(SwapChain&& other) noexcept
        : swap_chain(other.swap_chain), device_ref(other.device_ref), image_format(other.image_format), extent(other.extent), images(std::move(other.images)), image_views(std::move(other.image_views)) {
        
        other.swap_chain = VK_NULL_HANDLE;
        other.image_format = VK_FORMAT_UNDEFINED;
        other.extent = {0, 0};
        other.images.clear();
        other.image_views.clear();
    }

    SwapChain& SwapChain::operator=(SwapChain&& other) noexcept {
        if (this != &other) {
            destroy();
            swap_chain = other.swap_chain;
            image_format = other.image_format;
            extent = other.extent;
            images = other.images;
            image_views = other.image_views;
            // device_ref remains the same as it is a reference
            other.swap_chain = VK_NULL_HANDLE;
            other.image_format = VK_FORMAT_UNDEFINED;
            other.extent = {0, 0};
            other.images.clear();
            other.image_views.clear();
        }
        return *this;
    }

    SwapChain::~SwapChain()
    {
        try
        {
            destroy();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    void SwapChain::destroy()
    {
        for (auto view : image_views)
        {
            vkDestroyImageView(device_ref.logical_handle, view, nullptr);
        }
        // image_views.clear();
        vkDestroySwapchainKHR(device_ref.logical_handle, swap_chain, nullptr);

    }

    void SwapChain::init(Window& window, const Instance& instance)
    {
        createSwapChain(window, instance);
        createImageViews();
    }

    VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat()
    {
        for (const auto& available_format : device_ref.suitability.formats) {
            if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return available_format;
            }
        }
        return device_ref.suitability.formats[0];
    }

    VkPresentModeKHR SwapChain::chooseSwapPresentMode()
    {
        for (const auto& available_present_mode : device_ref.suitability.present_modes) {
            if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return available_present_mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::chooseSwapExtent(Window& window)
    {
        if (device_ref.suitability.capabilities.currentExtent.width !=
            std::numeric_limits<uint32_t>::max())
        {
            return device_ref.suitability.capabilities.currentExtent;
        }
        else
        {
            int width, height;
            window.getWindowFramebufferSize(width, height);

            VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                    static_cast<uint32_t>(height)};

            actualExtent.width =
                std::clamp(actualExtent.width,
                        device_ref.suitability.capabilities.minImageExtent.width,
                        device_ref.suitability.capabilities.maxImageExtent.width);
            actualExtent.height =
                std::clamp(actualExtent.height,
                        device_ref.suitability.capabilities.minImageExtent.height,
                        device_ref.suitability.capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void SwapChain::createSwapChain(Window& window, const Instance& instance) 
    {
        VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat();
        VkPresentModeKHR present_mode = chooseSwapPresentMode();
        extent = chooseSwapExtent(window);
        uint32_t image_count = device_ref.suitability.capabilities.minImageCount + 1;
        if (device_ref.suitability.capabilities.maxImageCount > 0 && image_count > device_ref.suitability.capabilities.maxImageCount) {
            image_count = device_ref.suitability.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swap_chain_info{};
        swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swap_chain_info.surface = instance.surface;
        swap_chain_info.minImageCount = image_count;
        swap_chain_info.imageFormat = surface_format.format;
        swap_chain_info.imageColorSpace = surface_format.colorSpace;
        swap_chain_info.imageExtent = extent;
        swap_chain_info.imageArrayLayers = 1;
        swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        image_format = surface_format.format;

        uint32_t fam_indexes[] = {device_ref.suitability.queue_fam_indexes["draw"], device_ref.suitability.queue_fam_indexes["present"]};

        if (device_ref.suitability.queue_fam_indexes["draw"] != device_ref.suitability.queue_fam_indexes["present"]) 
        {
            swap_chain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swap_chain_info.queueFamilyIndexCount = 2;
            swap_chain_info.pQueueFamilyIndices = fam_indexes;
        } 
        else 
        {
            swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swap_chain_info.queueFamilyIndexCount = 0;
            swap_chain_info.pQueueFamilyIndices = nullptr;
        }

        swap_chain_info.preTransform = device_ref.suitability.capabilities.currentTransform;
        swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swap_chain_info.presentMode = present_mode;
        swap_chain_info.clipped = VK_TRUE;
        swap_chain_info.oldSwapchain = VK_NULL_HANDLE;

        vkCheck(vkCreateSwapchainKHR(device_ref.logical_handle, &swap_chain_info, nullptr, &swap_chain));

        vkCheck(vkGetSwapchainImagesKHR(device_ref.logical_handle, swap_chain, &image_count, nullptr));
        images.resize(image_count);
        vkCheck(vkGetSwapchainImagesKHR(device_ref.logical_handle, swap_chain, &image_count, images.data()));

    }

    void SwapChain::createImageViews()
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

            vkCheck(vkCreateImageView(device_ref.logical_handle, &image_view_info, nullptr, &image_views[i]));
        }
    }

    
    void SwapChain::createFrameBuffers()
    {
        
    }
}
