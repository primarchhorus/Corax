#include "vulkan_common.h"

#include "window.h"
#include "device.h"

class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    void initVulkan() {

    }

    void mainLoop() {
        while (!glfw_window.closeCheck()) {
            glfwPollEvents();
        }
    }

    void cleanup() {
    }

    Vulkan::Window glfw_window{800, 600, "vulkan"};
    Vulkan::Device device;
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
