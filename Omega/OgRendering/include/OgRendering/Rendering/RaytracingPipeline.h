#pragma once
#include <OgRendering/Export.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <OgRendering/Rendering/Device.h>
#include <OgRendering/Resource/Model.h>
#include <OgRendering/Resource/Camera.h>
#include <OgRendering/Utils/VulkanTools.h>
#include <OgRendering/Managers/ResourceManager.h>
#include <OgRendering/Rendering/stb_image.h>
#include <OgRendering/UI/imgui/imgui.h>
#include <OgRendering/UI/imgui/imgui_internal.h>
#include <OgRendering/UI/imgui/imgui_impl_glfw.h>
#include <OgRendering/UI/imgui/imgui_impl_vulkan.h>
/*#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>*/

#define MAX_TEXTURES 64
#define MAX_OBJECTS 500

struct Semaphore {
    // Swap chain image presentation
    VkSemaphore presentComplete;
    // Command buffer submission and execution
    VkSemaphore renderComplete;
};

struct UniformData
{
    GPM::Matrix4F viewInverse;
    GPM::Matrix4F projInverse;
};

struct ShaderData
{
    std::vector<Buffer> vertexBuffer;
    std::vector<Buffer> indexBuffer;
    Buffer textureIDBuffer;
};

struct DepthStencil
{
    VkImage m_stencilImage;
    VkDeviceMemory m_stencilMemory;
    VkImageView m_stencilView;
};
struct SwapChainBuffer
{
    VkImage image;
    VkImageView view;
};

struct TextureData
{
    VkImage img;
    VkDescriptorImageInfo info;
    VkDeviceMemory memory;
};
struct SwapChain {
    VkFormat colorFormat{};
    VkColorSpaceKHR colorSpace{};
    /** @brief Handle to the current swap chain, required for recreation */
    VkSwapchainKHR swapChain{};
    uint32_t imageCount;
    std::vector<VkImage> images;
    std::vector<SwapChainBuffer> buffers;
    /** @brief Queue family index of the detected graphics and presenting device queue */
    uint32_t queueNodeIndex = 5000000;
};

struct StorageImage 
{
    VkDeviceMemory memory;
    VkImage image;
    VkImageView view;
    VkFormat format;
};


namespace OgEngine
{
    class RENDERING_API RaytracingPipeline
    {
    public:

        RaytracingPipeline(Device& p_device, uint32_t p_width, uint32_t p_height,
            VkQueue p_graphicQueue, VkQueue p_presentQueue, GLFWwindow* p_window, uint32_t p_minImageCount)
        {
            m_vulkanDevice = p_device;
            m_width = p_width;
            m_height = p_height;
            m_graphicsQueue = p_graphicQueue;
            m_presentQueue = p_presentQueue;
            m_window = p_window;
            m_minImageCount = p_minImageCount;

        }

        ~RaytracingPipeline() = default;

#pragma region Pipeline Methods
        void InitSwapchain();
        void InitRaytracingRenderer(bool resizedWindow);
        void SetRaytracingCmd();
        void SetupSwapchain(uint32_t p_width, uint32_t p_height, bool vsync = false);
        void SetupFramebuffer();
        void SetupDepthStencil();
        void SetupRenderPass();
        void SetupRaytracingPipeline();
        void StartPathTracing();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateBottomLevelAccelerationStructure(const VkGeometryNV* p_geometries);
        void CreateTopLevelAccelerationStructure(AccelerationStructure& p_accelerationStruct, uint32_t p_instanceCount);
        void CreateStorageImage();
        void AddObject(uint64_t p_id, std::shared_ptr<Mesh> p_mesh, uint32_t p_textureID = 0);
        void AddTexture(const char* p_texture);
        void CreateTLAS();
        void CreatePipeline(bool p_resizedWindow);
        void CreateSynchronizationPrimitives();
        void CreatePipelineCache();
        void CreateShaderBindingTable();
        void CreateDescriptorSets(bool resizedWindow);
        void CreateUniformBuffer();
        void UpdateShaderData(std::shared_ptr<Mesh> p_mesh);
        void UpdateTLAS();
        void UpdateDescriptorSets();
        void InitImGUI();
        void SetupImGUIStyle();
        void SetupImGUI();
        void SetupImGUIFrameBuffers();
        void RescaleImGUI();
        void RenderUI(uint32_t p_id);
#pragma endregion

#pragma region External Pipeline Methods
        void GenerateMipmaps(VkImage p_image, VkFormat p_imageFormat, int32_t p_texWidth, int32_t p_texHeight, uint32_t p_mipLevels) const;
        VkDescriptorImageInfo Create2DDescriptor(const VkDevice& device, const VkImage& image, const VkSamplerCreateInfo& samplerCreateInfo, const VkFormat& format, const VkImageLayout& layout);
        void DestroyObjects(); //Deprecated with ECS conversion
        void AddModelInGame(uint64_t p_id, std::shared_ptr<Mesh> p_mesh); //Deprecated with ECS conversion
        void RemoveModelInGame(); //Deprecated with ECS conversion
        void UpdateObject(uint64_t id, const GPM::Matrix4F& p_transform, const std::shared_ptr<Mesh>& p_mesh, uint32_t p_texID);
#pragma endregion

#pragma region Vulkan Helpers
        void QueueCmdBufferAndFlush(VkCommandBuffer p_commandBuffer, VkQueue p_queue, bool p_free = true) const;
        void PrepareFrame();
        void SubmitFrame();
        void Draw();
        void CleanUp();
        void DestroyShaderBuffers(bool p_resizedWindow);
        void ResizeWindow();
        void ResizeCleanup();
        void SetupTestEditor();
        void PrepareIMGUIFrame();
        void DrawImGUI();
        ImGuiContext* GetUIContext() { return ImGui::GetCurrentContext(); }

        static void CHECK_ERROR(VkResult result);
        static void SetImageLayout(VkCommandBuffer cmd_buffer, VkImage p_image, VkImageLayout p_oldImageLayout, VkImageLayout p_newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
        static void SetImageLayout(VkCommandBuffer cmd_buffer, VkImage p_image, VkImageAspectFlags p_aspectMask, VkImageLayout p_oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
        static VkBool32 GetSupportedDepthFormat(VkPhysicalDevice p_physicalDevice, VkFormat* p_depthFormat);
        VkShaderModule CreateShaderModule(const std::vector<char>& p_code) const;
        VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel p_level, bool p_begin) const;
        uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags p_properties, VkBool32* p_memTypeFound = nullptr) const;
        VkPipelineShaderStageCreateInfo LoadShader(std::string p_file_name, VkShaderStageFlagBits p_stage);
        VkDeviceSize CopyShaderIdentifier(uint8_t* p_data, const uint8_t* p_shaderHandleStorage, uint32_t p_groupIndex) const;
        VkResult AcquireNextImage(VkSemaphore p_presentCompleteSemaphore, uint32_t* p_imageIndex) const;
        VkResult QueuePresent(VkQueue p_queue, uint32_t p_imageIndex, VkSemaphore p_waitSemaphore) const;
        VkResult CreateBuffer(VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memoryPropertyFlags, VkDeviceSize p_size, VkBuffer* p_buffer, VkDeviceMemory* p_memory, void* p_data = nullptr) const;
        VkResult CreateBuffer(VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memoryPropertyFlags, Buffer* p_buffer, VkDeviceSize p_size, void* p_data = nullptr) const;
#pragma endregion

#pragma region Pipeline Core Variables
        GLFWwindow* m_window{};
        Device m_vulkanDevice{};
        
        SwapChain m_swapChain;
        DepthStencil m_depthStencil{};
        StorageImage m_storageImage{};
        UniformData m_cameraData{};
        Semaphore m_semaphores{};
        ShaderData m_shaderData{};

        uint32_t m_currentBuffer{ 0 };
        uint32_t m_width{ 0 };
        uint32_t m_height{ 0 };
        uint32_t m_minImageCount{ 0 };
#pragma endregion

#pragma region Pipeline Buffers
        Buffer m_shaderBindingTable;
        Buffer m_cameraBuffer;
        Buffer m_instancesBuffer;
#pragma endregion

#pragma region Pipeline Generic Variables
        Camera m_camera{};

        std::vector<VkShaderModule> m_shaderModules;
        std::vector<VkFence> m_waitFences;
        std::vector<VkCommandBuffer> m_commandBuffers;
        std::vector<VkCommandBuffer> m_ImGUIcommandBuffers;
        std::vector<VkFramebuffer> m_swapchainFrameBuffers;
        std::vector<VkFramebuffer> m_ImGUIframeBuffers;
        //std::vector<uint32_t> m_BLASTriangleCount;
        std::vector<uint32_t> m_InstanceRefToBLAS;
        std::vector<AccelerationStructure> m_BLAS;
        std::vector<std::shared_ptr<Mesh>> m_BLASmeshes;
        std::vector<GeometryInstance> m_instances;
        std::vector<Model> m_objects;
        std::vector<uint64_t> m_objectIDs;
        std::vector<uint32_t> textureIDs;
        AccelerationStructure m_TLAS;
        std::vector<TextureData> m_textures;

        int TLASinstanceID{ 0 };
#pragma endregion

#pragma region Vulkan Direct Variables
        VkQueue m_graphicsQueue{};
        VkQueue m_presentQueue{};
        VkCommandPool m_commandPool{};
        VkCommandPool m_ImGUIcommandPool{};
        VkRenderPass m_renderpass{};
        VkRenderPass m_ImGUIrenderPass{};
        VkPipeline m_pipeline{};
        VkPipelineLayout m_pipelineLayout{};
        VkDescriptorSet m_descriptorSet{};
        VkDescriptorSetLayout m_descriptorSetLayout{};

        VkDescriptorPool m_descriptorPool{};
        VkDescriptorPool m_ImGUIdescriptorPool{};

        VkFormat m_depthFormat{};
        VkPhysicalDeviceRayTracingPropertiesNV m_raytracingProperties{};
        VkPipelineCache m_pipelineCache{};
        VkSubmitInfo m_submitInfo{};
        VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
#pragma endregion

#pragma region Vulkan Function Pointers
        PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
        PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
        PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
        PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
        PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
        PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
        PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
        PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
        PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;
        PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV;
        PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV;
        PFN_vkCmdCopyAccelerationStructureNV vkCmdCopyAccelerationStructureNV;
#pragma endregion
    };
}
