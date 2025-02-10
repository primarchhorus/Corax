#pragma once

#include "vulkan_common.h"

namespace Vulkan {

    struct Device;

    // struct SubPrimitive {
    //     uint32_t start_index;
    //     uint32_t count;
    // };

    // struct Mesh {
    //     Mesh();
    //     ~Mesh();
    //     /*
    //     I think in general i would like to get away from the partial OO stuff
    //     Keep the mesh stuff as struct data and move all the functions to some mesh operator, maybe thats in device, maybe thats a new thing that does stuff?
    //     */
    //     void createVertexBuffer(Device& device, VmaAllocator allocator_handle, VkCommandPool pool_handle);
    //     void createIndexBuffer(Device& device, VmaAllocator allocator_handle, VkCommandPool pool_handle);
    //     void upload(Device& device, VmaAllocator allocator_handle, VkCommandPool pool_handle);
    //     void draw(VkCommandBuffer cmd, VkPipelineLayout layout_handle, VkExtent2D extent, float delta_time);
    //     void destroy(VmaAllocator allocator_handle);

    //     AllocatedBuffer vertex_buffer{};
    //     AllocatedBuffer index_buffer{};
    //     VkDeviceAddress vertex_buffer_address{};
    //     std::vector<Vertex> vertices;
    //     std::vector<uint32_t> indices;
    //     uint32_t vertex_count{0};
    //     uint32_t index_count{0};
    //     std::string name{""};
    //     std::vector<SubPrimitive> sub_primitives;

    // };

    namespace MeshOperations {
        void createMeshVertexBuffer(Device& device,
                                    VmaAllocator allocator_handle,
                                    VkCommandPool pool_handle,
                                    std::span<Vertex> vertices,
                                    MeshBuffer& upload_buffer);
        void createMeshIndexBuffer(Device& device,
                                   VmaAllocator allocator_handle,
                                   VkCommandPool pool_handle,
                                   std::span<uint32_t> indices,
                                   MeshBuffer& upload_buffer,
                                   uint32_t offset = 0);
        MeshBuffer uploadMeshData(Device& device, VmaAllocator allocator_handle,
                              VkCommandPool pool_handle,
                              std::span<uint32_t> indices,
                              std::span<Vertex> vertices);
        void destroyMesh(VmaAllocator allocator_handle, MeshBuffer mesh_buffer);
    }
}