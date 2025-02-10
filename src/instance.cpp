#include "instance.h"
#include "vulkan_utils.h"
#include "window.h"

#include <cstddef>
#include <memory>

namespace Vulkan {

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    Instance::Instance()
    {

    }

    Instance::~Instance()
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

    Instance::Instance(Instance&& other) noexcept
        : handle(other.handle), debug_messenger(other.debug_messenger) {
        other.handle = VK_NULL_HANDLE;
        other.debug_messenger = VK_NULL_HANDLE;
    }

    Instance& Instance::operator=(Instance&& other) noexcept {
        if (this != &other) {
            destroy();
            other.handle = nullptr;
        }
        return *this;
    }

    void Instance::create()
    {
        // Validation layer check, TODO wrap in  a debug thing
        uint32_t layers;
        vkEnumerateInstanceLayerProperties(&layers, nullptr);

        std::vector<VkLayerProperties> layers_found(layers);
        vkEnumerateInstanceLayerProperties(&layers, layers_found.data());
        bool found = true && layers;
        for (const char* name : validation_layers) {
            for (const auto& props : layers_found) {
                std::cout << name << " " << props.layerName << std::endl;
                if (strcmp(name, props.layerName) == 0) {
                    std::cout << "found layer" << std::endl;
                    found = found & true;
                    break;
                }
                else {
                    found = found | false;
                }
            }
        }

        if (!found)
        {
            std::cout << "No validation layers found" << std::endl;
        }

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);


        VkApplicationInfo application_information{};
        application_information.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        application_information.pApplicationName = "vulkan-renderer";
        application_information.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        application_information.pEngineName = "Not really an engine";
        application_information.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        application_information.apiVersion = VK_API_VERSION_1_2;

        VkDebugUtilsMessengerCreateInfoEXT instance_debug_info{};
        instance_debug_info.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        instance_debug_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        instance_debug_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        instance_debug_info.pfnUserCallback = debug_callback;

        VkInstanceCreateInfo create_information{};
        create_information.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_information.pApplicationInfo = &application_information;

        if (found) {
            create_information.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_information.ppEnabledLayerNames = validation_layers.data();
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            create_information.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&instance_debug_info);
        } else {
            create_information.enabledLayerCount = 0;
            create_information.pNext = nullptr;
        }

        std::cout << "Requesting extensions:\n";
        for (const char* extension : extensions) {
            std::cout << "\t" << extension << "\n";
        }

        create_information.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_information.ppEnabledExtensionNames = extensions.data();
        create_information.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

        std::cout << "Available Vulkan extensions:\n";
        for (const auto& extension : availableExtensions) {
            std::cout << "\t" << extension.extensionName << " (version "
                        << extension.specVersion << ")\n";
        }

        vkCheck(vkCreateInstance(&create_information, nullptr, &handle));
        std::cout << "Created vulkan instance" << std::endl;

        VkDebugUtilsMessengerCreateInfoEXT messenger_info{};
        messenger_info.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messenger_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messenger_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messenger_info.pfnUserCallback = debug_callback;
        messenger_info.pUserData = nullptr;
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            handle, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
          func(handle, &messenger_info, nullptr, &debug_messenger);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void Instance::destroy()
    {
        vkDestroySurfaceKHR(handle, surface, nullptr);
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            handle, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(handle, debug_messenger, nullptr);
        }
        vkDestroyInstance(handle, nullptr);
        std::cout << "Destroyed vulkan instance" << std::endl;
    }

    void Instance::createSurface(const Window& window)
    {
        vkCheck(glfwCreateWindowSurface(handle, window.handle, nullptr, &surface));
    }

}
