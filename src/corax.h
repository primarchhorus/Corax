// class Renderer {
// public:
//     Renderer();
//     ~Renderer();

//     // Initialization
//     void initialize(const RendererDesc& desc);
//     void shutdown();

//     // Resource Management
//     ResourceManager& getResourceManager() { return *resourceManager; }
//     SceneGraph& getSceneGraph() { return *sceneGraph; }
    
//     // Frame Management
//     void beginFrame();
//     void endFrame();
    
//     // Rendering
//     void setCamera(const Camera& camera);
//     void drawMesh(MeshHandle mesh, MaterialHandle material, const Transform& transform);
//     void drawScene(NodeHandle rootNode);

// private:
//     std::unique_ptr<ResourceManager> resourceManager;
//     std::unique_ptr<MemoryManager> memoryManager;
//     std::unique_ptr<SceneGraph> sceneGraph;
    
//     // Existing Vulkan members
//     Vulkan::Window window;
//     Vulkan::Instance instance;
//     Vulkan::Device device;
//     Vulkan::SwapChain swapChain;
//     Vulkan::FrameSync frameSync;
// };

#pragma once

#include "vulkan_common.h"
#include "window.h"
#include "instance.h"
#include "frame_sync.h"
#include "dynamic_rendering.h"
#include "pipeline.h"
#include "device.h"
#include "swap_chain.h"
#include "mesh.h"
#include "resource_manager.h"
#include "material.h"
#include "camera.h"

namespace Vulkan 
{
    struct CoraxRenderer {
        void run();
        void init();
        void createPipelineObjects();
        void updateRenderingInfo();
        void beginFrame(float delta_time);
        void endFrame(FrameResources& frame);
        void mainLoop();
        void destroy();
        void recreateSwapChain();
        void initDepthImage();
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void updateScene(float delta_time);
        static void processInputKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void processInputMouseEvent(GLFWwindow* window, double xpos, double ypos);

        Window glfw_window{};
        Instance instance{};
        Device device{};
        SwapChain swap_chain{};
        FrameSync frame_sync{};

        // Pipeline::Object* main_pipeline{};
        Pipeline::Configuration basic_mesh_pipeline_config{};
        Pipeline::Cache pipeline_cache{};
        Pipeline::Shader mesh_vertex{};
        Pipeline::Shader mesh_fragment{};
        
        bool swap_chain_resized{false};
        VkRenderingAttachmentInfoKHR color_attachment_info{};
        VkImageMemoryBarrier image_memory_barrier{};
        VkImageMemoryBarrier depth_barrier{};
        VkRect2D area{};
        VkRenderingInfo render_info{};
        uint32_t current_index{0};
        MeshBuffer rectangle;
        AllocatedImage depth_image;
        uint64_t last_frame_index{0};

        VkCommandPool transfer_pool;
        VmaAllocator allocator;

        Scene scene_data;
        DescriptorLayout global_layout;
        DescriptorLayout scene_layout;
        DescriptorWrite descriptor_write;
        DescriptorAllocation global_descriptor_allocator{.pool_size_ratios = { 
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
        }, .num_pools = 1, .sets_per_pool = 1000};

        AllocatedTexture _whiteImage;
        AllocatedTexture _blackImage;
        AllocatedTexture _greyImage;
        AllocatedTexture _errorCheckerboardImage;

        VkSampler _defaultSamplerLinear;
        VkSampler _defaultSamplerNearest;

        MaterialOperation::MaterialInstance default_data;
        MaterialOperation::GLTFOperations material_operations;

        DeletionQueue main_deletion_queue;

        MaterialOperation::DrawContext main_draw_context;
        std::unordered_map<std::string, std::shared_ptr<MaterialOperation::Node>> loaded_nodes;

        Camera::Type fps_camera;
    };
}
