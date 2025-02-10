#pragma once

#include "device.h"
#include "vulkan_common.h"

namespace Vulkan {

    struct Instance;

    namespace Transition {
        void image(VkCommandBuffer command, VkImage image,
                   VkImageLayout old_layout, VkImageLayout new_layout,
                   VkPipelineStageFlags src_stage_mask,
                   VkPipelineStageFlags dst_stage_mask, VkAccessFlags src_access_flag, VkAccessFlags dst_access_flag);
    }

    namespace CommandPool {
        VkCommandPool createPool(const Device& device);
        void createBuffers(const Device& device, VkCommandPool handle,
                           std::vector<VkCommandBuffer>& buffer_handles);
        void freeBuffers(const Device& device, VkCommandPool handle,
                         std::vector<VkCommandBuffer>& buffer_handles,
                         const uint32_t buffer_count);
        void destroyPool(const Device& device, VkCommandPool handle);
    }  // namespace CommandPool

    namespace MemoryAllocator {
        void destroyAllocator(VmaAllocator handle);
        VmaAllocator createAllocator(const Instance& instance,
                                     const Device& device);
    }  // namespace MemoryAllocator

    namespace Buffer {
        AllocatedBuffer allocateBuffer(VmaAllocator handle, const size_t size,
                                       const VkBufferUsageFlags usage_flag,
                                       const VmaMemoryUsage usage_type);
        void destroyBuffer(VmaAllocator handle, const AllocatedBuffer& buffer);
    }  // namespace Buffer

    namespace Immediate {
        template <typename T>
        AllocatedBuffer uploadBuffer(const std::span<T>& data,
                                     VmaAllocator allocator_handle,
                                     VkCommandPool pool_handle,
                                     const Device& device,
                                     VkBufferUsageFlags usage,
                                     VmaMemoryUsage memory_usage,
                                     uint32_t offset = 0) {
            VkDeviceSize buffer_size = sizeof(T) * data.size();

            AllocatedBuffer staging_buffer = Buffer::allocateBuffer(
                allocator_handle, buffer_size + offset,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

            memcpy((char*)staging_buffer.info.pMappedData + offset, data.data(),
                   buffer_size);
            AllocatedBuffer device_buffer = Buffer::allocateBuffer(
                allocator_handle, buffer_size, usage, memory_usage);

            Submit(
                [&](VkCommandBuffer cmd) {
                    VkBufferCopy copy{};
                    copy.size = buffer_size;
                    copy.dstOffset = 0;
                    copy.srcOffset = offset;
                    vkCmdCopyBuffer(cmd, staging_buffer.buffer,
                                    device_buffer.buffer, 1, &copy);
                },
                pool_handle, device);

            Buffer::destroyBuffer(allocator_handle, staging_buffer);

            return device_buffer;
        }

        void Submit(std::function<void(VkCommandBuffer cmd)>&& function,
                    VkCommandPool pool_handle, const Device& device);
    }  // namespace Immediate

    namespace Descriptors {
        void initPool(DescriptorAllocation& allocator, const Device& device);
        void clearPools(DescriptorAllocation& allocator, const Device& device);
        void destroyPools(DescriptorAllocation& allocator,
                          const Device& device);
        VkDescriptorSet allocate(DescriptorAllocation& allocator,
                                 const Device& device,
                                 VkDescriptorSetLayout layout,
                                 void* p_next = nullptr);
        VkDescriptorPool getPool(DescriptorAllocation& allocator,
                                 const Device& device);
        VkDescriptorPool createPool(const Device& device, const std::vector<PoolSizeRatio>& pool_size_ratios,
                                    uint32_t sets);

        void writeImage(DescriptorWrite& writer, uint32_t binding,
                        VkImageView image, VkSampler sampler,
                        VkImageLayout layout, VkDescriptorType type);
        void writeBuffer(DescriptorWrite& writer, uint32_t binding,
                         VkBuffer buffer, size_t size, size_t offset,
                         VkDescriptorType type);
        void clearBinds(DescriptorWrite& writer);
        void updateSet(DescriptorWrite& writer, const Device& device,
                       VkDescriptorSet set);

        void addLayoutBinding(DescriptorLayout& layout, uint32_t binding,
                              VkDescriptorType type, VkShaderStageFlags flags);
        void clearLayoutBindings(DescriptorLayout& layout);
        void destroyLayout(const Device& device, DescriptorLayout& layout);
        void buildLayout(
            DescriptorLayout& layout, const Device& device,
            VkShaderStageFlags shaderStages = 0, void* pNext = nullptr,
            VkDescriptorSetLayoutCreateFlags flags = 0);

        constexpr size_t max_descriptor_sets{4096};
    }  // namespace Descriptors

    namespace Texture {
        AllocatedTexture create(const Device& device, VmaAllocator handle,
                                VkExtent3D size, VkFormat format,
                                VkImageUsageFlags usage,
                                bool mipmapped = false);
        AllocatedTexture upload(const Device& device, VmaAllocator handle,
                                VkCommandPool pool_handle, void* data,
                                VkExtent3D size, VkFormat format,
                                VkImageUsageFlags usage,
                                bool mipmapped = false);
        void destroy(const Device& device, VmaAllocator handle, const AllocatedTexture& img);
    }  // namespace Texture
}  // namespace Vulkan