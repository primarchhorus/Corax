#include "material.h"
#include "vulkan_operations.h"

namespace Vulkan {
    namespace MaterialOperation {
        void buildPipelines(const Device& device, const SwapChain& swap_chain,
                            GLTFOperations& gltf_material,
                            Pipeline::Cache& pipeline_cache, DescriptorLayout& scene_layout) {

            /*
            I suspect this isnt the best approach, but i dont think i have enough exposure to the use cases
            At what point are these pipelines usuially built, and at what point do you have all the required information.
            This will work for this basic renderer, i dont need a lot of pipelines so i can define them here
            but i would guess that the configuring and building of these things is probably something that need flexibility
            */
            Descriptors::addLayoutBinding(
                gltf_material.material_layout, 0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
            Descriptors::addLayoutBinding(
                gltf_material.material_layout, 1,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
            Descriptors::addLayoutBinding(
                gltf_material.material_layout, 2,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
            Descriptors::buildLayout(gltf_material.material_layout, device);

            Pipeline::Shader mesh_vertex{};
            mesh_vertex.filename =
                "C:/Users/bgarner/Documents/repos/Corax/build/shaders/"
                "mesh.vert.spv";
            Pipeline::loadShader(mesh_vertex);
            Pipeline::createShaderModule(device, mesh_vertex);

            Pipeline::Shader mesh_fragment{};
            mesh_fragment.filename =
                "C:/Users/bgarner/Documents/repos/Corax/build/shaders/"
                "mesh.frag.spv";
            Pipeline::loadShader(mesh_fragment);
            Pipeline::createShaderModule(device, mesh_fragment);

            VkPipelineShaderStageCreateInfo frag_info{};
            frag_info.sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            frag_info.module = mesh_fragment.module;
            frag_info.pName = "main";

            VkPipelineShaderStageCreateInfo vertex_info{};
            vertex_info.sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertex_info.module = mesh_vertex.module;
            vertex_info.pName = "main";

            VkDescriptorSetLayout layouts[] = {scene_layout.layout_handle,
            gltf_material.material_layout.layout_handle};

            gltf_material.opaque_pipeline_config.name = "opaque_pipeline";
            gltf_material.opaque_pipeline_config.vertex_stages = vertex_info;
            gltf_material.opaque_pipeline_config.fragment_stages = frag_info;
            gltf_material.opaque_pipeline_config.format =
                swap_chain.image_format;
            gltf_material.opaque_pipeline_config.extent = swap_chain.extent;
            gltf_material.opaque_pipeline_config.descriptor_set_layout = layouts;
            gltf_material.opaque_pipeline_config.num_descriptor_sets = 2;
            gltf_material.opaque_pipeline_config.enable_blend = VK_FALSE;
            gltf_material.opaque_pipeline_config.enable_depth = VK_TRUE;

            auto opaque_pipeline = Pipeline::createPipelineObject(
                device, gltf_material.opaque_pipeline_config);
            Pipeline::addPipelineToCache(gltf_material.opaque_pipeline_config,
                                         pipeline_cache,
                                         std::move(opaque_pipeline));

            gltf_material.transparent_pipeline_config.name =
                "transparent_pipeline";
            gltf_material.transparent_pipeline_config.vertex_stages = vertex_info;
            gltf_material.transparent_pipeline_config.fragment_stages =
                frag_info;
            gltf_material.transparent_pipeline_config.format =
                swap_chain.image_format;
            gltf_material.transparent_pipeline_config.extent =
                swap_chain.extent;
            gltf_material.transparent_pipeline_config.descriptor_set_layout = layouts;
            gltf_material.transparent_pipeline_config.num_descriptor_sets = 2;
            gltf_material.transparent_pipeline_config.enable_blend = VK_TRUE;
            gltf_material.transparent_pipeline_config.enable_depth = VK_FALSE;

            auto transparent_pipeline = Pipeline::createPipelineObject(
                device, gltf_material.transparent_pipeline_config);
            Pipeline::addPipelineToCache(
                gltf_material.transparent_pipeline_config, pipeline_cache,
                std::move(transparent_pipeline));

            Pipeline::destroyShaderModule(device, mesh_vertex);
            Pipeline::destroyShaderModule(device, mesh_fragment);
        }

        void destroyResources(const Device& device, GLTFOperations& material_operator) {
            Descriptors::destroyLayout(device, material_operator.material_layout);
        }

        MaterialInstance writeMaterial(
            const Device& device, MaterialPass pass,
            const MaterialResources& resources,
            DescriptorAllocation& descriptor_allocator,
            GLTFOperations& material_operator,
            Pipeline::Cache& pipeline_cache) {

            MaterialInstance mat_data;
            mat_data.pass_type = pass;
            if (pass == MaterialPass::TRANSPARENTCOLOUR) {
                mat_data.pipeline = Pipeline::getPipelineFromCache(
                    material_operator.transparent_pipeline_config,
                    pipeline_cache);
            } else {
                mat_data.pipeline = Pipeline::getPipelineFromCache(
                    material_operator.opaque_pipeline_config, pipeline_cache);
            }

            mat_data.material_set = Descriptors::allocate(
                descriptor_allocator, device,
                material_operator.material_layout.layout_handle);

            Descriptors::writeBuffer(
                material_operator.writer, 0, resources.data_buffer,
                sizeof(MaterialResources), resources.data_buffer_offset,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            Descriptors::writeImage(material_operator.writer, 1,
                                    resources.color_image.view,
                                    resources.color_sampler,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            Descriptors::writeImage(material_operator.writer, 2,
                                    resources.metal_rough_image.view,
                                    resources.metal_rough_sampler,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            assert(resources.color_image.view != VK_NULL_HANDLE);
            assert(resources.metal_rough_image.view != VK_NULL_HANDLE);
            Descriptors::updateSet(material_operator.writer, device,
                                   mat_data.material_set);

            return mat_data;
        }

        void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
        {
            glm::mat4 nodeMatrix = topMatrix * worldTransform;

            for (auto& s : mesh->surfaces) {
                RenderObject def;
                def.index_ount = s.count;
                def.first_index = s.start_index;
                def.index_buffer = mesh->mesh_buffers.index_buffer.buffer;
                def.material = s.material.get();

                def.transform = nodeMatrix;
                def.vertex_buffer_address = mesh->mesh_buffers.vertex_buffer_address;
                
                ctx.opaque_surfaces.push_back(def);
            }

            // recurse down
            Node::Draw(topMatrix, ctx);
        }
    }  // namespace Material
}  // namespace Vulkan
