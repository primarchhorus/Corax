#include "device.h"
#include "swap_chain.h"
#include "vulkan_common.h"
#include "window.h"
#include "instance.h"
#include "pipeline.h"

class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    void initVulkan() {
        instance.init();
        glfw_window.init(instance);
        device.init(instance, glfw_window);
        swap_chain.init(device, glfw_window);
    }

    void createPipelineObjects() {
        Vulkan::ShaderModule vertex_module("shaders/shader.vert");
        Vulkan::ShaderModule frag_module("shaders/shader.frag");
        vertex_module.load();
        frag_module.load();
        vertex_module.create(device);
        frag_module.create(device);

        Vulkan::PipeLineConfig config{};
        VkPipelineShaderStageCreateInfo vertex_info{};
        vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_info.module = vertex_module.module;
        vertex_info.pName = "main";
        config.shader_stages.push_back(vertex_info);

        VkPipelineShaderStageCreateInfo frag_info{};
        frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_info.module = frag_module.module;
        frag_info.pName = "main";
        config.shader_stages.push_back(frag_info);


        Vulkan::PipeLine pipeline(config, device);
    }

    void mainLoop() {
        while (!glfw_window.closeCheck()) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        swap_chain.destroy(device);
        device.destroy();
        glfw_window.destroy(instance);
        instance.destroy();

    }

    Vulkan::Window glfw_window{800, 600, "vulkan"};
    Vulkan::Instance instance;
    Vulkan::SwapChain swap_chain;
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
