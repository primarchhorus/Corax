#include "mesh.h"
#include "device.h"
#include "vulkan_operations.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/projection.hpp>
#include <glm/gtx/transform.hpp>

namespace Vulkan {

    // Mesh::Mesh() {}

    // Mesh::~Mesh() {}

    // void Mesh::upload(Device& device, VmaAllocator allocator_handle,
    //                   VkCommandPool pool_handle) {
    //     createVertexBuffer(device, allocator_handle, pool_handle);
    //     createIndexBuffer(device, allocator_handle, pool_handle);
    // }

    // void Mesh::createVertexBuffer(Device& device, VmaAllocator allocator_handle,
    //                               VkCommandPool pool_handle) {
    //     VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    //     vertex_buffer = Immediate::uploadBuffer<Vertex>(
    //         vertices, allocator_handle, pool_handle, device,
    //         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
    //             VK_BUFFER_USAGE_TRANSFER_DST_BIT |
    //             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    //         VMA_MEMORY_USAGE_GPU_ONLY);
    //     VkBufferDeviceAddressInfo deviceAdressInfo{
    //         .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    //         .buffer = vertex_buffer.buffer};
    //     vertex_buffer_address =
    //         vkGetBufferDeviceAddress(device.logical_handle, &deviceAdressInfo);
    // }

    // void Mesh::createIndexBuffer(Device& device, VmaAllocator allocator_handle,
    //                              VkCommandPool pool_handle) {
    //     VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
    //     index_buffer = Immediate::uploadBuffer<uint32_t>(
    //         indices, allocator_handle, pool_handle, device,
    //         VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    //         VMA_MEMORY_USAGE_GPU_ONLY, sizeof(vertices[0]) * vertices.size());
    // }

    // void Mesh::draw(VkCommandBuffer cmd, VkPipelineLayout layout_handle,
    //                 VkExtent2D extent, float delta_time) {
    //     static float angle = 0.0f;
    //     angle += delta_time * glm::radians(45.0f);

    //     glm::mat4 model =
    //         glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    //     glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
    //                                  glm::vec3(0.0f, 0.0f, 0.0f),
    //                                  glm::vec3(0.0f, 1.0f, 0.0f));
    //     glm::mat4 projection = glm::perspective(
    //         glm::radians(70.f), (float)extent.width / (float)(extent.height),
    //         0.1f, 10.0f);

    //     projection[1][1] *= -1;
    //     GPUDrawPushConstants push_constants;
    //     push_constants.worldMatrix = projection * view * model;
    //     push_constants.vertexBuffer = vertex_buffer_address;

    //     vkCmdPushConstants(cmd, layout_handle, VK_SHADER_STAGE_VERTEX_BIT, 0,
    //                        sizeof(GPUDrawPushConstants), &push_constants);
    //     vkCmdBindIndexBuffer(cmd, index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    //     vkCmdDrawIndexed(cmd, sub_primitives[0].count, 1,
    //                      sub_primitives[0].start_index, 0, 0);
    // }

    // void Mesh::destroy(VmaAllocator allocator_handle) {
    //     Buffer::destroyBuffer(allocator_handle, vertex_buffer);
    //     Buffer::destroyBuffer(allocator_handle, index_buffer);
    // }

    namespace MeshOperations {

        void createMeshVertexBuffer(Device& device, VmaAllocator allocator_handle,
                                VkCommandPool pool_handle,
                                std::span<Vertex> vertices,
                                MeshBuffer& upload_buffer) {
            VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

            upload_buffer.vertex_buffer = Immediate::uploadBuffer<Vertex>(
                vertices, allocator_handle, pool_handle, device,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
            VkBufferDeviceAddressInfo deviceAdressInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = upload_buffer.vertex_buffer.buffer};
            upload_buffer.vertex_buffer_address = vkGetBufferDeviceAddress(
                device.logical_handle, &deviceAdressInfo);
        }

        void createMeshIndexBuffer(Device& device, VmaAllocator allocator_handle,
                               VkCommandPool pool_handle,
                               std::span<uint32_t> indices,
                               MeshBuffer& upload_buffer, uint32_t offset) {
            VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
            upload_buffer.index_buffer = Immediate::uploadBuffer<uint32_t>(
                indices, allocator_handle, pool_handle, device,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, offset);
        }

        MeshBuffer uploadMeshData(Device& device, VmaAllocator allocator_handle,
                    VkCommandPool pool_handle, std::span<uint32_t> indices,
                    std::span<Vertex> vertices) {
            MeshBuffer upload_buffer;
            createMeshVertexBuffer(device, allocator_handle, pool_handle, vertices,
                               upload_buffer);
            createMeshIndexBuffer(device, allocator_handle, pool_handle, indices,
                              upload_buffer,
                              (sizeof(vertices[0]) * vertices.size()));
            return upload_buffer;
        }

        void destroyMesh(VmaAllocator allocator_handle,
                         MeshBuffer mesh_buffer) {
            
            Buffer::destroyBuffer(allocator_handle, mesh_buffer.vertex_buffer);
            Buffer::destroyBuffer(allocator_handle, mesh_buffer.index_buffer);
        }
    }  // namespace MeshOperations
}  // namespace Vulkan