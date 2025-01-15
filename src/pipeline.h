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
    struct ShaderModule {
        VkShaderModule module;
        VkShaderStageFlagBits stage;
        std::vector<uint32_t> spirv_binary;

        ShaderModule() {
 
        }

        ~ShaderModule()
        {
        }

        bool load(const std::string& filename) {
            std::error_code error;
            std::filesystem::path path(filename);
            std::cout << path << std::endl;
            size_t file_size = std::filesystem::file_size(path, error);
            if (error)
            {
                return false;
            }
            const uint32_t chunk_size = 4096;
        #ifdef _WIN32
            HANDLE file = CreateFile(filename.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE) {
                return false;
            }

            std::vector<char> buffer(chunk_size);
            DWORD bytes_read;
            while (ReadFile(file, buffer.data(), chunk_size, &bytes_read, nullptr) && bytes_read > 0) {
                spirv_binary.insert(spirv_binary.end(),
                    reinterpret_cast<const uint32_t*>(buffer.data()),
                    reinterpret_cast<const uint32_t*>(buffer.data() + bytes_read));
            }

            CloseHandle(file);
        #else
            int fd = open(filename.c_str(), O_RDONLY);
            if (fd == -1) {
                return false;
            }

            std::vector<char> buffer(chunk_size);
            ssize_t bytes_read;
            while ((bytes_read = read(fd, buffer.data(), chunk_size)) > 0) {
                spirv_binary.insert(spirv_binary.end(),
                    reinterpret_cast<const uint32_t*>(buffer.data()),
                    reinterpret_cast<const uint32_t*>(buffer.data() + bytes_read));
            }

            close(fd);
        #endif

            return true;
        }

        void create(Device& device) {
            VkShaderModuleCreateInfo shader_info{};
            shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shader_info.codeSize = spirv_binary.size() * sizeof(uint32_t);
            shader_info.pCode = spirv_binary.data();

            vkCheck(vkCreateShaderModule(device.logical_handle, &shader_info, nullptr, &module));
        }

        void destroy(Device& device) {
            vkDestroyShaderModule(device.logical_handle, module, nullptr);
        }
    };

    struct PipeLineConfig {
        // Figure out the size order of these things at somepoint so i can pack the struct
        VkPipelineShaderStageCreateInfo vertex_stages;
        VkPipelineShaderStageCreateInfo fragment_stages;
        std::string name;
        VkExtent2D extent;
        VkFormat format;
    };

    struct PipeLine {
        PipeLine(Device& device, const PipeLineConfig& config);
        ~PipeLine();
        PipeLine(const PipeLine&) = delete;
        PipeLine& operator=(const PipeLine&) = delete;
        PipeLine(PipeLine&& other) noexcept;
        PipeLine& operator=(PipeLine&& other) noexcept;

        void destroy();
        void create(const PipeLineConfig& config);
        
        VkPipeline handle;
        VkPipelineLayout layout_handle;
        Device& device_ref;
    };

} // namespace Vulkan