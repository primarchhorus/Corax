#include "command_pool.h"
#include "device.h"

namespace Vulkan {
    CommandPool::CommandPool()
    {

    }

    CommandPool::~CommandPool()
    {
    }

    CommandPool::CommandPool(CommandPool&& other) noexcept
        : handle(std::move(other.handle)) {
    }

    CommandPool& CommandPool::operator=(CommandPool&& other) noexcept {
        if (this != &other) {
            handle = std::move(other.handle);
            other.handle = VK_NULL_HANDLE;
        }
        return *this;
    }

    void CommandPool::createPool(const Device& device)
    {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = device.suitability.queue_fam_indexes.at("draw");

        vkCheck(vkCreateCommandPool(device.logical_handle, &pool_info, nullptr, &handle));
    }

    void CommandPool::createBuffer(const Device& device, std::vector<VkCommandBuffer>& buffer)
    {
        VkCommandBufferAllocateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        buffer_info.commandPool = handle;
        buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        buffer_info.commandBufferCount = static_cast<uint32_t>(buffer.size());

       vkCheck(vkAllocateCommandBuffers(device.logical_handle, &buffer_info, buffer.data()));
    }

    void CommandPool::freeBuffers(const Device& device, std::vector<VkCommandBuffer>& buffer, uint32_t buffer_count)
    {
       vkFreeCommandBuffers(device.logical_handle, handle, buffer_count, buffer.data());
    }

    void CommandPool::destroy(const Device& device)
    {
        if (handle)
        {
            vkDestroyCommandPool(device.logical_handle, handle, nullptr);
        }
    }
}
  