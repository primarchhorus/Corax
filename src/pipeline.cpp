#include "pipeline.h"
#include "vulkan_utils.h"
#include <array>

namespace Vulkan {

    namespace Pipeline {
        void destroyPipelineObject(const Device& device, const std::unique_ptr<Object>& pipeline)
        {
            vkDestroyPipelineLayout(device.logical_handle, pipeline->layout_handle, nullptr);
            vkDestroyPipeline(device.logical_handle, pipeline->handle, nullptr);
        }

        std::unique_ptr<Object> createPipelineObject(const Device& device, const Configuration& config)
        {
            /*
            Could use a memory allocator here and placement new the pipeline object from some arena/free list thingo maybe
            */
            std::unique_ptr<Object> pipeline = std::make_unique<Object>();
            pipeline->vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            pipeline->vertex_input_info.vertexBindingDescriptionCount = 0; // No vertex buffers
            pipeline->vertex_input_info.pVertexBindingDescriptions = nullptr;
            pipeline->vertex_input_info.vertexAttributeDescriptionCount = 0;
            pipeline->vertex_input_info.pVertexAttributeDescriptions = nullptr;

            pipeline->input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            pipeline->input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            pipeline->input_assembly.primitiveRestartEnable = VK_FALSE;

            std::vector<VkDynamicState> dynamic_states = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };

            // std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {{
            //     {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)}, // Position
            //     {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},   // Normal
            //     {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv_x)},        // UV
            //     {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}    // Tangent
            // }};
            
            // std::array<VkVertexInputBindingDescription, 1> bindingDescriptions = {{
            //     {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
            // }};
            
            // pipeline->vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            // pipeline->vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
            // pipeline->vertex_input_info.pVertexBindingDescriptions = bindingDescriptions.data();
            // pipeline->vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
            // pipeline->vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

            pipeline->dynamic_states_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            pipeline->dynamic_states_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
            pipeline->dynamic_states_info.pDynamicStates = dynamic_states.data();

            pipeline->viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            pipeline->viewport_state.viewportCount = 1;
            pipeline->viewport_state.scissorCount = 1;

            pipeline->rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            pipeline->rasterizer.depthClampEnable = VK_FALSE;
            pipeline->rasterizer.rasterizerDiscardEnable = VK_FALSE;
            pipeline->rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            pipeline->rasterizer.lineWidth = 1.0;
            pipeline->rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
            pipeline->rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            // pipeline->rasterizer.depthClampEnable = VK_TRUE;
            pipeline->rasterizer.depthBiasEnable = VK_FALSE;
            pipeline->rasterizer.depthBiasConstantFactor = 0.0f;
            pipeline->rasterizer.depthBiasClamp = 0.0f;
            pipeline->rasterizer.depthBiasSlopeFactor = 0.0f;

            pipeline->multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            pipeline->multisampling.sampleShadingEnable = VK_FALSE;
            pipeline->multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            pipeline->multisampling.minSampleShading = 1.0f;
            pipeline->multisampling.pSampleMask = nullptr;
            pipeline->multisampling.alphaToCoverageEnable = VK_FALSE;
            pipeline->multisampling.alphaToOneEnable = VK_FALSE;

            pipeline->color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            pipeline->color_blend_attachment.blendEnable = config.enable_blend;
            pipeline->color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            pipeline->color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            pipeline->color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            pipeline->color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            pipeline->color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            pipeline->color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

            pipeline->color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            pipeline->color_blending.logicOpEnable = VK_FALSE;
            pipeline->color_blending.logicOp = VK_LOGIC_OP_COPY;
            pipeline->color_blending.attachmentCount = 1;
            pipeline->color_blending.pAttachments = &pipeline->color_blend_attachment;
            pipeline->color_blending.blendConstants[0] = 0.0f;
            pipeline->color_blending.blendConstants[1] = 0.0f;
            pipeline->color_blending.blendConstants[2] = 0.0f;
            pipeline->color_blending.blendConstants[3] = 0.0f;

            pipeline->buffer_range.offset = 0;
            pipeline->buffer_range.size = sizeof(GPUDrawPushConstants);
            pipeline->buffer_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            pipeline->pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline->pipeline_layout_info.setLayoutCount = config.num_descriptor_sets;
            pipeline->pipeline_layout_info.pSetLayouts = config.descriptor_set_layout;
            pipeline->pipeline_layout_info.pushConstantRangeCount = 1;
            pipeline->pipeline_layout_info.pPushConstantRanges = &pipeline->buffer_range;

            vkCheck(vkCreatePipelineLayout(device.logical_handle, &pipeline->pipeline_layout_info, nullptr, &pipeline->layout_handle));

            pipeline->render_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
            pipeline->render_info.pNext = nullptr; 
            pipeline->render_info.colorAttachmentCount = 1;
            pipeline->render_info.pColorAttachmentFormats = &config.format;
            pipeline->render_info.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

            VkPipelineShaderStageCreateInfo stages[]= {config.vertex_stages, config.fragment_stages};
            pipeline->pipeline_info.pNext = &pipeline->render_info; 
            pipeline->pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline->pipeline_info.stageCount = 2;
            pipeline->pipeline_info.pStages = stages;

            pipeline->depth_stencil_info.depthTestEnable = config.enable_depth;
            pipeline->depth_stencil_info.depthWriteEnable = config.enable_depth;
            pipeline->depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
            pipeline->depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
            pipeline->depth_stencil_info.stencilTestEnable = VK_FALSE;
            pipeline->depth_stencil_info.front = {};
            pipeline->depth_stencil_info.back = {};
            pipeline->depth_stencil_info.minDepthBounds = 0.f;
            pipeline->depth_stencil_info.maxDepthBounds = 1.f;

            
            pipeline->pipeline_info.pVertexInputState = &pipeline->vertex_input_info;
            pipeline->pipeline_info.pInputAssemblyState = &pipeline->input_assembly;
            pipeline->pipeline_info.pViewportState = &pipeline->viewport_state;
            pipeline->pipeline_info.pRasterizationState = &pipeline->rasterizer;
            pipeline->pipeline_info.pMultisampleState = &pipeline->multisampling;
            pipeline->pipeline_info.pDepthStencilState = &pipeline->depth_stencil_info;
            pipeline->pipeline_info.pColorBlendState = &pipeline->color_blending;
            pipeline->pipeline_info.pDynamicState = &pipeline->dynamic_states_info;

            pipeline->pipeline_info.layout = pipeline->layout_handle;
            pipeline->pipeline_info.renderPass = VK_NULL_HANDLE;
            pipeline->pipeline_info.subpass = 0;
            pipeline->pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline->pipeline_info.basePipelineIndex = -1;


            vkCheck(vkCreateGraphicsPipelines(device.logical_handle, VK_NULL_HANDLE, 1, &pipeline->pipeline_info, nullptr, &pipeline->handle)); 

            return std::move(pipeline);
        }

        Object* getPipelineFromCache(const Configuration& config, Cache& cache) {
            auto it = cache.object_map.find(config.name);
            if (it != cache.object_map.end()) {
                return it->second.get();
            }
            return VK_NULL_HANDLE;
        }

        void addPipelineToCache(const Configuration& config, Cache& cache, std::unique_ptr<Object> pipeline)
        {
            cache.object_map[config.name] = std::move(pipeline);
        }

        void clearCache(const Device& device, Cache& cache)
        {
            for (const auto& pair : cache.object_map) {
                destroyPipelineObject(device, pair.second);
            }
            cache.object_map.clear();
        }

        bool loadShader(Shader& shader) {
            std::error_code error;
            std::filesystem::path path(shader.filename);
            std::cout << path << std::endl;
            bool exists = std::filesystem::exists(path, error);
            if (error || !exists)
            {
                return false;
            }
            const uint32_t chunk_size = 4096;
        #ifdef _WIN32
            HANDLE file = CreateFile(shader.filename.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE) {
                return false;
            }

            std::vector<char> buffer(chunk_size);
            DWORD bytes_read;
            while (ReadFile(file, buffer.data(), chunk_size, &bytes_read, nullptr) && bytes_read > 0) {
                shader.spirv_binary.insert(shader.spirv_binary.end(),
                    reinterpret_cast<const uint32_t*>(buffer.data()),
                    reinterpret_cast<const uint32_t*>(buffer.data() + bytes_read));
            }

            CloseHandle(file);
        #else
            int fd = open(shader.filename.c_str(), O_RDONLY);
            if (fd == -1) {
                return false;
            }

            std::vector<char> buffer(chunk_size);
            ssize_t bytes_read;
            while ((bytes_read = read(fd, buffer.data(), chunk_size)) > 0) {
                shader.spirv_binary.insert(shader.spirv_binary.end(),
                    reinterpret_cast<const uint32_t*>(buffer.data()),
                    reinterpret_cast<const uint32_t*>(buffer.data() + bytes_read));
            }

            close(fd);
        #endif

            return true;
        }

        void createShaderModule(const Device& device, Shader& shader) {
            VkShaderModuleCreateInfo shader_info{};
            shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shader_info.codeSize = shader.spirv_binary.size() * sizeof(uint32_t);
            shader_info.pCode = shader.spirv_binary.data();

            vkCheck(vkCreateShaderModule(device.logical_handle, &shader_info, nullptr, &shader.module));
        }

        void destroyShaderModule(const Device& device, Shader& shader) {
            if (shader.module)
            {
                vkDestroyShaderModule(device.logical_handle, shader.module, nullptr);
            }
        }
    }
}
