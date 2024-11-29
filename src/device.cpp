#include "device.h"
#include "vulkan_utils.h"

namespace Vulkan {
    Device::Device()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        std::cout << "Requesting extensions:\n";
        for (const char* extension : extensions) {
            std::cout << "\t" << extension << "\n";
        }

        VkApplicationInfo application_information{};
        application_information.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        application_information.pApplicationName = "vulkan-renderer";
        application_information.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        application_information.pEngineName = "Not really an engine";
        application_information.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        application_information.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo create_information{};
        create_information.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_information.pApplicationInfo = &application_information;
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
        vkCheck(vkCreateInstance(&create_information, nullptr, &instance));
    }
}
