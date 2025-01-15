#pragma once

#include "pipeline_cache.h"

namespace Vulkan {

    struct Device;
    struct SwapChain;

    struct PipeLineConfigs {
        Vulkan::PipeLineConfig triangle;
    };

    struct ShaderModules {
        Vulkan::ShaderModule triangle_vertex;
        Vulkan::ShaderModule triangle_frag;
    };

    struct PipeLineBuilder {
        PipeLineBuilder(Device& device);
        ~PipeLineBuilder();
        PipeLineBuilder(const PipeLineBuilder&) = delete;
        PipeLineBuilder& operator=(const PipeLineBuilder&) = delete;
        PipeLineBuilder(PipeLineBuilder&& other) noexcept;
        PipeLineBuilder& operator=(PipeLineBuilder&& other) noexcept;

        void destroy();

        void loadShaders();
        void buildPipeLines(const SwapChain& swap_chain);
        PipeLine* getPipeLine(const PipeLineConfig& config);
        

        Device& device_ref;
        PipeLineCache cache;
        PipeLineConfigs configs;
        ShaderModules modules;

    };
}
