#pragma once

#include "vulkan_common.h"
#include "vulkan_utils.h"
#include "device.h"
#include "swap_chain.h"
#include "pipeline.h"

namespace Vulkan {
        inline bool operator==(const VkPipelineShaderStageCreateInfo& lhs, const VkPipelineShaderStageCreateInfo& rhs) {
            return lhs.sType == rhs.sType &&
                lhs.pNext == rhs.pNext &&
                lhs.flags == rhs.flags &&
                lhs.stage == rhs.stage &&
                lhs.module == rhs.module &&
                lhs.pName == rhs.pName &&
                lhs.pSpecializationInfo == rhs.pSpecializationInfo;
        }
        struct PipeLineCache {
        PipeLineCache() {}

        Vulkan::PipeLine* getPipeline(const Vulkan::PipeLineConfig& config) {
            auto it = cache.find(config.name);
            if (it != cache.end()) {
                return it->second.get();
            }
            return VK_NULL_HANDLE;

        }

        void createPipeline(Device& device, const Vulkan::PipeLineConfig& config)
        {
            /*
            Could use a memory allocator here and placement new the pipeline object from some arena/free list thingo
            */

            auto pipeline = std::make_unique<PipeLine>(device, config);
            cache[config.name] = std::move(pipeline);
        }

        void clearCache()
        {
            cache.clear();
        }
        // Update to use the config as the hash when my life force can deal with all the overloading of the == operator and hash specialization into the dark of the void of whats required.
        std::unordered_map<std::string, std::unique_ptr<Vulkan::PipeLine>> cache;
    };
}