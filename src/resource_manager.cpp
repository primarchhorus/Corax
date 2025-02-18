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

// What fucking insanity is this, why the hell does this need to be included after all the other shit
// Is it the inclusion of

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
            AllocatedTexture& default_texture, MaterialOperation::MaterialResources& default_resources, MaterialOperation::GLTFOperations& material_operations, Pipeline::Cache& pipeline_cache) {
            std::cout << "Loading GLTF: " << filepath << std::endl;

            std::shared_ptr<MaterialOperation::LoadedGLTF> scene = std::make_shared<MaterialOperation::LoadedGLTF>();
            MaterialOperation::LoadedGLTF& file = *scene.get();

            auto cleanup = [&device, &allocator_handle, &pool_handle, scene, default_texture](){
                for (auto& [k, v] : scene->meshes) {
                    Buffer::destroyBuffer(allocator_handle, v->mesh_buffers.index_buffer);
                    Buffer::destroyBuffer(allocator_handle,v->mesh_buffers.vertex_buffer);
                }

                for (auto& [k, v] : scene->textures) {

                    if (v.image == default_texture.image) {
                        // dont destroy the default images
                        continue;
                    }
                    Texture::destroy(device, allocator_handle, v);
                }

                for (auto& sampler : scene->samplers) {
                    vkDestroySampler(device.logical_handle, sampler, nullptr);
                }

                Descriptors::destroyPools(scene->descriptor_pool, device);

                Buffer::destroyBuffer(allocator_handle, scene->material_data_buffer);


                std::cout << "destroyng loaded gltf" << std::endl;
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

                sampl.magFilter = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
                sampl.minFilter = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

                sampl.mipmapMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

                VkSampler newSampler;
                vkCreateSampler(device.logical_handle, &sampl, nullptr, &newSampler);

                file.samplers.push_back(newSampler);
            }

            std::vector<std::shared_ptr<MaterialOperation::MeshAsset>> meshes;
            std::vector<std::shared_ptr<MaterialOperation::Node>> nodes;
            std::vector<AllocatedTexture> textures;
            std::vector<std::shared_ptr<MaterialOperation::MaterialInstance>> materials;

            for (fastgltf::Image& image : gltf.images) {
                std::optional<AllocatedTexture> img = loadImage(device, allocator_handle, pool_handle, gltf, image);

                if (img.has_value()) {
                    textures.push_back(img.value());
                    file.textures[image.name.c_str()] = img.value();
                    std::cout << "This should be the proper image " << image.name << std::endl;
                }
                else {
                    // we failed to load, so lets give the slot a default white texture to not
                    // completely break loading
                    textures.push_back(default_texture);
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
                std::shared_ptr<MaterialOperation::MaterialInstance> newMat =
                    std::make_shared<MaterialOperation::MaterialInstance>();
                materials.push_back(newMat);
                file.materials[mat.name.c_str()] = newMat;

                MaterialOperation::MaterialConstants constants;

                float metallic = mat.pbrData.metallicFactor; 
                float roughness = mat.pbrData.roughnessFactor;

                constants.color_factors = glm::clamp(glm::vec4(mat.pbrData.baseColorFactor[0], 
                                                            mat.pbrData.baseColorFactor[1], 
                                                            mat.pbrData.baseColorFactor[2], 1.0f), 
                                                0.0f, 0.95f); // Avoid full 1.0 values

                constants.metal_factors = mat.pbrData.metallicFactor;
                constants.rough_factors = mat.pbrData.roughnessFactor;
                std::cout << mat.name.c_str() << " constants.metal_factors: " << constants.metal_factors << " constants.rough_factors: " << constants.rough_factors << std::endl;
                constants.ao = 0.5f;

                // write material parameters to buffer
                

                MaterialOperation::MaterialPass pass_type = MaterialOperation::MaterialPass::MAINCOLOUR;

                if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
                    pass_type = MaterialOperation::MaterialPass::TRANSPARENTCOLOUR;
                    std::cout << "transparent: " << mat.name.c_str() << std::endl;
                }

                MaterialOperation::MaterialResources material_resources;
                // default the material textures
                material_resources.color_image = default_resources.color_image;
                material_resources.color_sampler = default_resources.color_sampler;
                material_resources.metal_rough_image = default_resources.metal_rough_image;
                material_resources.metal_rough_sampler = default_resources.metal_rough_sampler;

                // set the uniform buffer for the material data
                material_resources.data_buffer = file.material_data_buffer.buffer;
                material_resources.data_buffer_offset = data_index * sizeof(MaterialOperation::MaterialConstants);
                // grab textures from gltf file
                if (mat.pbrData.baseColorTexture.has_value()) {
                    size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                    size_t sampler =
                        gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                    material_resources.color_image = textures[img];
                    material_resources.color_sampler = file.samplers[sampler];
                }

                if (mat.pbrData.metallicRoughnessTexture.has_value()) {
                    constants.has_metal_rough_texture = 1;
                    size_t img = gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
                    size_t sampler =
                        gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.value();

                    material_resources.metal_rough_image= textures[img];
                    material_resources.metal_rough_sampler = file.samplers[sampler];
                }
                else
                {
                    constants.has_metal_rough_texture = 0;
                }
                // build material

                scene_material_constants[data_index] = constants;

                *newMat = MaterialOperation::writeMaterial(
                device, pass_type,
                material_resources, file.descriptor_pool,
                material_operations, pipeline_cache);

                data_index++;
            }

            std::vector<uint32_t> indices;
            std::vector<Vertex> vertices;

            for (fastgltf::Mesh& mesh : gltf.meshes) {
                std::shared_ptr<MaterialOperation::MeshAsset> newmesh = std::make_shared<MaterialOperation::MeshAsset>();
                meshes.push_back(newmesh);
                file.meshes[mesh.name.c_str()] = newmesh;
                newmesh->name = mesh.name;

                // clear the mesh arrays each mesh, we dont want to merge them by error
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

                        fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                            [&](std::uint32_t idx) {
                                indices.push_back(idx + initial_vtx);
                            });
                    }

                    // load vertex positions
                    {
                        fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
                        vertices.resize(vertices.size() + posAccessor.count);

                        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                            [&](glm::vec3 v, size_t index) {
                                Vertex newvtx;
                                newvtx.position = v;
                                newvtx.normal = { 1, 0, 0 };
                                newvtx.color = glm::vec4 { 1.f };
                                newvtx.uv_x = 0;
                                newvtx.uv_y = 0;
                                vertices[initial_vtx + index] = newvtx;
                            });
                    }

                    // load vertex normals
                    auto normals = p.findAttribute("NORMAL");
                    if (normals != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).accessorIndex],
                            [&](glm::vec3 v, size_t index) {
                                vertices[initial_vtx + index].normal = v;
                            });
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

                        fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).accessorIndex],
                            [&](glm::vec4 v, size_t index) {
                                vertices[initial_vtx + index].color = v;
                            });
                    }

                    if (p.materialIndex.has_value()) {
                        new_surface.material = materials[p.materialIndex.value()];
                    } else {
                        new_surface.material = materials[0];
                    }

                    newmesh->surfaces.push_back(new_surface);
                }

                newmesh->mesh_buffers = MeshOperations::uploadMeshData(device, allocator_handle, pool_handle, indices, vertices);
            }

         

            for (fastgltf::Node& node : gltf.nodes) {
                std::shared_ptr<MaterialOperation::Node> newNode;

                // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
                if (node.meshIndex.has_value()) {
                    newNode = std::make_shared<MaterialOperation::MeshNode>();
                    static_cast<MaterialOperation::MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
                } else {
                    newNode = std::make_shared<MaterialOperation::Node>();
                }

                nodes.push_back(newNode);
                file.nodes[node.name.c_str()];

                std::visit(fastgltf::visitor { [&](fastgltf::math::fmat4x4 matrix) {
                                                memcpy(&newNode->localTransform, &matrix, sizeof(matrix));
                                            },
                            [&](fastgltf::TRS transform) {
                                glm::vec3 tl(transform.translation[0], transform.translation[1],
                                    transform.translation[2]);
                                glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1],
                                    transform.rotation[2]);
                                glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                glm::mat4 rm = glm::toMat4(rot);
                                glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                newNode->localTransform = tm * rm * sm;
                            } },
                    node.transform);
            }

            for (int i = 0; i < gltf.nodes.size(); i++) {
                fastgltf::Node& node = gltf.nodes[i];
                std::shared_ptr<MaterialOperation::Node>& sceneNode = nodes[i];

                for (auto& c : node.children) {
                    sceneNode->children.push_back(nodes[c]);
                    nodes[c]->parent = sceneNode;
                }
            }

            // find the top nodes, with no parents
            for (auto& node : nodes) {
                if (node->parent.lock() == nullptr) {
                    file.top_nodes.push_back(node);
                    node->refreshTransform(glm::mat4 { 1.f });
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


        std::optional<AllocatedTexture> loadImage(Device& device, VmaAllocator& allocator_handle, VkCommandPool& pool_handle, fastgltf::Asset& asset, fastgltf::Image& image)
        {
            AllocatedTexture newImage {};

            int width, height, nrChannels;

            std::visit(
                fastgltf::visitor {
                    [](auto& arg) {},
                    [&](fastgltf::sources::URI& filePath) {
                        assert(filePath.fileByteOffset == 0); 
                        assert(filePath.uri.isLocalPath()); 
                                                           

                        const std::string path(filePath.uri.path().begin(),
                            filePath.uri.path().end()); 
                        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                        if (data) {
                            VkExtent3D imagesize;
                            imagesize.width = width;
                            imagesize.height = height;
                            imagesize.depth = 1;

                            newImage = Texture::upload(device, allocator_handle, pool_handle, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                            stbi_image_free(data);
                            std::cout << "fastgltf::sources::URI& filePath" << std::endl;
                        }
                    },
                    [&](fastgltf::sources::Vector& vector) {
                        unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()),
                            &width, &height, &nrChannels, 4);
                        if (data) {
                            VkExtent3D imagesize;
                            imagesize.width = width;
                            imagesize.height = height;
                            imagesize.depth = 1;

                            newImage = Texture::upload(device, allocator_handle, pool_handle, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                            stbi_image_free(data);
                            std::cout << "fastgltf::sources::Vector& vector" << std::endl;
                        }
                    },
                    [&](fastgltf::sources::BufferView& view) {
                        auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                        auto& buffer = asset.buffers[bufferView.bufferIndex];
                        auto vec = std::get<fastgltf::sources::Array>(buffer.data);
                        unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vec.bytes.data()) + bufferView.byteOffset,
                                            static_cast<int>(bufferView.byteLength),
                                            &width, &height, &nrChannels, 4);
                                        if (data) {
                                            VkExtent3D imagesize;
                                            imagesize.width = width;
                                            imagesize.height = height;
                                            imagesize.depth = 1;

                                            newImage = Texture::upload(device, allocator_handle, pool_handle, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                                            stbi_image_free(data);
                                            std::cout << "fastgltf::sources::BufferView& view" << std::endl;
                                        }
                                        else {
                                            std::cout << "load data is null" << std::endl;
                                        }
                        // std::visit(fastgltf::visitor { 
                        //             [](auto& arg) {},
                        //             [&](fastgltf::sources::Vector& vector) {
                                        
                        //             } },
                        //     buffer.data);
                        
                    },
                },
                image.data);

            // if any of the attempts to load the data failed, we havent written the image
            // so handle is null
            if (newImage.image == VK_NULL_HANDLE) {
                std::cout << "image is null" << std::endl;
                return {};
            } else {
                return newImage;
            }
        }
    }  // namespace ResourceManagement
}  // namespace Vulkan