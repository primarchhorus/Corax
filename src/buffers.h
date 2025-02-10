#pragma once

#include "vulkan_common.h"

#include <vma/vk_mem_alloc.h>

namespace Vulkan {

    struct Device;
    struct Instance;

    struct MemoryAllocator 
    {
        MemoryAllocator();
        ~MemoryAllocator();
        MemoryAllocator(const MemoryAllocator&) = delete;
        MemoryAllocator& operator=(const MemoryAllocator&) = delete;
        MemoryAllocator(MemoryAllocator&& other) noexcept;
        MemoryAllocator& operator=(MemoryAllocator&& other) noexcept;
        
        void destroy();
        void create(const Instance& instance, const Device& device);
        AllocatedBuffer allocateBuffer(size_t size, VkBufferUsageFlags usage_flag, VmaMemoryUsage usage_type);
        void destroyBuffer(const AllocatedBuffer& buffer);

        VmaAllocator handle{VK_NULL_HANDLE};

    };
}
