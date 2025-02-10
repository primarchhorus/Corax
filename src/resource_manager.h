#pragma once

#include "vulkan_common.h"

#include <unordered_map>
#include <optional>

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
    }
    

    // struct ResourceManager {
    //     ResourceManager();
    //     ~ResourceManager();

    //     // Mesh Management
    //     bool loadAsset(Device& device, const std::string& filepath, VmaAllocator allocator_handle, VkCommandPool pool_handle);
    //     void unloadMesh(MeshHandle handle);
    //     std::optional<Mesh> getMesh(MeshHandle handle);

    //     void destroy(VmaAllocator allocator_handle);
        
    //     // // Texture Management  
    //     // TextureHandle loadTexture(const std::string& filepath);
    //     // void unloadTexture(TextureHandle handle);

    //     // // Material Management
    //     // MaterialHandle createMaterial(const MaterialDesc& desc);
    //     // void destroyMaterial(MaterialHandle handle);

    //     std::unordered_map<MeshHandle, Mesh> meshes;
    //     // std::unordered_map<TextureHandle, std::unique_ptr<Texture>> textures;
    //     // std::unordered_map<MaterialHandle, std::unique_ptr<Material>> materials;
    // };

    namespace ResourceManagement {
        std::vector<std::shared_ptr<MaterialOperation::MeshAsset>> loadAssets(Device& device, const std::string& filepath, VmaAllocator allocator_handle, VkCommandPool pool_handle);
    }
}
