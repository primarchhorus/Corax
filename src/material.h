#pragma once

#include "pipeline.h"
#include "vulkan_common.h"

namespace Vulkan {
    namespace MaterialOperation {
        // Not thrilled with some of the naming, going to make another pass on that once i totally get an understanding of the needs
        enum class MaterialPass {
            MAINCOLOUR,
            TRANSPARENTCOLOUR,
            OTHER
        };

        struct MaterialInstance {
            Pipeline::Object* pipeline;
            VkDescriptorSet material_set;
            MaterialPass pass_type;
        };

        struct RenderObject {
            uint32_t index_ount;
            uint32_t first_index;
            VkBuffer index_buffer;

            MaterialInstance* material;

            glm::mat4 transform;
            VkDeviceAddress vertex_buffer_address;
        };

        struct DrawContext {
            std::vector<RenderObject> opaque_surfaces;
            std::vector<RenderObject> transparent_surfaces;
        };

        struct IRenderable {

            virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
        };

        struct Bounds {
            glm::vec3 origin;
            float sphere_radius;
            glm::vec3 extents;
        };

        struct GeoSurface {
            uint32_t start_index;
            uint32_t count;
            Bounds bounds;
            std::shared_ptr<MaterialInstance> material;
        };

        struct MeshAsset {
            std::string name;
            std::vector<GeoSurface> surfaces;
            MeshBuffer mesh_buffers;
        };

        struct Node : public IRenderable {

            // parent pointer must be a weak pointer to avoid circular dependencies
            std::weak_ptr<Node> parent;
            std::vector<std::shared_ptr<Node>> children;

            glm::mat4 localTransform;
            glm::mat4 worldTransform;

            void refreshTransform(const glm::mat4& parentMatrix)
            {
                worldTransform = parentMatrix * localTransform;
                for (auto c : children) {
                    c->refreshTransform(worldTransform);
                }
            }

            virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx)
            {
                // draw children
                for (auto& c : children) {
                    c->Draw(topMatrix, ctx);
                }
            }
        };

        struct MeshNode : public Node {

            std::shared_ptr<MeshAsset> mesh;

            virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
        };

        struct alignas(256) MaterialConstants {
            glm::vec4 color_factors;
            float metal_factors;
            float rough_factors;
            float ao;
            uint32_t has_metal_rough_texture{0};
            //Something about padding, the alignas should do it, but perhaps that doesnt work for GPU memory?
            glm::vec4 extra[14];// turns out both are good
        };

        struct MaterialResources {
            AllocatedTexture color_image;
            VkSampler color_sampler;
            AllocatedTexture metal_rough_image;
            VkSampler metal_rough_sampler;
            VkBuffer data_buffer;
            uint32_t data_buffer_offset;
        };

        struct GLTFOperations {
            Pipeline::Configuration opaque_pipeline_config{};
            Pipeline::Configuration transparent_pipeline_config{};
            Pipeline::Object opaque_pipeline{};
            Pipeline::Object transparent_pipeline{};
            DescriptorLayout material_layout{};
            DescriptorWrite writer{};

        };

        struct LoadedGLTF : public IRenderable {

            // storage for all the data on a given glTF file
            std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
            std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
            std::unordered_map<std::string, AllocatedTexture> textures;
            std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> materials;

            // nodes that dont have a parent, for iterating through the file in tree order
            std::vector<std::shared_ptr<Node>> top_nodes{};

            std::vector<VkSampler> samplers;

            DescriptorAllocation descriptor_pool{.pool_size_ratios = {
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
            }, .num_pools = 1, .sets_per_pool = 1000};

            AllocatedBuffer material_data_buffer;

            std::function<void()> onDestroy;

            ~LoadedGLTF() { };

            virtual void Draw(const glm::mat4& top_matrix, DrawContext& ctx);

            void setCleanupFunction(std::function<void()> destroy_func) {
                onDestroy = std::move(destroy_func);
            }
        };

        
        void buildPipelines(const Device& device, const SwapChain& swap_chain,
                        GLTFOperations& gltf_material,
                        Pipeline::Cache& pipeline_cache, DescriptorLayout& scene_layout);
        void destroyResources(const Device& device, GLTFOperations& material_operator);
        MaterialInstance writeMaterial(
                        const Device& device, MaterialPass pass,
                        const MaterialResources& resources,
                        DescriptorAllocation& descriptor_allocator,
                        GLTFOperations& material_operator,
                        Pipeline::Cache& pipeline_cache);
    }  // namespace Material
}  // namespace Vulkan
