#include "vulkan_operations.h"
#include "device.h"
#include "instance.h"
#include "vulkan_utils.h"

#include <ranges>

namespace Vulkan {

    namespace Transition {
        void image(VkCommandBuffer command, VkImage image,
                   VkImageLayout old_layout, VkImageLayout new_layout,
                   VkPipelineStageFlags src_stage_mask,
                   VkPipelineStageFlags dst_stage_mask,
                   VkAccessFlags src_access_flag,
                   VkAccessFlags dst_access_flag) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = old_layout;
            barrier.newLayout = new_layout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask =
                (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                    ? VK_IMAGE_ASPECT_DEPTH_BIT
                    : VK_IMAGE_ASPECT_COLOR_BIT;
            ;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = src_access_flag;
            barrier.dstAccessMask = dst_access_flag;

            vkCmdPipelineBarrier(command, src_stage_mask, dst_stage_mask, 0, 0,
                                 nullptr, 0, nullptr, 1, &barrier);
        }

    }  // namespace Transition

    namespace CommandPool {
        VkCommandPool createPool(const Device& device) {
            VkCommandPool handle{};
            VkCommandPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            pool_info.queueFamilyIndex =
                device.suitability.queue_fam_indexes.at("draw");

            vkCheck(vkCreateCommandPool(device.logical_handle, &pool_info,
                                        nullptr, &handle));
            return handle;
        }

        void createBuffers(const Device& device, VkCommandPool handle,
                           std::vector<VkCommandBuffer>& buffer) {
            VkCommandBufferAllocateInfo buffer_info{};
            buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            buffer_info.commandPool = handle;
            buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            buffer_info.commandBufferCount =
                static_cast<uint32_t>(buffer.size());

            vkCheck(vkAllocateCommandBuffers(device.logical_handle,
                                             &buffer_info, buffer.data()));
        }

        void freeBuffers(const Device& device, VkCommandPool handle,
                         std::vector<VkCommandBuffer>& buffer,
                         const uint32_t buffer_count) {
            vkFreeCommandBuffers(device.logical_handle, handle, buffer_count,
                                 buffer.data());
        }

        void destroyPool(const Device& device, VkCommandPool handle) {
            if (handle) {
                vkDestroyCommandPool(device.logical_handle, handle, nullptr);
            }
        }
    }  // namespace CommandPool

    namespace MemoryAllocator {
        VmaAllocator createAllocator(const Instance& instance,
                                     const Device& device) {
            VmaAllocator handle{};
            VmaAllocatorCreateInfo allocator_info = {};
            allocator_info.flags =
                VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
            allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
            allocator_info.physicalDevice = device.physical_handle;
            allocator_info.device = device.logical_handle;
            allocator_info.instance = instance.handle;

            VmaVulkanFunctions vulkanFunctions{};
            vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
            vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
            allocator_info.pVulkanFunctions = &vulkanFunctions;

            vmaCreateAllocator(&allocator_info, &handle);
            return handle;
        }

        void destroyAllocator(VmaAllocator handle) {
            if (handle) {
                vmaDestroyAllocator(handle);
            }
        }
    }  // namespace MemoryAllocator

    namespace Buffer {
        AllocatedBuffer allocateBuffer(VmaAllocator handle, const size_t size,
                                       const VkBufferUsageFlags usage_flag,
                                       const VmaMemoryUsage usage_type) {
            VkBufferCreateInfo buffer_info{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            buffer_info.pNext = nullptr;
            buffer_info.size = size;

            buffer_info.usage = usage_flag;

            VmaAllocationCreateInfo allocation_info{};
            allocation_info.usage = usage_type;
            allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
            AllocatedBuffer buffer{};

            vkCheck(vmaCreateBuffer(handle, &buffer_info, &allocation_info,
                                    &buffer.buffer, &buffer.allocation,
                                    &buffer.info));

            return buffer;
        }

        void destroyBuffer(VmaAllocator handle, const AllocatedBuffer& buffer) {
            if (buffer.buffer != VK_NULL_HANDLE) {
                vmaDestroyBuffer(handle, buffer.buffer, buffer.allocation);
            }
        }
    }  // namespace Buffer

    namespace Immediate {
        void Submit(std::function<void(VkCommandBuffer cmd)>&& function,
                    VkCommandPool pool_handle, const Device& device) {
            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandPool = pool_handle;
            alloc_info.commandBufferCount = 1;

            std::vector<VkCommandBuffer> command_buffers{1};
            CommandPool::createBuffers(device, pool_handle, command_buffers);

            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(command_buffers[0], &begin_info);
            function(command_buffers[0]);
            vkEndCommandBuffer(command_buffers[0]);

            VkSubmitInfo submit{};
            submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &command_buffers[0];

            vkQueueSubmit(device.graphics_queue, 1, &submit, VK_NULL_HANDLE);
            vkQueueWaitIdle(device.graphics_queue);

            CommandPool::freeBuffers(device, pool_handle, command_buffers,
                                     command_buffers.size());
        }
    }  // namespace Immediate

    namespace Descriptors {
        void initPool(DescriptorAllocation& allocator, const Device& device) {
            allocator.ready_pools.resize(allocator.num_pools);
            size_t start = 0;
            for (size_t i :
                 std::ranges::views::iota(start, allocator.num_pools)) {
                allocator.ready_pools[i] =
                    createPool(device, allocator.pool_size_ratios, allocator.sets_per_pool);
            }
        }

        void clearPools(DescriptorAllocation& allocator, const Device& device) {
            for (auto& pool : allocator.ready_pools) {
                vkResetDescriptorPool(device.logical_handle, pool, 0);
            }
            for (auto& pool : allocator.full_pools) {
                vkResetDescriptorPool(device.logical_handle, pool, 0);
                allocator.ready_pools.push_back(pool);
            }
            allocator.full_pools.clear();
        }

        void destroyPools(DescriptorAllocation& allocator,
                          const Device& device) {
            for (auto& pool : allocator.ready_pools) {
                vkDestroyDescriptorPool(device.logical_handle, pool, nullptr);
            }
            allocator.ready_pools.clear();

            for (auto& pool : allocator.full_pools) {
                vkDestroyDescriptorPool(device.logical_handle, pool, nullptr);
            }
            allocator.full_pools.clear();
        }

        VkDescriptorSet allocate(DescriptorAllocation& allocator,
                                 const Device& device,
                                 VkDescriptorSetLayout layout, void* p_next) {
            VkDescriptorPool pool_in_use = getPool(allocator, device);

            VkDescriptorSetAllocateInfo allocation_info{};
            allocation_info.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocation_info.descriptorPool = pool_in_use;
            allocation_info.descriptorSetCount = 1;
            allocation_info.pSetLayouts = &layout;
            allocation_info.pNext = p_next;

            VkDescriptorSet descriptor_set;
            VkResult result = vkAllocateDescriptorSets(
                device.logical_handle, &allocation_info, &descriptor_set);

            if (result == VK_ERROR_OUT_OF_POOL_MEMORY ||
                result == VK_ERROR_FRAGMENTED_POOL) {
                allocator.full_pools.push_back(pool_in_use);

                pool_in_use = getPool(allocator, device);
                allocation_info.descriptorPool = pool_in_use;

                vkCheck(vkAllocateDescriptorSets(
                    device.logical_handle, &allocation_info, &descriptor_set));
            }

            allocator.ready_pools.push_back(pool_in_use);
            return descriptor_set;
        }

        VkDescriptorPool getPool(DescriptorAllocation& allocator,
                                 const Device& device) {
            VkDescriptorPool new_pool;
            if (!allocator.ready_pools.empty()) {
                new_pool = allocator.ready_pools.back();
                allocator.ready_pools.pop_back();
            } else {
                new_pool =
                    createPool(device, allocator.pool_size_ratios, allocator.sets_per_pool);
                allocator.sets_per_pool =
                    std::min(static_cast<size_t>(allocator.sets_per_pool * 1.5),
                             max_descriptor_sets);
            }

            return new_pool;
        }

        VkDescriptorPool createPool(const Device& device, const std::vector<PoolSizeRatio>& pool_size_ratios,
                                    uint32_t sets) {
            std::vector<VkDescriptorPoolSize> pool_sizes;
            for (const auto& ratio : pool_size_ratios) {
                VkDescriptorPoolSize pool_size{};
                pool_size.type = ratio.type;
                pool_size.descriptorCount = static_cast<uint32_t>(sets * ratio.ratio);
                pool_sizes.push_back(pool_size);
            }

            VkDescriptorPoolCreateInfo pool_create_info{};
            pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
            pool_create_info.pPoolSizes = pool_sizes.data();
            pool_create_info.maxSets = sets;

            VkDescriptorPool descriptor_pool;
            vkCheck(vkCreateDescriptorPool(device.logical_handle, &pool_create_info, nullptr, &descriptor_pool));
            return descriptor_pool;
        }

        void writeImage(DescriptorWrite& writer, uint32_t binding,
                        VkImageView image, VkSampler sampler,
                        VkImageLayout layout, VkDescriptorType type) {
            VkDescriptorImageInfo& info = writer.image_info_queue.emplace_back(
                VkDescriptorImageInfo{.sampler = sampler,
                                      .imageView = image,
                                      .imageLayout = layout});

            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

            write.dstBinding = binding;
            write.dstSet =
                VK_NULL_HANDLE;  //left empty for now until we need to write it
            write.descriptorCount = 1;
            write.descriptorType = type;
            write.pImageInfo = &info;

            writer.writes.push_back(write);
        }

        void writeBuffer(DescriptorWrite& writer, uint32_t binding,
                         VkBuffer buffer, size_t size, size_t offset,
                         VkDescriptorType type) {
            VkDescriptorBufferInfo& info =
                writer.buffer_info_queue.emplace_back(VkDescriptorBufferInfo{
                    .buffer = buffer, .offset = offset, .range = size});

            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

            write.dstBinding = binding;
            write.dstSet =
                VK_NULL_HANDLE;  //left empty for now until we need to write it
            write.descriptorCount = 1;
            write.descriptorType = type;
            write.pBufferInfo = &info;

            writer.writes.push_back(write);
        }

        void clearBinds(DescriptorWrite& writer) {
            writer.image_info_queue.clear();
            writer.writes.clear();
            writer.buffer_info_queue.clear();
        }

        void updateSet(DescriptorWrite& writer, const Device& device,
                       VkDescriptorSet set) {
            for (VkWriteDescriptorSet& write : writer.writes) {
                write.dstSet = set;
            }

            vkUpdateDescriptorSets(device.logical_handle,
                                   static_cast<uint32_t>(writer.writes.size()),
                                   writer.writes.data(), 0, nullptr);
            clearBinds(writer);
        }

        void addLayoutBinding(DescriptorLayout& layout, uint32_t binding,
                              VkDescriptorType type, VkShaderStageFlags flags) {
            VkDescriptorSetLayoutBinding newbind{};
            newbind.binding = binding;
            newbind.descriptorCount = 1;
            newbind.descriptorType = type;
            newbind.stageFlags = flags;

            layout.bindings.push_back(newbind);
        }

        void clearLayoutBindings(DescriptorLayout& layout) {
            layout.bindings.clear();
        }

        void destroyLayout(const Device& device, DescriptorLayout& layout) {
            if (layout.layout_handle)
            {
                vkDestroyDescriptorSetLayout(device.logical_handle, layout.layout_handle,
                                 nullptr);
            }
        }

        void buildLayout(
            DescriptorLayout& layout, const Device& device,
            VkShaderStageFlags shaderStages, void* pNext,
            VkDescriptorSetLayoutCreateFlags flags) {
            // for (auto& b : layout.bindings) {
            //     b.stageFlags |= shaderStages;
            // }

            VkDescriptorSetLayoutCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            info.pNext = pNext;

            info.pBindings = layout.bindings.data();
            info.bindingCount = static_cast<uint32_t>(layout.bindings.size());
            info.flags = flags;

            vkCheck(vkCreateDescriptorSetLayout(device.logical_handle, &info,
                                                nullptr, &layout.layout_handle));

        }
    }  // namespace Descriptors

    namespace Texture {
        AllocatedTexture create(const Device& device, VmaAllocator handle,
                                VkExtent3D size, VkFormat format,
                                VkImageUsageFlags usage, bool mipmapped) {
            AllocatedTexture new_image{};
            new_image.format = format;
            new_image.extent = size;

            VkImageCreateInfo img_info{};
            img_info.format = format;
            img_info.usage = usage;
            img_info.extent = size;
            img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            img_info.pNext = nullptr;
            img_info.imageType = VK_IMAGE_TYPE_2D;
            img_info.mipLevels = 1;
            img_info.arrayLayers = 1;
            // img_info.

            //for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
            img_info.samples = VK_SAMPLE_COUNT_1_BIT;

            //optimal tiling, which means the image is stored on the best gpu format
            img_info.tiling = VK_IMAGE_TILING_OPTIMAL;

            if (mipmapped) {
                img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(
                                         std::max(size.width, size.height)))) +
                                     1;
            }

            VmaAllocationCreateInfo allocinfo = {};
            allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            allocinfo.requiredFlags =
                VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            vkCheck(vmaCreateImage(handle, &img_info, &allocinfo,
                                   &new_image.image, &new_image.allocation,
                                   nullptr));

            VkImageAspectFlags aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
            if (format == VK_FORMAT_D32_SFLOAT) {
                aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
            }

            VkImageViewCreateInfo view_info{};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = format;
            view_info.image = new_image.image;
            view_info.subresourceRange.baseMipLevel = 0;
            view_info.subresourceRange.levelCount = img_info.mipLevels;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount = 1;
            view_info.subresourceRange.aspectMask = aspect_flag;

            vkCheck(vkCreateImageView(device.logical_handle, &view_info,
                                      nullptr, &new_image.view));

            return new_image;
        }

        AllocatedTexture upload(const Device& device, VmaAllocator handle,
                                VkCommandPool pool_handle, void* data,
                                VkExtent3D size, VkFormat format,
                                VkImageUsageFlags usage, bool mipmapped) {
            size_t data_size = size.depth * size.width * size.height * 4;
            AllocatedBuffer uploadbuffer = Buffer::allocateBuffer(
                handle, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU);

            memcpy(uploadbuffer.info.pMappedData, data, data_size);

            AllocatedTexture new_image =
                create(device, handle, size, format,
                       usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                           VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                       mipmapped);

            Immediate::Submit(
                [&](VkCommandBuffer cmd) {
                    Transition::image(cmd, new_image.image,
                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                      VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                      VK_ACCESS_TRANSFER_WRITE_BIT);

                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset = 0;
                    copyRegion.bufferRowLength = 0;
                    copyRegion.bufferImageHeight = 0;

                    copyRegion.imageSubresource.aspectMask =
                        VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel = 0;
                    copyRegion.imageSubresource.baseArrayLayer = 0;
                    copyRegion.imageSubresource.layerCount = 1;
                    copyRegion.imageExtent = size;

                    // copy the buffer into the image
                    vkCmdCopyBufferToImage(
                        cmd, uploadbuffer.buffer, new_image.image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                    Transition::image(cmd, new_image.image,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                      VK_ACCESS_TRANSFER_WRITE_BIT,
                                      VK_ACCESS_SHADER_READ_BIT);
                },
                pool_handle, device);

            Buffer::destroyBuffer(handle, uploadbuffer);

            return new_image;
        }

        void destroy(const Device& device, VmaAllocator handle,
                     const AllocatedTexture& img) {
            vkDestroyImageView(device.logical_handle, img.view, nullptr);
            vmaDestroyImage(handle, img.image, img.allocation);
        }
    }  // namespace Texture
}  // namespace Vulkan