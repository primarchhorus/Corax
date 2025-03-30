#define NOMINMAX
#include <Windows.h>

#include "device.h"
#include "material.h"
#include "mesh.h"
#include "resource_manager.h"
#include "vulkan_operations.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdexcept>
#define GLM_ENABLE_EXPERIMENTAL
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <glm/gtx/quaternion.hpp>


#include <ktx.h>
#include <ktxvulkan.h>

namespace Vulkan {

    namespace ResourceManagement {
        std::vector<std::shared_ptr<MaterialOperation::MeshAsset>> loadAssets(Device& device,
                                                                              const std::string& filepath,
                                                                              VmaAllocator allocator_handle,
                                                                              VkCommandPool pool_handle) {

            std::filesystem::path f_path(filepath);
            std::cout << "Loading GLTF: " << f_path << std::endl;
            fastgltf::GltfDataBuffer data;
            auto dltfile = data.FromPath(f_path);
            std::cout << static_cast<uint64_t>(dltfile.error()) << std::endl;

            constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

            fastgltf::Asset gltf;
            fastgltf::Parser parser{};

            std::cout << f_path.parent_path() << std::endl;

            std::vector<std::shared_ptr<MaterialOperation::MeshAsset>> meshes;
            auto load = parser.loadGltfBinary(dltfile.get(), f_path.parent_path(), gltfOptions);
            if (load) {
                gltf = std::move(load.get());
            } else {
                std::cout << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
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
                    new_sub_primitive.start_index = (uint32_t)indices.size();
                    new_sub_primitive.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

                    size_t initial_vtx = vertices.size();

                    // load indexes
                    {
                        fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                        indices.reserve(indices.size() + indexaccessor.count);

                        fastgltf::iterateAccessor<std::uint32_t>(
                            gltf, indexaccessor, [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
                    }

                    // load vertex positions
                    {
                        fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
                        vertices.resize(vertices.size() + posAccessor.count);

                        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                                                                      [&](glm::vec3 v, size_t index) {
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
                            [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].normal = v; });
                    }

  

                    // load UVs
                    auto uv = p.findAttribute("TEXCOORD_0");
                    if (uv != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex],
                                                                      [&](glm::vec2 v, size_t index) {
                                                                          vertices[initial_vtx + index].uv_x = v.x;
                                                                          vertices[initial_vtx + index].uv_y = v.y;
                                                                      });
                    }

                    // load vertex colors
                    auto colors = p.findAttribute("COLOR_0");
                    if (colors != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec4>(
                            gltf, gltf.accessors[(*colors).accessorIndex],
                            [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].color = v; });
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

                sub_mesh.mesh_buffers =
                    MeshOperations::uploadMeshData(device, allocator_handle, pool_handle, indices, vertices);

                // meshes.emplace(mesh_index, sub_mesh);
                meshes.emplace_back(std::make_shared<MaterialOperation::MeshAsset>(std::move(sub_mesh)));
            }

            return meshes;
        }

        std::optional<std::shared_ptr<MaterialOperation::LoadedGLTF>> loadGLTF(
            Device& device, const std::string& filepath, VmaAllocator& allocator_handle, VkCommandPool& pool_handle,
            AllocatedTexture& default_texture, MaterialOperation::MaterialResources& default_resources,
            MaterialOperation::GLTFOperations& material_operations, Pipeline::Cache& pipeline_cache) {
            std::cout << "Loading GLTF: " << filepath << std::endl;

            std::shared_ptr<MaterialOperation::LoadedGLTF> scene = std::make_shared<MaterialOperation::LoadedGLTF>();
            MaterialOperation::LoadedGLTF& file = *scene.get();

            auto cleanup = [&device, &allocator_handle, &pool_handle, scene, default_texture]() {
                Descriptors::destroyPools(scene->descriptor_pool, device);
                Buffer::destroyBuffer(allocator_handle, scene->material_data_buffer);

                for (auto& v : scene->textures) {

                    if (v.image == default_texture.image) {
                        continue;
                    }
                    Texture::destroy(device, allocator_handle, v);
                }

                for (auto& sampler : scene->samplers) {
                    vkDestroySampler(device.logical_handle, sampler, nullptr);
                }

                // for (auto& v: scene->meshes) {
                //     Buffer::destroyBuffer(allocator_handle, v->mesh_buffers.index_buffer);
                //     Buffer::destroyBuffer(allocator_handle, v->mesh_buffers.vertex_buffer);
                // }
                // Something a bit fishy here, the above should clear all the meshes since its the same pointer
                // the below works but at somepoint figureing out why would be nice

                for (auto& node : scene->nodes) {
                    auto mn = static_cast<MaterialOperation::MeshNode*>(node.get());
                    Buffer::destroyBuffer(allocator_handle, mn->mesh->mesh_buffers.index_buffer);
                    Buffer::destroyBuffer(allocator_handle, mn->mesh->mesh_buffers.vertex_buffer);
                }

                std::cout << "destroying loaded gltf" << std::endl;
            };
            scene->setCleanupFunction(cleanup);

            fastgltf::GltfDataBuffer data;
            auto dltfile = data.FromPath(filepath);
            std::cout << static_cast<uint64_t>(dltfile.error()) << std::endl;
            constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
                                         fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers |
                                         fastgltf::Options::LoadExternalBuffers;

            fastgltf::Asset gltf;
            fastgltf::Parser parser{};

            std::filesystem::path path = filepath;

            auto type = fastgltf::determineGltfFileType(dltfile.get());
            if (type == fastgltf::GltfType::glTF) {
                std::cout << "loadGltf" << std::endl;
                auto load = parser.loadGltf(dltfile.get(), path.parent_path(), gltfOptions);
                if (load) {
                    gltf = std::move(load.get());
                } else {
                    std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
                    return {};
                }
            } else if (type == fastgltf::GltfType::GLB) {
                std::cout << "loadGltfBinary" << std::endl;
                auto load = parser.loadGltfBinary(dltfile.get(), path.parent_path(), gltfOptions);
                if (load) {
                    gltf = std::move(load.get());
                } else {
                    std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
                    return {};
                }
            } else {
                std::cerr << "Failed to determine glTF container" << std::endl;
                return {};
            }

            Descriptors::initPool(file.descriptor_pool, device);

            for (fastgltf::Sampler& sampler : gltf.samplers) {

                VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
                sampl.maxLod = VK_LOD_CLAMP_NONE;
                sampl.minLod = 0;

                sampl.magFilter = VK_FILTER_LINEAR;
                sampl.minFilter = VK_FILTER_LINEAR;

                sampl.mipmapMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

                VkSampler newSampler;
                vkCreateSampler(device.logical_handle, &sampl, nullptr, &newSampler);

                file.samplers.push_back(newSampler);
            }

            for (fastgltf::Image& image : gltf.images) {
                std::optional<AllocatedTexture> img = loadImage(device, allocator_handle, pool_handle, gltf, image);
                std::cout << image.name.c_str() << std::endl;
                if (img.has_value()) {
                    file.textures.push_back(img.value());
                    std::cout << "This should be the proper image " << image.name << std::endl;
                } else {
                    file.textures.push_back(default_texture);
                    std::cout << "gltf failed to load texture " << image.name << std::endl;
                }
            }

            file.material_data_buffer = Buffer::allocateBuffer(
                allocator_handle, sizeof(MaterialOperation::MaterialConstants) * gltf.materials.size(),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            size_t data_index = 0;
            MaterialOperation::MaterialConstants* scene_material_constants =
                static_cast<MaterialOperation::MaterialConstants*>(file.material_data_buffer.info.pMappedData);

            for (fastgltf::Material& mat : gltf.materials) {
                std::shared_ptr<MaterialOperation::MaterialInstance> new_material =
                    std::make_shared<MaterialOperation::MaterialInstance>();
                file.materials.push_back(new_material);

                MaterialOperation::MaterialConstants constants;

                float metallic = mat.pbrData.metallicFactor;
                float roughness = mat.pbrData.roughnessFactor;

                constants.color_factors =
                    glm::vec4(mat.pbrData.baseColorFactor[0], mat.pbrData.baseColorFactor[1],
                                         mat.pbrData.baseColorFactor[2], mat.pbrData.baseColorFactor[3]);  // Avoid full 1.0 values

                constants.metal_factors = mat.pbrData.metallicFactor;
                constants.rough_factors = mat.pbrData.roughnessFactor;
                std::cout << mat.name.c_str() << " constants.metal_factors: " << constants.metal_factors
                          << " constants.rough_factors: " << constants.rough_factors << std::endl;
                constants.ao = 1.0f;

                MaterialOperation::MaterialPass pass_type = MaterialOperation::MaterialPass::MAINCOLOUR;

                if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
                    pass_type = MaterialOperation::MaterialPass::TRANSPARENTCOLOUR;
                    std::cout << "transparent: " << mat.name.c_str() << std::endl;
                }

                MaterialOperation::MaterialResources material_resources;
                material_resources.color_image = default_resources.color_image;
                material_resources.color_sampler = default_resources.color_sampler;
                material_resources.metal_rough_image = default_resources.metal_rough_image;
                material_resources.metal_rough_sampler = default_resources.metal_rough_sampler;

                material_resources.data_buffer = file.material_data_buffer.buffer;
                material_resources.data_buffer_offset = data_index * sizeof(MaterialOperation::MaterialConstants);

                std::cout << "Base Color Texture Index: " << mat.pbrData.baseColorTexture.value().textureIndex << std::endl;
                std::cout << "Metallic-Roughness Texture Index: " << mat.pbrData.metallicRoughnessTexture.value().textureIndex << std::endl;

                if (mat.pbrData.baseColorTexture.has_value()) {
                    size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                    size_t sampler =
                        gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();
                    std::cout << mat.name.c_str() << " " << img << std::endl;
                    material_resources.color_image = file.textures[img];
                    material_resources.color_sampler = file.samplers[sampler];
                }

                if (mat.pbrData.metallicRoughnessTexture.has_value()) {
                    constants.has_metal_rough_texture = 1;
                    size_t img =
                        gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
                        std::cout << "Using metal roughness texture: " << img << std::endl;
                    size_t sampler =
                        gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.value();

                    material_resources.metal_rough_image = file.textures[img];
                    material_resources.metal_rough_sampler = file.samplers[sampler];
                } else {
                    constants.has_metal_rough_texture = 0;
                }

                if (mat.normalTexture.has_value())
                {
                    std::cout << "normal texture found" << std::endl;
                    size_t texIndex = mat.normalTexture.value().textureIndex;
                    size_t imgIndex = gltf.textures[texIndex].imageIndex.value();
                    size_t samplerIndex = gltf.textures[texIndex].samplerIndex.value();
                
                    material_resources.normal_image = file.textures[imgIndex];
                    material_resources.normal_sampler = file.samplers[samplerIndex];
                }

                scene_material_constants[data_index] = constants;
                *new_material = MaterialOperation::writeMaterial(
                    device, pass_type, material_resources, file.descriptor_pool, material_operations, pipeline_cache);

                data_index++;
            }

            std::vector<uint32_t> indices;
            std::vector<Vertex> vertices;

            for (fastgltf::Mesh& mesh : gltf.meshes) {
                std::shared_ptr<MaterialOperation::MeshAsset> newmesh =
                    std::make_shared<MaterialOperation::MeshAsset>();
                file.meshes.push_back(newmesh);
                newmesh->name = mesh.name;

                indices.clear();
                vertices.clear();

                for (auto&& p : mesh.primitives) {
                    MaterialOperation::GeoSurface new_surface;
                    new_surface.start_index = (uint32_t)indices.size();
                    new_surface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

                    size_t initial_vtx = vertices.size();

                    // load indexes
                    {
                        fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                        indices.reserve(indices.size() + indexaccessor.count);

                        fastgltf::iterateAccessor<std::uint32_t>(
                            gltf, indexaccessor, [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
                    }

                    // load vertex positions
                    {
                        fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
                        vertices.resize(vertices.size() + posAccessor.count);

                        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                                                                      [&](glm::vec3 v, size_t index) {
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
                            [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].normal = v; });
                    }

                    // load UVs
                    auto uv = p.findAttribute("TEXCOORD_0");
                    if (uv != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex],
                                                                      [&](glm::vec2 v, size_t index) {
                                                                          vertices[initial_vtx + index].uv_x = v.x;
                                                                          vertices[initial_vtx + index].uv_y = v.y;
                                                                      });
                    }

                    // load vertex colors
                    auto colors = p.findAttribute("COLOR_0");
                    if (colors != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec4>(
                            gltf, gltf.accessors[(*colors).accessorIndex],
                            [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].color = v; });
                    }

                    std::vector<glm::vec3> tangents(vertices.size(), glm::vec3(0.0f));

                    for (size_t i = 0; i < indices.size(); i += 3) {
                        uint32_t i0 = indices[i];
                        uint32_t i1 = indices[i + 1];
                        uint32_t i2 = indices[i + 2];

                        glm::vec3 v0 = vertices[i0].position;
                        glm::vec3 v1 = vertices[i1].position;
                        glm::vec3 v2 = vertices[i2].position;

                        glm::vec2 uv0 = glm::vec2(vertices[i0].uv_x, vertices[i0].uv_y);
                        glm::vec2 uv1 = glm::vec2(vertices[i1].uv_x, vertices[i1].uv_y);
                        glm::vec2 uv2 = glm::vec2(vertices[i2].uv_x, vertices[i2].uv_y);

                        glm::vec3 deltaPos1 = v1 - v0;
                        glm::vec3 deltaPos2 = v2 - v0;
                        glm::vec2 deltaUV1 = uv1 - uv0;
                        glm::vec2 deltaUV2 = uv2 - uv0;

                        float r = (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
                        if (fabs(r) < 1e-6) r = 1.0f; // Avoid division by zero

                        float invDet = 1.0f / r;
                        glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * invDet;

                        // Accumulate for averaging
                        tangents[i0] += tangent;
                        tangents[i1] += tangent;
                        tangents[i2] += tangent;
                    }

                    // Normalize and Orthogonalize Tangents
                    for (size_t i = 0; i < vertices.size(); i++) {
                        glm::vec3 N = glm::normalize(vertices[i].normal);
                        glm::vec3 T = glm::normalize(tangents[i]);

                        // Gram-Schmidt orthogonalization
                        T = glm::normalize(T - N * glm::dot(N, T));

                        vertices[i].tangent = T;

                        if (i % 500 == 0) { // Print every 500th vertex
                            std::cout << "Vertex " << i << " Tangent: " << T.x << ", " << T.y << ", " << T.z << std::endl;
                        }
                    }

                    

                    if (p.materialIndex.has_value()) {
                        new_surface.material = file.materials[p.materialIndex.value()];
                    } else {
                        new_surface.material = file.materials[0];
                    }

                    newmesh->surfaces.push_back(new_surface);
                }

                newmesh->mesh_buffers =
                    MeshOperations::uploadMeshData(device, allocator_handle, pool_handle, indices, vertices);
            }

            for (fastgltf::Node& node : gltf.nodes) {
                std::shared_ptr<MaterialOperation::Node> new_node;

                if (node.meshIndex.has_value()) {
                    new_node = std::make_shared<MaterialOperation::MeshNode>();
                    static_cast<MaterialOperation::MeshNode*>(new_node.get())->mesh = file.meshes[node.meshIndex.value()];
                } else {
                    new_node = std::make_shared<MaterialOperation::Node>();
                }

                file.nodes.push_back(new_node);

                std::visit(fastgltf::visitor{[&](fastgltf::math::fmat4x4 matrix) {
                                                 memcpy(&new_node->localTransform, &matrix, sizeof(matrix));
                                             },
                                             [&](fastgltf::TRS transform) {
                                                 glm::vec3 tl(transform.translation[0], transform.translation[1],
                                                              transform.translation[2]);
                                                 glm::quat rot(transform.rotation[3], transform.rotation[0],
                                                               transform.rotation[1], transform.rotation[2]);
                                                 glm::vec3 sc(transform.scale[0], transform.scale[1],
                                                              transform.scale[2]);

                                                 glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                                 glm::mat4 rm = glm::toMat4(rot);
                                                 glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                                 new_node->localTransform = tm * rm * sm;
                                             }},
                           node.transform);
            }

            // AllocatedTexture irradianceMap = loadKTXTexture(device, pool_handle, "C:/Users/bgarner/Documents/repos/Corax/assets/_ibl.ktx");
            // AllocatedTexture prefilteredEnvMap = loadKTXTexture(device, pool_handle, "C:/Users/bgarner/Documents/repos/Corax/assets/_skybox.ktx");
            // AllocatedTexture brdfLUT = loadKTXTexture(device, pool_handle, "brdfLUT.ktx");
            // std::optional<AllocatedTexture> img = loadImage(device, allocator_handle, pool_handle, gltf, image);

            for (int i = 0; i < gltf.nodes.size(); i++) {
                fastgltf::Node& node = gltf.nodes[i];
                std::shared_ptr<MaterialOperation::Node>& sceneNode = file.nodes[i];
                std::cout << "parent: " << i << " child: " << node.children.size() << std::endl;
                for (auto& c : node.children) {
                    
                    sceneNode->children.push_back(file.nodes[c]);
                    file.nodes[c]->parent = sceneNode;
                }
            }

            // find the top nodes, with no parents
            for (auto& node : file.nodes) {
                if (node->parent.lock() == nullptr) {
                    file.top_nodes.push_back(node);
                    node->refreshTransform(glm::mat4{1.f});
                }
            }
            return scene;
        }

        VkFilter extractFilter(fastgltf::Filter filter) {
            switch (filter) {
                // nearest samplers
                case fastgltf::Filter::Nearest:
                case fastgltf::Filter::NearestMipMapNearest:
                case fastgltf::Filter::NearestMipMapLinear:
                    return VK_FILTER_NEAREST;

                // linear samplers
                case fastgltf::Filter::Linear:
                case fastgltf::Filter::LinearMipMapNearest:
                case fastgltf::Filter::LinearMipMapLinear:
                default:
                    return VK_FILTER_LINEAR;
            }
        }

        VkSamplerMipmapMode extractMipmapMode(fastgltf::Filter filter) {
            switch (filter) {
                case fastgltf::Filter::NearestMipMapNearest:
                case fastgltf::Filter::LinearMipMapNearest:
                    return VK_SAMPLER_MIPMAP_MODE_NEAREST;

                case fastgltf::Filter::NearestMipMapLinear:
                case fastgltf::Filter::LinearMipMapLinear:
                default:
                    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }
        }

        AllocatedTexture loadBRDFLUT(const Device& device, VmaAllocator allocator_handle, VkCommandPool& pool_handle, const std::string& filename) {
            std::cout << "fastgltf::sources::URI& filePath" << std::endl;
            const std::string path(filename.c_str());
            int width, height, nrChannels;
            unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
            AllocatedTexture new_image;
            if (data) {
                VkExtent3D imagesize;
                imagesize.width = width;
                imagesize.height = height;
                imagesize.depth = 1;

                new_image = Texture::upload(device, allocator_handle, pool_handle, data, imagesize,
                                            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                stbi_image_free(data);
                
            }

            return new_image;
        }

        VkSampler createIBLSampler(VkDevice device) {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;  // Smooth interpolation
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // Use mipmapping for specular map
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.anisotropyEnable = VK_FALSE; // Optional: Set to VK_TRUE if using anisotropic filtering
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
        
            VkSampler sampler;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create IBL sampler!");
            }
            return sampler;
        }

        AllocatedTexture loadKTXTexture(const Device& device, VkCommandPool& pool_handle, const std::string& filename) {
            // Load KTX texture
            ktxTexture* ktxTex;
            KTX_error_code result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);

            if (result != KTX_SUCCESS) {
                throw std::runtime_error("Failed to load KTX texture: " + filename);
            }

            // Create KTX Vulkan device info
            ktxVulkanDeviceInfo vdi;
            ktxVulkanDeviceInfo_Construct(&vdi, device.physical_handle, device.logical_handle, device.graphics_queue, pool_handle, nullptr);

            // Prepare Vulkan texture structure
            ktxVulkanTexture vkTex;
            result = ktxTexture_VkUpload(ktxTex, &vdi, &vkTex);

            if (result != KTX_SUCCESS) {
                ktxTexture_Destroy(ktxTex);
                ktxVulkanDeviceInfo_Destruct(&vdi);
                throw std::runtime_error("Failed to upload KTX texture to Vulkan: " + filename);
            }

            // Store texture in Vulkan AllocatedTexture structure
            AllocatedTexture texture;
            texture.image = vkTex.image;

            // Cleanup
            ktxTexture_Destroy(ktxTex);
            ktxVulkanDeviceInfo_Destruct(&vdi);

            return texture;
        }

        std::optional<AllocatedTexture> loadImage(Device& device, VmaAllocator& allocator_handle,
                                                  VkCommandPool& pool_handle, fastgltf::Asset& asset,
                                                  fastgltf::Image& image) {
            AllocatedTexture newImage{};

            int width, height, nrChannels;

            std::visit(fastgltf::visitor{
                           [](auto& arg) {
                           },
                           [&](fastgltf::sources::Array& array) {
                               std::cout << "fastgltf::sources::Array& array" << std::endl;
                           },
                           [&](fastgltf::sources::Fallback& fallback) {
                               std::cout << "fastgltf::sources::Fallback& fallback" << std::endl;
                           },
                           [&](fastgltf::sources::ByteView& byteview) {
                               std::cout << "fastgltf::sources::ByteView& byteview" << std::endl;
                           },
                            [&](fastgltf::sources::CustomBuffer& custombuffer) {
                               std::cout << "fastgltf::sources::CustomBuffer& custombuffer" << std::endl;
                           },
                           [&](fastgltf::sources::URI& filePath) {
                               assert(filePath.fileByteOffset == 0);
                               assert(filePath.uri.isLocalPath());
                               std::cout << "fastgltf::sources::URI& filePath" << std::endl;
                               const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                               unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                               if (data) {
                                   VkExtent3D imagesize;
                                   imagesize.width = width;
                                   imagesize.height = height;
                                   imagesize.depth = 1;

                                   newImage = Texture::upload(device, allocator_handle, pool_handle, data, imagesize,
                                                              VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                                   stbi_image_free(data);
                                   
                               }
                           },
                           [&](fastgltf::sources::Vector& vector) {
                               unsigned char* data = stbi_load_from_memory(
                                   reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
                                   static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                               std::cout << "fastgltf::sources::Vector& vector" << std::endl;
                               if (data) {
                                   VkExtent3D imagesize;
                                   imagesize.width = width;
                                   imagesize.height = height;
                                   imagesize.depth = 1;

                                   newImage = Texture::upload(device, allocator_handle, pool_handle, data, imagesize,
                                                              VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                                   stbi_image_free(data);
                                   
                               }
                           },
                           [&](fastgltf::sources::BufferView& view) {
                               auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                               auto& buffer = asset.buffers[bufferView.bufferIndex];
                               auto vec = std::get<fastgltf::sources::Array>(buffer.data);
                               unsigned char* data = stbi_load_from_memory(
                                   reinterpret_cast<const stbi_uc*>(vec.bytes.data()) + bufferView.byteOffset,
                                   static_cast<int>(bufferView.byteLength), &width, &height, &nrChannels, 4);
                               std::cout << "fastgltf::sources::BufferView& view" << std::endl;
                               if (data) {
                                   VkExtent3D imagesize;
                                   imagesize.width = width;
                                   imagesize.height = height;
                                   imagesize.depth = 1;

                                   newImage = Texture::upload(device, allocator_handle, pool_handle, data, imagesize,
                                                              VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                                   stbi_image_free(data);
                                   
                               } else {
                                   std::cout << "load data is null" << std::endl;
                               }
                           },
                       },
                       image.data);

            if (newImage.image == VK_NULL_HANDLE) {
                std::cout << "image is null" << std::endl;
                return {};
            } else {
                return newImage;
            }
        }
    }  // namespace ResourceManagement
}  // namespace Vulkan