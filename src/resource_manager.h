#pragma once

#include "vulkan_common.h"


#include <unordered_map>
#include <optional>

namespace fastgltf {
    enum class Filter : std::uint16_t;
    class Asset;
    class Image;
}

namespace Vulkan 
{
    using MeshHandle = uint64_t;
    using TextureHandle = uint64_t;
    using MaterialHandle = uint64_t;

    struct MaterialDesc {
        std::string shader_path;
        std::vector<std::string> texture_paths;
        std::unordered_map<std::string, float> parameters;
    };

    struct Mesh;
    struct Device;
    namespace MaterialOperation {
        struct MeshAsset;
        struct LoadedGLTF;
        struct MaterialResources;
        struct GLTFOperations;
    }

    namespace Pipeline {
        struct Cache;
    }
    


    namespace ResourceManagement {
        std::vector<std::shared_ptr<MaterialOperation::MeshAsset>> loadAssets(Device& device, const std::string& filepath, VmaAllocator allocator_handle, VkCommandPool pool_handle);
        std::optional<std::shared_ptr<MaterialOperation::LoadedGLTF>> loadGLTF(
            Device& device, const std::string& filepath, VmaAllocator& allocator_handle, VkCommandPool& pool_handle,
            AllocatedTexture& default_texture, MaterialOperation::MaterialResources& default_resources, MaterialOperation::GLTFOperations& material_operations, Pipeline::Cache& pipeline_cache);
            std::optional<AllocatedTexture> loadImage(Device& device, VmaAllocator& allocator_handle, VkCommandPool& pool_handle, fastgltf::Asset& asset, fastgltf::Image& image);
        VkFilter extractFilter(fastgltf::Filter filter);
        VkSamplerMipmapMode extractMipmapMode(fastgltf::Filter filter);
    }
}
