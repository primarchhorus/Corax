#include "buffers.h"
#include "instance.h"
#include "device.h"
#include "vulkan_utils.h"

namespace Vulkan
{
    MemoryAllocator::MemoryAllocator()
    {

    }

    MemoryAllocator::~MemoryAllocator()
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

    MemoryAllocator::MemoryAllocator(MemoryAllocator&& other) noexcept
        : handle(std::move(other.handle)) {

    }

    MemoryAllocator& MemoryAllocator::operator=(MemoryAllocator&& other) noexcept {
        if (this != &other) {
            destroy();
            handle = std::move(other.handle);
        }
        return *this;
    }

    void MemoryAllocator::destroy()
    {
        vmaDestroyAllocator(handle);
    }

    void MemoryAllocator::create(const Instance& instance, const Device& device)
    {
        
        VmaAllocatorCreateInfo allocator_info = {};
        allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
        allocator_info.physicalDevice = device.physical_handle;
        allocator_info.device = device.logical_handle;
        allocator_info.instance = instance.handle;

        VmaVulkanFunctions vulkanFunctions{};
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
        allocator_info.pVulkanFunctions = &vulkanFunctions;
        
        vmaCreateAllocator(&allocator_info, &handle);
    }

    AllocatedBuffer MemoryAllocator::allocateBuffer(size_t size, VkBufferUsageFlags usage_flag, VmaMemoryUsage usage_type)
    {
        
        VkBufferCreateInfo buffer_info{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.pNext = nullptr;
        buffer_info.size = size;

        buffer_info.usage = usage_flag;

        VmaAllocationCreateInfo allocation_info{};
        allocation_info.usage = usage_type;
        allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        AllocatedBuffer buffer{};

        vkCheck(vmaCreateBuffer(handle, &buffer_info, &allocation_info, &buffer.buffer, &buffer.allocation,
            &buffer.info));

        return buffer;
    }

    void MemoryAllocator::destroyBuffer(const AllocatedBuffer& buffer)
    {
        vmaDestroyBuffer(handle, buffer.buffer, buffer.allocation);
    }
}