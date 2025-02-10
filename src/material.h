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
            glm::vec4 metal_rough_factors;
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
