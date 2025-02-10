#define NOMINMAX
#include <Windows.h>

#include "resource_manager.h"
#include "device.h"
#include "mesh.h"
#include "vulkan_operations.h"
#include "material.h"

#include <stdexcept>
#define GLM_ENABLE_EXPERIMENTAL
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <glm/gtx/quaternion.hpp>

// What fucking insanity is this, why the hell does this need to be included after all the other shit
// Is it the inclusion of 


namespace Vulkan {

    // ResourceManager::ResourceManager() {}

    // ResourceManager::~ResourceManager() {
    //     // Resources cleaned up automatically through unique_ptr
    // }

    // bool ResourceManager::loadAsset(Device& device, const std::string& filepath,
    //                                 VmaAllocator allocator_handle,
    //                                 VkCommandPool pool_handle) {

    //     std::filesystem::path f_path(filepath);
    //     std::cout << "Loading GLTF: " << f_path << std::endl;
    //     fastgltf::GltfDataBuffer data;
    //     auto dltfile = data.FromPath(f_path);
    //     std::cout << static_cast<uint64_t>(dltfile.error()) << std::endl;

    //     constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers |
    //                                  fastgltf::Options::LoadExternalBuffers;

    //     fastgltf::Asset gltf;
    //     fastgltf::Parser parser{};

    //     std::cout << f_path.parent_path() << std::endl;

    //     auto load = parser.loadGltfBinary(dltfile.get(), f_path.parent_path(),
    //                                       gltfOptions);
    //     if (load) {
    //         gltf = std::move(load.get());
    //     } else {
    //         std::cout << "Failed to load glTF: "
    //                   << fastgltf::to_underlying(load.error()) << std::endl;
    //         return false;
    //     }

    //     uint32_t mesh_index = 0;
    //     for (fastgltf::Mesh& mesh : gltf.meshes) {
    //         Mesh sub_mesh;
    //         sub_mesh.name = mesh.name;
    //         std::cout << sub_mesh.name << std::endl;

    //         for (auto&& p : mesh.primitives) {
    //             SubPrimitive new_sub_primitive;
    //             ;
    //             new_sub_primitive.start_index =
    //                 (uint32_t)sub_mesh.indices.size();
    //             new_sub_primitive.count =
    //                 (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

    //             size_t initial_vtx = sub_mesh.vertices.size();

    //             // load indexes
    //             {
    //                 fastgltf::Accessor& indexaccessor =
    //                     gltf.accessors[p.indicesAccessor.value()];
    //                 sub_mesh.indices.reserve(sub_mesh.indices.size() +
    //                                          indexaccessor.count);

    //                 fastgltf::iterateAccessor<std::uint32_t>(
    //                     gltf, indexaccessor, [&](std::uint32_t idx) {
    //                         sub_mesh.indices.push_back(idx + initial_vtx);
    //                     });
    //             }

    //             // load vertex positions
    //             {
    //                 fastgltf::Accessor& posAccessor =
    //                     gltf.accessors[p.findAttribute("POSITION")
    //                                        ->accessorIndex];
    //                 sub_mesh.vertices.resize(sub_mesh.vertices.size() +
    //                                          posAccessor.count);

    //                 fastgltf::iterateAccessorWithIndex<glm::vec3>(
    //                     gltf, posAccessor, [&](glm::vec3 v, size_t index) {
    //                         Vertex newvtx;
    //                         newvtx.position = v;
    //                         newvtx.normal = {1, 0, 0};
    //                         newvtx.color = glm::vec4{1.f};
    //                         newvtx.uv_x = 0;
    //                         newvtx.uv_y = 0;
    //                         sub_mesh.vertices[initial_vtx + index] = newvtx;
    //                     });
    //             }

    //             // load vertex normals
    //             auto normals = p.findAttribute("NORMAL");
    //             if (normals != p.attributes.end()) {

    //                 fastgltf::iterateAccessorWithIndex<glm::vec3>(
    //                     gltf, gltf.accessors[(*normals).accessorIndex],
    //                     [&](glm::vec3 v, size_t index) {
    //                         sub_mesh.vertices[initial_vtx + index].normal = v;
    //                     });
    //             }

    //             // load UVs
    //             auto uv = p.findAttribute("TEXCOORD_0");
    //             if (uv != p.attributes.end()) {

    //                 fastgltf::iterateAccessorWithIndex<glm::vec2>(
    //                     gltf, gltf.accessors[(*uv).accessorIndex],
    //                     [&](glm::vec2 v, size_t index) {
    //                         sub_mesh.vertices[initial_vtx + index].uv_x = v.x;
    //                         sub_mesh.vertices[initial_vtx + index].uv_y = v.y;
    //                     });
    //             }

    //             // load vertex colors
    //             auto colors = p.findAttribute("COLOR_0");
    //             if (colors != p.attributes.end()) {

    //                 fastgltf::iterateAccessorWithIndex<glm::vec4>(
    //                     gltf, gltf.accessors[(*colors).accessorIndex],
    //                     [&](glm::vec4 v, size_t index) {
    //                         sub_mesh.vertices[initial_vtx + index].color = v;
    //                     });
    //             }
    //             sub_mesh.sub_primitives.push_back(new_sub_primitive);
    //         }

    //         // display the vertex normals
    //         constexpr bool OverrideColors = true;
    //         if (OverrideColors) {
    //             for (Vertex& vtx : sub_mesh.vertices) {
    //                 vtx.color = glm::vec4(vtx.normal, 1.f);
    //             }
    //         }
    //         sub_mesh.upload(device, allocator_handle, pool_handle);

    //         meshes.emplace(mesh_index, sub_mesh);
    //         mesh_index++;
    //     }

    //     return true;
    // }

    // void ResourceManager::unloadMesh(MeshHandle handle) {
    //     if (auto it = meshes.find(handle); it != meshes.end()) {
    //         //Will need to deal with any allocations here, or double check the destructor can handle them.
    //         meshes.erase(it);
    //     }
    // }

    // std::optional<Mesh> ResourceManager::getMesh(MeshHandle handle) {
    //     if (auto it = meshes.find(handle); it != meshes.end()) {
    //         return it->second;
    //     }
    //     return std::nullopt;
    // }

    // void ResourceManager::destroy(VmaAllocator allocator_handle) {
    //     for (auto&& m : meshes) {
    //         m.second.destroy(allocator_handle);
    //     }

    //     meshes.clear();
    // }
    // TextureHandle ResourceManager::loadTexture(const std::string& filepath) {
    //     try {
    //         auto texture = std::make_unique<Texture>(device);
    //         if (!TextureLoader::loadFromFile(*texture, filepath)) {
    //             throw std::runtime_error("Failed to load texture: " + filepath);
    //         }

    //         TextureHandle handle = next_texture_handle++;
    //         textures[handle] = std::move(texture);
    //         return handle;
    //     }
    //     catch (const std::exception& e) {
    //         throw std::runtime_error("Texture loading failed: " + std::string(e.what()));
    //     }
    // }

    // void ResourceManager::unloadTexture(TextureHandle handle) {
    //     if (auto it = textures.find(handle); it != textures.end()) {
    //         textures.erase(it);
    //     }
    // }

    // MaterialHandle ResourceManager::createMaterial(const MaterialDesc& desc) {
    //     try {
    //         auto material = std::make_unique<Material>(device, desc);
    //         MaterialHandle handle = next_material_handle++;
    //         materials[handle] = std::move(material);
    //         return handle;
    //     }
    //     catch (const std::exception& e) {
    //         throw std::runtime_error("Material creation failed: " + std::string(e.what()));
    //     }
    // }

    // void ResourceManager::destroyMaterial(MaterialHandle handle) {
    //     if (auto it = materials.find(handle); it != materials.end()) {
    //         materials.erase(it);
    //     }
    // }

    // Texture* ResourceManager::getTexture(TextureHandle handle) {
    //     if (auto it = textures.find(handle); it != textures.end()) {
    //         return it->second.get();
    //     }
    //     return nullptr;
    // }

    // Material* ResourceManager::getMaterial(MaterialHandle handle) {
    //     if (auto it = materials.find(handle); it != materials.end()) {
    //         return it->second.get();
    //     }
    //     return nullptr;
    // }
    namespace ResourceManagement {
        std::vector<std::shared_ptr<MaterialOperation::MeshAsset>> loadAssets(
            Device& device, const std::string& filepath,
            VmaAllocator allocator_handle, VkCommandPool pool_handle) {
            
            std::filesystem::path f_path(filepath);
            std::cout << "Loading GLTF: " << f_path << std::endl;
            fastgltf::GltfDataBuffer data;
            auto dltfile = data.FromPath(f_path);
            std::cout << static_cast<uint64_t>(dltfile.error()) << std::endl;

            constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers |
                                         fastgltf::Options::LoadExternalBuffers;

            fastgltf::Asset gltf;
            fastgltf::Parser parser{};

            std::cout << f_path.parent_path() << std::endl;

            std::vector<std::shared_ptr<MaterialOperation::MeshAsset>> meshes;
            auto load = parser.loadGltfBinary(
                dltfile.get(), f_path.parent_path(), gltfOptions);
            if (load) {
                gltf = std::move(load.get());
            } else {
                std::cout << "Failed to load glTF: "
                          << fastgltf::to_underlying(load.error()) << std::endl;
                return meshes;
            }

            std::vector<uint32_t> indices;
            std::vector<Vertex> vertices;
            uint32_t mesh_index = 0;
            for (fastgltf::Mesh& mesh : gltf.meshes) {
                MaterialOperation::MeshAsset sub_mesh;
                sub_mesh.name = mesh.name;
                std::cout << sub_mesh.name << std::endl;

                indices.clear();
                vertices.clear();
                for (auto&& p : mesh.primitives) {
                    MaterialOperation::GeoSurface new_sub_primitive;
                    ;
                    new_sub_primitive.start_index =
                        (uint32_t)indices.size();
                    new_sub_primitive.count =
                        (uint32_t)gltf.accessors[p.indicesAccessor.value()]
                            .count;

                    size_t initial_vtx = vertices.size();

                    // load indexes
                    {
                        fastgltf::Accessor& indexaccessor =
                            gltf.accessors[p.indicesAccessor.value()];
                        indices.reserve(indices.size() +
                                                 indexaccessor.count);

                        fastgltf::iterateAccessor<std::uint32_t>(
                            gltf, indexaccessor, [&](std::uint32_t idx) {
                                indices.push_back(idx + initial_vtx);
                            });
                    }

                    // load vertex positions
                    {
                        fastgltf::Accessor& posAccessor =
                            gltf.accessors[p.findAttribute("POSITION")
                                               ->accessorIndex];
                        vertices.resize(vertices.size() +
                                                 posAccessor.count);

                        fastgltf::iterateAccessorWithIndex<glm::vec3>(
                            gltf, posAccessor, [&](glm::vec3 v, size_t index) {
                                Vertex newvtx;
                                newvtx.position = v;
                                newvtx.normal = {1, 0, 0};
                                newvtx.color = glm::vec4{1.f};
                                newvtx.uv_x = 0;
                                newvtx.uv_y = 0;
                                vertices[initial_vtx + index] = newvtx;
                            });
                    }

                    // load vertex normals
                    auto normals = p.findAttribute("NORMAL");
                    if (normals != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec3>(
                            gltf, gltf.accessors[(*normals).accessorIndex],
                            [&](glm::vec3 v, size_t index) {
                                vertices[initial_vtx + index].normal =
                                    v;
                            });
                    }

                    // load UVs
                    auto uv = p.findAttribute("TEXCOORD_0");
                    if (uv != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec2>(
                            gltf, gltf.accessors[(*uv).accessorIndex],
                            [&](glm::vec2 v, size_t index) {
                                vertices[initial_vtx + index].uv_x =
                                    v.x;
                                vertices[initial_vtx + index].uv_y =
                                    v.y;
                            });
                    }

                    // load vertex colors
                    auto colors = p.findAttribute("COLOR_0");
                    if (colors != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec4>(
                            gltf, gltf.accessors[(*colors).accessorIndex],
                            [&](glm::vec4 v, size_t index) {
                                vertices[initial_vtx + index].color =
                                    v;
                            });
                    }
                    sub_mesh.surfaces.push_back(new_sub_primitive);
                }

                // display the vertex normals
                constexpr bool OverrideColors = false;
                if (OverrideColors) {
                    for (Vertex& vtx : vertices) {
                        vtx.color = glm::vec4(vtx.normal, 1.f);
                    }
                }
                // sub_mesh.upload(device, allocator_handle, pool_handle);

                sub_mesh.mesh_buffers = MeshOperations::uploadMeshData(device, allocator_handle, pool_handle, indices, vertices);

                // meshes.emplace(mesh_index, sub_mesh);
                meshes.emplace_back(std::make_shared<MaterialOperation::MeshAsset>(std::move(sub_mesh)));
            }

            return meshes;
        }
    }  // namespace ResourceManagement
}  // namespace Vulkan