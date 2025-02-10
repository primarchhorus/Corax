#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 color;
};

struct TextureDesc {
    uint32_t width;
    uint32_t height;
    VkFormat format;
    bool generateMips;
    VkImageUsageFlags usage;
};

struct MaterialDesc {
    std::string shader_path;
    std::unordered_map<std::string, TextureHandle> textures;
    std::unordered_map<std::string, glm::vec4> parameters;
};

namespace Vulkan {
class Mesh {
public:
    Mesh(Device& device);
    ~Mesh();

    void setVertices(const std::vector<Vertex>& vertices);
    void setIndices(const std::vector<uint32_t>& indices);
    void draw(VkCommandBuffer cmd);

private:
    Device& device;
    AllocatedBuffer vertex_buffer;
    AllocatedBuffer index_buffer;
    uint32_t vertex_count{0};
    uint32_t index_count{0};
    
    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
};
}

#include "mesh.h"

namespace Vulkan {

Mesh::Mesh(Device& device) : device(device) {}

Mesh::~Mesh() {
    device.destroy_buffer(vertex_buffer);
    device.destroy_buffer(index_buffer);
}

void Mesh::setVertices(const std::vector<Vertex>& vertices) {
    vertex_count = static_cast<uint32_t>(vertices.size());
    createVertexBuffer(vertices);
}

void Mesh::setIndices(const std::vector<uint32_t>& indices) {
    index_count = static_cast<uint32_t>(indices.size());
    createIndexBuffer(indices);
}

void Mesh::createVertexBuffer(const std::vector<Vertex>& vertices) {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
    
    AllocatedBuffer staging = device.create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );

    void* data = staging.info.pMappedData;
    memcpy(data, vertices.data(), buffer_size);

    vertex_buffer = device.create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    device.immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy copy{};
        copy.size = buffer_size;
        vkCmdCopyBuffer(cmd, staging.buffer, vertex_buffer.buffer, 1, &copy);
    });

    device.destroy_buffer(staging);
}

void Mesh::createIndexBuffer(const std::vector<uint32_t>& indices) {
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
    
    AllocatedBuffer staging = device.create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );

    void* data = staging.info.pMappedData;
    memcpy(data, indices.data(), buffer_size);

    index_buffer = device.create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    device.immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy copy{};
        copy.size = buffer_size;
        vkCmdCopyBuffer(cmd, staging.buffer, index_buffer.buffer, 1, &copy);
    });

    device.destroy_buffer(staging);
}

void Mesh::draw(VkCommandBuffer cmd) {
    VkBuffer buffers[] = { vertex_buffer.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
    
    if (index_count > 0) {
        vkCmdBindIndexBuffer(cmd, index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, index_count, 1, 0, 0, 0);
    } else {
        vkCmdDraw(cmd, vertex_count, 1, 0, 0);
    }
}

}

#pragma once
#include "vulkan_common.h"

namespace Vulkan {

class Texture {
public:
    Texture(Device& device);
    ~Texture();

    void create(const TextureDesc& desc, const void* data);
    void createView(VkFormat format, VkImageAspectFlags aspect);
    void createSampler();
    
    VkImageView getImageView() const { return image_view; }
    VkSampler getSampler() const { return sampler; }

private:
    Device& device;
    AllocatedImage image;
    VkImageView image_view{VK_NULL_HANDLE};
    VkSampler sampler{VK_NULL_HANDLE};
    
    void transitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout);
    void generateMipmaps(VkCommandBuffer cmd);
};

}

#pragma once
#include "texture.h"

namespace Vulkan {

Texture::Texture(Device& device) : device(device) {}

Texture::~Texture() {
    if (sampler != VK_NULL_HANDLE)
        vkDestroySampler(device.logical_handle, sampler, nullptr);
    if (image_view != VK_NULL_HANDLE)
        vkDestroyImageView(device.logical_handle, image_view, nullptr);
    device.destroy_image(image);
}

void Texture::create(const TextureDesc& desc, const void* data) {
    VkExtent3D extent{desc.width, desc.height, 1};
    
    image = device.create_image(
        extent,
        desc.format,
        desc.usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    size_t image_size = desc.width * desc.height * 4; // Assuming RGBA8
    
    AllocatedBuffer staging = device.create_buffer(
        image_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );

    memcpy(staging.info.pMappedData, data, image_size);

    device.immediate_submit([&](VkCommandBuffer cmd) {
        transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        
        VkBufferImageCopy copy{};
        copy.imageExtent = extent;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        
        vkCmdCopyBufferToImage(cmd, staging.buffer, image.image, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        if (desc.generateMips)
            generateMipmaps(cmd);
        else
            transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    device.destroy_buffer(staging);
}

void Texture::createView(VkFormat format, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(device.logical_handle, &view_info, nullptr, &image_view));
}

void Texture::createSampler() {
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16.0f;

    VK_CHECK(vkCreateSampler(device.logical_handle, &sampler_info, nullptr, &sampler));
}

void Texture::transitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

void Texture::generateMipmaps(VkCommandBuffer cmd) {
    // Mipmap generation implementation
}

}

#pragma once
#include "material_types.h"
#include "texture.h"

namespace Vulkan {

class Material {
public:
    Material(Device& device, const MaterialDesc& desc);
    ~Material();

    void setParameter(const std::string& name, const MaterialParameterValue& value);
    void setTexture(const std::string& name, Texture* texture);
    void bind(VkCommandBuffer cmd);

private:
    Device& device;
    MaterialLayout layout;
    VkDescriptorSet descriptor_set{VK_NULL_HANDLE};
    VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};
    AllocatedBuffer uniform_buffer;
    
    struct UniformBlock {
        alignas(16) glm::mat4 model_matrix{1.0f};
        alignas(16) glm::vec4 parameters[16]{};
    } uniform_data;

    void createDescriptorPool();
    void createDescriptorSet();
    void createUniformBuffer();
    void updateDescriptorSet();
};

}

#pragma once
#include "material.h"

namespace Vulkan {

MaterialOperation::Material(Device& device, const MaterialDesc& desc) : device(device) {
    createDescriptorPool();
    createDescriptorSet();
    createUniformBuffer();
    updateDescriptorSet();
}

MaterialOperation::~Material() {
    vkDestroyDescriptorPool(device.logical_handle, descriptor_pool, nullptr);
    device.destroy_buffer(uniform_buffer);
}

void Material::createDescriptorPool() {
    std::vector<VkDescriptorPoolSize> pool_sizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 }
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = 1;

    VK_CHECK(vkCreateDescriptorPool(device.logical_handle, &pool_info, nullptr, &descriptor_pool));
}

void Material::createDescriptorSet() {
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout.layout;

    VK_CHECK(vkAllocateDescriptorSets(device.logical_handle, &alloc_info, &descriptor_set));
}

void Material::createUniformBuffer() {
    uniform_buffer = device.create_buffer(
        sizeof(UniformBlock),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
}

void Material::setParameter(const std::string& name, const MaterialParameterValue& value) {
    // Update uniform data based on parameter name and value
    // Update descriptor set if needed
}

void Material::setTexture(const std::string& name, Texture* texture) {
    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = texture->getImageView();
    image_info.sampler = texture->getSampler();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_set;
    write.dstBinding = 1; // Update based on parameter layout
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(device.logical_handle, 1, &write, 0, nullptr);
}

void Material::bind(VkCommandBuffer cmd) {
    // Update uniform buffer if needed
    memcpy(uniform_buffer.info.pMappedData, &uniform_data, sizeof(UniformBlock));

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout.layout, 0, 1, &descriptor_set, 0, nullptr);
}

void Material::updateDescriptorSet() {
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = uniform_buffer.buffer;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(UniformBlock);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device.logical_handle, 1, &write, 0, nullptr);
}

}