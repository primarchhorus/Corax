#include "device.h"
#include "vulkan_common.h"
#include "instance.h"
#include "vulkan_utils.h"
#include "window.h"

#include <_types/_uint32_t.h>
#include <exception>
#include <set>

namespace Vulkan {
    Device::Device()
    {

    }

    Device::Device(Device&& other) noexcept
        : physical_handle(other.physical_handle) {
        other.physical_handle = nullptr;
    }

    Device& Device::operator=(Device&& other) noexcept {
        if (this != &other) {
            destroy();
            other.physical_handle = nullptr;
        }
        return *this;
    }

    Device::~Device()
    {
    }

    void Device::init(const Instance& instance, const Window& window)
    {
        initPhysical(instance, window);
        initLogical(instance);
        initQueues();
    }

    void Device::initLogical(const Instance& instance)
    {
        std::vector<VkDeviceQueueCreateInfo> device_queue_information;
        device_queue_information.reserve(suitability.queue_fam_indexes.size());

        std::map<std::string, uint32_t>::iterator it;
        for (it = suitability.queue_fam_indexes.begin(); it != suitability.queue_fam_indexes.end(); it++)
        {
            VkDeviceQueueCreateInfo queue_information{};
            queue_information.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_information.queueFamilyIndex = it->second;
            queue_information.queueCount = 1;
            float priority = 1.0f;
            queue_information.pQueuePriorities = &priority;
            device_queue_information.push_back(queue_information);
        }


        VkPhysicalDeviceFeatures features{};

        VkDeviceCreateInfo device_information{};
        device_information.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_information.pQueueCreateInfos = device_queue_information.data();
        device_information.queueCreateInfoCount = static_cast<uint32_t>(device_queue_information.size());;

        if (instance.enable_validation) {
            device_information.enabledLayerCount = static_cast<uint32_t>(instance.validation_layers.size());
            device_information.ppEnabledLayerNames = instance.validation_layers.data();
        } else {
            device_information.enabledLayerCount = 0;
        }
        device_information.pEnabledFeatures = &features;
        device_information.enabledExtensionCount = static_cast<uint32_t>(required_device_extensions.size());
        device_information.ppEnabledExtensionNames = required_device_extensions.data();

        vkCheck(vkCreateDevice(physical_handle, &device_information, nullptr, &logical_handle));
        std::cout << "Created vulkan logical device" << std::endl;
    }

    void Device::initPhysical(const Instance& instance, const Window& window)
    {
        uint32_t num_device_found = 0;
        vkEnumeratePhysicalDevices(instance.handle, &num_device_found, nullptr);
        if (num_device_found == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::vector<VkPhysicalDevice> devices(num_device_found);
        vkEnumeratePhysicalDevices(instance.handle, &num_device_found, devices.data());
        for (const auto& device : devices) {
            checkDeviceSuitability(device, suitability, window);
            checkDeviceExtensions(device);
            querySwapChainSupport(device, window);
            if (suitability.result()) {
                physical_handle = device;
                std::cout << "Device chosen" << std::endl;
                break;
            }
        }

        if (physical_handle == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void Device::checkDeviceSuitability(const VkPhysicalDevice& device, DeviceSuitability& suitability, const Window& window)
    {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);

        suitability.properties_suitable =
            suitability.properties_suitable |
            (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) |
            (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);

        uint32_t num_queue_families{0};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, nullptr);

        std::vector<VkQueueFamilyProperties> queue_fams(num_queue_families);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, queue_fams.data());

        int i = 0;
        for (const auto& fam : queue_fams) {
            suitability.queue_fam_draw_suitable = suitability.queue_fam_draw_suitable | (fam.queueFlags & VK_QUEUE_GRAPHICS_BIT);
            if (suitability.queue_fam_draw_suitable) {
                suitability.queue_fam_indexes["draw"] = i;
                suitability.queue_fam_total++;
            }
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, window.surface, &suitability.queue_fam_present_suitable);
            if (suitability.queue_fam_present_suitable)
            {
                suitability.queue_fam_indexes["present"] = i;
                suitability.queue_fam_total++;
            }
            i++;
        }
    }

    void Device::checkDeviceExtensions(const VkPhysicalDevice& device)
    {
        uint32_t num_extensions{0};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);

        std::vector<VkExtensionProperties> extensions_found(num_extensions);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, extensions_found.data());

        std::set<std::string> required_extensions(required_device_extensions.begin(), required_device_extensions.end());
        uint32_t found_required{0};
        for (const auto& extension : extensions_found) {
            auto it = required_extensions.find(extension.extensionName);
            if (it != required_extensions.end())
            {
                found_required++;
            }
        }

        suitability.extension_suitable = (required_extensions.size() == found_required);
    }

    void Device::querySwapChainSupport(const VkPhysicalDevice& device, const Window& window)
    {

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, window.surface, &suitability.capabilities);
        uint32_t num_formats{0};
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, window.surface, &num_formats, nullptr);

        if (num_formats != 0) {
            suitability.formats.resize(num_formats);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, window.surface, &num_formats, suitability.formats.data());
        }

        uint32_t num_present_modes{0};
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, window.surface, &num_present_modes, nullptr);

        if (num_present_modes != 0) {
            suitability.present_modes.resize(num_present_modes);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, window.surface, &num_present_modes, suitability.present_modes.data());
        }
    }

    void Device::initQueues()
    {
        vkGetDeviceQueue(logical_handle, suitability.queue_fam_indexes["draw"], 0, &graphics_queue);
        vkGetDeviceQueue(logical_handle, suitability.queue_fam_indexes["present"], 0, &present_queue);
        std::cout << "Initialized queues" << std::endl;
    }

    void Device::destroy()
    {
        vkDestroyDevice(logical_handle, nullptr);
        std::cout << "Destroyed vulkan logical device" << std::endl;
    }
}
