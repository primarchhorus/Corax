#pragma once

#ifdef _WIN32
    #undef APIENTRY //Some total nonsense, already defined in glfw header
#endif

#include "vulkan_common.h"
#include "vulkan_utils.h"
#include "device.h"
#include "swap_chain.h"

#include <filesystem>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <functional>
#include <tuple>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <unistd.h>
#endif



namespace Vulkan {

        /*
        Need to sort out a ref to the device for the shader modules
        */
    // struct ShaderModule {
    //     VkShaderModule module;
    //     VkShaderStageFlagBits stage;
    //     std::vector<uint32_t> spirv_binary;

    //     ShaderModule() {
 
    //     }

    //     ~ShaderModule()
    //     {
    //     }

    //     bool load(const std::string& filename) {
    //         std::error_code error;
    //         std::filesystem::path path(filename);
    //         std::cout << path << std::endl;
    //         bool exists = std::filesystem::exists(path, error);
    //         if (error || !exists)
    //         {
    //             return false;
    //         }
    //         const uint32_t chunk_size = 4096;
    //     #ifdef _WIN32
    //         HANDLE file = CreateFile(filename.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    //         if (file == INVALID_HANDLE_VALUE) {
    //             return false;
    //         }

    //         std::vector<char> buffer(chunk_size);
    //         DWORD bytes_read;
    //         while (ReadFile(file, buffer.data(), chunk_size, &bytes_read, nullptr) && bytes_read > 0) {
    //             spirv_binary.insert(spirv_binary.end(),
    //                 reinterpret_cast<const uint32_t*>(buffer.data()),
    //                 reinterpret_cast<const uint32_t*>(buffer.data() + bytes_read));
    //         }

    //         CloseHandle(file);
    //     #else
    //         int fd = open(filename.c_str(), O_RDONLY);
    //         if (fd == -1) {
    //             return false;
    //         }

    //         std::vector<char> buffer(chunk_size);
    //         ssize_t bytes_read;
    //         while ((bytes_read = read(fd, buffer.data(), chunk_size)) > 0) {
    //             spirv_binary.insert(spirv_binary.end(),
    //                 reinterpret_cast<const uint32_t*>(buffer.data()),
    //                 reinterpret_cast<const uint32_t*>(buffer.data() + bytes_read));
    //         }

    //         close(fd);
    //     #endif

    //         return true;
    //     }

    //     void create(const Device& device) {
    //         VkShaderModuleCreateInfo shader_info{};
    //         shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    //         shader_info.codeSize = spirv_binary.size() * sizeof(uint32_t);
    //         shader_info.pCode = spirv_binary.data();

    //         vkCheck(vkCreateShaderModule(device.logical_handle, &shader_info, nullptr, &module));
    //     }

    //     void destroy(const Device& device) {
    //         if (module)
    //         {
    //             vkDestroyShaderModule(device.logical_handle, module, nullptr);
    //         }
    //     }
    // };

    // struct PipeLineConfig {
    //     // Figure out the size order of these things at somepoint so i can pack the struct
    //     VkPipelineShaderStageCreateInfo vertex_stages;
    //     VkPipelineShaderStageCreateInfo fragment_stages;
    //     std::string_view name;
    //     VkExtent2D extent;
    //     VkFormat format;
    //     VkDescriptorSetLayout descriptor_set_layout;
    // };

    // struct PipeLineObject {
    //     PipeLineObject();
    //     ~PipeLineObject();
    //     PipeLineObject(const PipeLineObject&) = delete;
    //     PipeLineObject& operator=(const PipeLineObject&) = delete;
    //     PipeLineObject(PipeLineObject&& other) noexcept;
    //     PipeLineObject& operator=(PipeLineObject&& other) noexcept;

    //     void destroy(const Device& device);
    //     void create(const Device& device, const PipeLineConfig& config);
        
    //     VkPipeline handle;
    //     VkPipelineLayout layout_handle;
    //     VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    //     VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    //     VkPipelineDynamicStateCreateInfo dynamic_states_info{};
    //     VkPipelineViewportStateCreateInfo viewport_state{};
    //     VkPipelineRasterizationStateCreateInfo rasterizer{};
    //     VkPipelineMultisampleStateCreateInfo multisampling{};
    //     VkPipelineColorBlendAttachmentState color_blend_attachment{};
    //     VkPipelineColorBlendStateCreateInfo color_blending{};
    //     VkPushConstantRange buffer_range{};
    //     VkPipelineLayoutCreateInfo pipeline_layout_info{};
    //     VkPipelineRenderingCreateInfo render_info{};
    //     VkGraphicsPipelineCreateInfo pipeline_info{};
    //     VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
    // };

    namespace Pipeline {
        // Data
        struct Object {
            VkPipeline handle;
            VkPipelineLayout layout_handle;
            VkPipelineVertexInputStateCreateInfo vertex_input_info{};
            VkPipelineInputAssemblyStateCreateInfo input_assembly{};
            VkPipelineDynamicStateCreateInfo dynamic_states_info{};
            VkPipelineViewportStateCreateInfo viewport_state{};
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            VkPipelineMultisampleStateCreateInfo multisampling{};
            VkPipelineColorBlendAttachmentState color_blend_attachment{};
            VkPipelineColorBlendStateCreateInfo color_blending{};
            VkPushConstantRange buffer_range{};
            VkPipelineLayoutCreateInfo pipeline_layout_info{};
            VkPipelineRenderingCreateInfo render_info{};
            VkGraphicsPipelineCreateInfo pipeline_info{};
            VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
        };

        struct Configuration {
            VkPipelineShaderStageCreateInfo vertex_stages{};
            VkPipelineShaderStageCreateInfo fragment_stages{};
            std::string_view name{""};
            VkExtent2D extent{};
            VkFormat format{};
            VkDescriptorSetLayout* descriptor_set_layout;
            uint32_t num_descriptor_sets{0};
            VkPolygonMode polygon_mode{};
            VkCullModeFlags cull_mode_flags{};
            VkFrontFace front_face{};
            VkBool32 enable_blend{VK_TRUE};
            VkBool32 enable_depth{VK_TRUE};
            VkCompareOp depth_compare{};
        };

        struct Cache {
            // Update to use the config as the hash when my life force can deal with all the overloading of the == operator and hash specialization into the dark of the void of whats required.
            // Maybe just change to a uint handle? why use the config, i dont quite see the benefit, at least for my case at the moment.
            std::unordered_map<std::string_view, std::unique_ptr<Object>> object_map;
        };

        struct Shader {
            VkShaderModule module;
            VkShaderStageFlagBits stage{};
            std::vector<uint32_t> spirv_binary;
            std::string filename{};
        };

        // Operators

        void destroyPipelineObject(const Device& device, const std::unique_ptr<Object>& pipeline);
        std::unique_ptr<Object> createPipelineObject(const Device& device, const Configuration& config);
        void addPipelineToCache(const Configuration& config, Cache& cache, std::unique_ptr<Object> pipeline);
        Object* getPipelineFromCache(const Configuration& config, Cache& cache);
        void clearCache(const Device& device, Cache& cache);
        bool loadShader(Shader& shader);
        void createShaderModule(const Device& device, Shader& shader);
        void destroyShaderModule(const Device& device, Shader& shader);
    }

} // namespace Vulkan