#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vma/vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <span>
#include <thread>
#include <functional>
#include <deque>

namespace Vulkan 
{
    struct AllocatedBuffer 
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
    };

    struct MeshBuffer {

        AllocatedBuffer index_buffer;
        AllocatedBuffer vertex_buffer;
        VkDeviceAddress vertex_buffer_address;
    };

    struct Vertex {

        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 color;
    };

    struct GPUDrawPushConstants {
        glm::mat4 worldMatrix;
        VkDeviceAddress vertexBuffer;
    };

    struct AllocatedImage {
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
        VkDeviceMemory memory;
        VkExtent3D imageExtent;
        VkFormat imageFormat;
    };

    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    struct DescriptorAllocation {
        std::vector<VkDescriptorPool> full_pools;
        std::vector<VkDescriptorPool> ready_pools;
        std::vector<PoolSizeRatio> pool_size_ratios;
        /*
        At some point i would like to have a free sets concept in here, going to leave it for now, this should be enough for lots of things
        */
        VkDescriptorType type;
        size_t num_pools;
        size_t sets_per_pool;
    };

    struct DescriptorWrite {
        std::deque<VkDescriptorImageInfo> image_info_queue;
        std::deque<VkDescriptorBufferInfo> buffer_info_queue;
        std::vector<VkWriteDescriptorSet> writes;
    };

    struct DescriptorLayout {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        VkDescriptorSetLayout layout_handle{VK_NULL_HANDLE};
    };

    struct Scene {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view_projection;
        glm::vec4 ambient_color;
        glm::vec4 sunlight_direction; // w for sun power
        glm::vec4 sunlight_color;
        glm::vec4 camera_position; 
        glm::vec4 light_position;
    };

    struct DeletionQueue
    {
        std::deque<std::function<void()>> deletors;

        void pushDeleter(std::function<void()>&& function) {
            deletors.push_back(function);
        }

        void flush() {
            for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
                if (*it)
                {
                    (*it)();
                }
                
            }

            deletors.clear();
        }
    };

    struct AllocatedTexture {
        VkImage image;
        VkImageView view;
        VmaAllocation allocation;
        VkExtent3D extent;
        VkFormat format;
    };
};
