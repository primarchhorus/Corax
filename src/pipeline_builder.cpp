#include "pipeline_builder.h"

namespace Vulkan 
{
     PipeLineBuilder::PipeLineBuilder(Device& device) : device_ref(device)
    {

    }

    PipeLineBuilder::~PipeLineBuilder()
    {
        try
        {
            destroy();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    PipeLineBuilder::PipeLineBuilder(PipeLineBuilder&& other) noexcept
        : cache(std::move(other.cache)), device_ref(other.device_ref) {
        
    }

    PipeLineBuilder& PipeLineBuilder::operator=(PipeLineBuilder&& other) noexcept {
        if (this != &other) {
            destroy();
            cache = std::move(other.cache);
        }
        return *this;
    }


    void PipeLineBuilder::buildPipeLines(const SwapChain& swap_chain)
    {
        loadShaders();

        configs.triangle.name = "triangle_pipeline";
        VkPipelineShaderStageCreateInfo vertex_info{};
        vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_info.module = modules.triangle_vertex.module;
        vertex_info.pName = "main";
        configs.triangle.vertex_stages = vertex_info;

        VkPipelineShaderStageCreateInfo frag_info{};
        frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_info.module = modules.triangle_frag.module;
        frag_info.pName = "main";
        configs.triangle.fragment_stages = frag_info;
        configs.triangle.format = swap_chain.image_format;
        configs.triangle.extent = swap_chain.extent;
        cache.createPipeline(device_ref, configs.triangle);

        /*
        Assuming i create them all here on start up, after the shader loading/compilation
        */
    }

    PipeLine* PipeLineBuilder::getPipeLine(const PipeLineConfig& config)
    {
        return cache.getPipeline(config);
    }

    void PipeLineBuilder::loadShaders()
    {
        modules.triangle_vertex.load("C:/Users/bgarner/Documents/repos/Corax/build/shaders/shader.vert.spv");
        modules.triangle_frag.load("C:/Users/bgarner/Documents/repos/Corax/build/shaders/shader.frag.spv");
        modules.triangle_vertex.create(device_ref);
        modules.triangle_frag.create(device_ref);
    }

    void PipeLineBuilder::destroy()
    {
        cache.clearCache();
        modules.triangle_vertex.destroy(device_ref);
        modules.triangle_frag.destroy(device_ref);
    }

}