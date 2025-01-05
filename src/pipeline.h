#pragma once

#include "vulkan_common.h"
#include "vulkan_utils.h"
#include "device.h"

#include <filesystem>
#include <vector>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>

namespace Vulkan {

    class SwapChain;

    struct ShaderModule {
        VkShaderModule module;
        VkShaderStageFlagBits stage;
        std::vector<uint32_t> spirv_binary;
        std::string shader_name;

        ShaderModule(const std::string& filename) : shader_name(filename) {}

        bool load() {
            std::error_code error;
            std::filesystem::path path(shader_name);
            size_t file_size = std::filesystem::file_size(path, error);
            if (error)
            {
                return false;
            }

            #ifdef _WIN32
                // Fill in win nonsense later
                return false;
            #else
                int fd = open(shader_name.c_str(), O_RDONLY);
                if (fd == -1)
                {
                    return false;
                }

                void *data =
                    mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);

                if (data == MAP_FAILED)
                {
                    close(fd);
                    return false;
                }

                spirv_binary.assign(
                    reinterpret_cast<const uint32_t*>(data),
                    reinterpret_cast<const uint32_t*>(static_cast<const char*>(data) + file_size)
                );

                munmap(data, file_size);
                close(fd);
            #endif

            return true;
        }

        void create(const Device& device) {
            VkShaderModuleCreateInfo shader_info{};
            shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shader_info.codeSize = spirv_binary.size() * sizeof(uint32_t);
            shader_info.pCode = spirv_binary.data();

            vkCheck(vkCreateShaderModule(device.logical_handle, &shader_info, nullptr, &module));
        }

        void destroy(const Device& device) {
            vkDestroyShaderModule(device.logical_handle, module, nullptr);
        }
    };

    struct PipeLineConfig {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        // VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
        // VkPipelineViewportStateCreateInfo viewport_info;
        // VkPipelineRasterizationStateCreateInfo rasterizer_info;
        // VkPipelineMultisampleStateCreateInfo multisampling_info;
        // VkPipelineColorBlendAttachmentState color_blend_attachment;
        // VkPipelineColorBlendStateCreateInfo color_blending_info;
        // VkPipelineLayout pipeline_layout;
        // VkRenderPass render_pass;
        // VkExtent2D extent;
    };

    struct PipeLine {
        PipeLine();
        ~PipeLine();
        PipeLine(const PipeLine&) = delete;
        PipeLine& operator=(const PipeLine&) = delete;
        PipeLine(PipeLine&& other) noexcept;
        PipeLine& operator=(PipeLine&& other) noexcept;

        void create(const PipeLineConfig& config, const Device& device, const SwapChain& swap_chain);
        void destroy(const Device& device);

        VkPipeline handle;
        VkPipelineLayout layout_handle;
    };
}
