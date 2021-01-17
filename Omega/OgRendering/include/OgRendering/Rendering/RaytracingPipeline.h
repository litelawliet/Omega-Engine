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
#include <OgRendering/Resource/TextureData.h>
#include <OgRendering/Utils/VulkanTools.h>
#include <OgRendering/Managers/ResourceManager.h>
#include <OgRendering/Rendering/SwapChainSupportDetails.h>
#include <OgRendering/UI/imgui/imgui.h>
#include <OgRendering/UI/imgui/imgui_internal.h>
#include <OgRendering/UI/imgui/imgui_impl_glfw.h>
#include <OgRendering/UI/imgui/imgui_impl_vulkan.h>
/*#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>*/

#define MAX_TEXTURES 64
#define MAX_OBJECTS 10000
#define MAX_FRAMES_IN_FLIGHT 2

struct Semaphore {
    // Swap chain image presentation
    VkSemaphore imageAvailable;
    // Command buffer submission and execution
    VkSemaphore renderComplete;
};

struct RTMaterial
{
    glm::vec4 albedo;
    glm::vec4 specular;
    glm::vec4 data;
    glm::vec4 emissive;
};

struct RTLight
{
    glm::vec4 pos;
    glm::vec4 color;
    glm::vec4 dir;
};

struct UniformData
{
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    glm::vec4 data;
    glm::vec4 settings;
    glm::vec4 samples;
};

struct ShaderData
{
    std::vector<Buffer> vertexBuffer;
    std::vector<Buffer> indexBuffer;
    Buffer objectBLASbuffer;
    //std::vector<Buffer> materialBuffer;
    //std::vector<Buffer> lightBuffer;
    //Buffer textureIDBuffer;
    //Buffer normalMapIDBuffer;
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

struct FrameBufferAttachment
{
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;
};

struct ORenderPass
{
    int32_t width, height;
    std::vector<VkFramebuffer> frameBuffers;
    FrameBufferAttachment color, depth;
    VkRenderPass renderPass;
    VkSampler sampler;
    VkDescriptorImageInfo descriptor;
};

struct SwapChain 
{
    VkFormat colorFormat{};
    VkSwapchainKHR swapChain{};
    VkExtent2D extent{};
    uint32_t imageCount{};
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
};

struct GameViewProperties
{
    uint32_t width{ 0u };
    uint32_t height{ 0u };
};

struct StorageImage
{
    VkDeviceMemory memory;
    VkImage image;
    VkImageView view;
    VkFormat format;
    VkSampler imgSampler;
};

struct CustomConstraints
{
    static void Wide(ImGuiSizeCallbackData* data) { data->DesiredSize.y = data->DesiredSize.x * (9.0f / 16.0f); }
    static void Square(ImGuiSizeCallbackData* data) { data->DesiredSize.x = data->DesiredSize.y = (data->DesiredSize.x > data->DesiredSize.y ? data->DesiredSize.x : data->DesiredSize.y); }
    static void Step(ImGuiSizeCallbackData* data) { float step = (float)(int)(intptr_t)data->UserData; data->DesiredSize = ImVec2((int)(data->DesiredSize.x / step + 0.5f) * step, (int)(data->DesiredSize.y / step + 0.5f) * step); }
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

        /**
        *   @brief Initialize the swapChain for the current pipeline
        */
        void FindQueueFamilies();

        /**
        *   @brief Setup the buffers and properties of the pipeline, and bind it to Vulkan
        *   @param p_resizedWindow if the window is resized, only recreates some parts of the pipeline
        */
        void SetupPipelineAndBind();

        /**
        *   @brief Configure the Raytracing Commands required by the NV_RAYTRACING extension
        */
        void ConfigureRaytracingCommands();

        /**
        *   @brief Prepare and Configure the SwapChain for the selected pipeline
        *   @param p_width width of the OS window generated by external library
        *   @param p_height height of the OS window generated by external library
        *   @param p_vsync activate the V-SYNC on the Window (false by default)
        *   @note p_width and p_height must be the same value as the window size, if not, the rendered image might give black borders
        */
        void InitSwapChain(uint32_t p_width, uint32_t p_height, bool p_vsync = false);

        [[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat>& p_candidates, VkImageTiling p_tiling, VkFormatFeatureFlags p_features) const;

        /**
        *   @brief Prepare and configure the SwapChain depth stencil
        */
        //void SetupDepthStencil();


        /**
        *   @brief Automatically prepare and configure the whole Pipeline
        */
        void SetupRaytracingPipeline();

        void InitSyncObjects();
        /**
        *   @brief Tell vulkan to start the Path Tracing extension and fill the image with the traced Data
        *   @note You only need to call it once as Vulkan executes de raytracing algorithm and fills the image asynchronously with the main thread
        */
        void DispatchRays();

        /**
        *   @brief Initialize the SwapChain command pool
        */
        void CreateCommandPool();

        /**
        *   @brief Initialize the SwapChain command buffers
        */
        void CreateCommandBuffers();

        /**
        *   @brief Create geometry data structure that is used by the NV_RAYTRACING extension
        *   @param p_geometries pointer to the previously filled with a mesh and generated
        */
        void CreateBottomLevelAccelerationStructure(const VkGeometryNV* p_geometries);

        /**
        *   @brief Create instance data structure that is used by the NV_RAYTRACING extension
        *   @param p_accelerationStruct acceleration structure selected to be filled
        *   @param p_instanceCount number of instances filled in the p_accelerationStruct
        */
        void CreateTopLevelAccelerationStructure(AccelerationStructure& p_accelerationStruct, uint32_t p_instanceCount);

        void AddNewTopAccelAndBuild();
        void UpdateTopAccelAndBuild();
        void AllocateTopLevelAcceleration(AccelerationStructure& p_accelerationStruct);
        void BindTopLevelAcceleration(AccelerationStructure& p_accelerationStruct);
        /**
        *   @brief Create and configure the image used by the NV_RAYTRACING extension
        *   @param p_image is the image that we will fill de data with
        */
        void CreateStorageImage(StorageImage& p_image);


        void ReloadPipeline();

        /**
        *   @brief Create bindings, layouts and pipeline configuration then Allocate and Bind it to vulkan
        */
        void CreatePipeline();

        void InitBuffers();

        /**
        *   @brief Reloads the current shaders used by the pipeline (for runtime Hot Reload purpose)
        */
        void ReloadShaders();

        /**
        *   @brief Allocate a pipeline cache for Vulkan
        */
        void CreatePipelineCache();
        /**
        *   @brief Setup the binding table with all the raytracing shaders used by the NV_RAYTRACING extension
        */
        void CreateShaderBindingTable();

        /**
        *   @brief Prepare the descriptors sets with the bindings previously bound with CreatePipeline
        *   @param p_resizedWindow receives the window resize event (has been resized or not)
        */
        void CreateDescriptorSets();

        /**
        *   @brief Update the editor camera with the new previously set values
        */
        void UpdateCamera();

        /**
        *   @brief Update the top level acceleration structure with the new entities data
        */
        void UpdateTransforms();

        /**
        *   @brief Initialize the Imgui context with the vulkan context
        */
        void InitImGUI();

        /**
        *   @brief Setup the Editor colors, shape and fonts
        */
        void SetupImGUIStyle();

        /**
        *   @brief Creates and allocates the ImGui command pool, command buffers and setup the vulkan scene Texture to ImGui
        */
        void SetupImGUI();
        /**
        *   @brief Creates and allocates the ImGui required framebuffers
        */
        void SetupImGUIFrameBuffers();

        /**
        *   @brief Rescale the Editor with the new set window dimensions
        */
        void RescaleImGUI();

        /**
        *   @brief Fills the Vulkan rendering queue with the Editor data to be rendered
        *   @param p_id is the current swapChain image index previously acquired to be rendered on
        */
        void RenderUI(uint32_t p_id);

#pragma endregion

#pragma region External Pipeline Methods

        /**
        *   @brief Prepare and setup the Editor Camera
        *   @note This function will be deprecated with the ECS implementation of the Camera
        */
        void CreateCamera();

        /**
        *   @brief Add an Entity in the pipeline so that it can be rendered on screen
        *   @param p_id id of the entity to be stored
        *   @param p_mesh mesh of the entity
        *   @param p_textureID id of the texture to be assigned to the mesh (default = 0, wich is a white texture)
        *   @param p_material material of the entity
        */
        void AddEntity(uint64_t p_id, Mesh* p_mesh, uint32_t p_textureID, RTMaterial p_material, uint32_t p_normID);

        /**
        * @brief Loads an image from path in an understandable format for ImGui
        * @param p_texture is the path of the texture to load
        */
        ImTextureID AddUITexture(const char* p_texture);

        /**
        *   @brief Add a texture in the data storage so that it can be used by Entities
        *   @param p_texture File path to the texture
        */
        void AddTexture(const std::string& p_texture, const TEXTURE_TYPE p_type = TEXTURE_TYPE::TEXTURE);

        /**
        *   @brief Create texture mipmaps
        *   @param p_image texture image previously loaded and allocated
        *   @param p_imageFormat is the texture format
        *   @param p_texWidth is the width of the texture
        *   @param p_texHeight is the height of the texture
        *   @param p_mipLevels is the mipmap level of the texture
        */
        void CreateTextureMipmaps(VkImage p_image, VkFormat p_imageFormat, int32_t p_texWidth, int32_t p_texHeight, uint32_t p_mipLevels) const;

        /**
        *   @brief Create the texture descriptor used by Descriptor sets for Editor and shader puprose
        *   @param p_device is the logical device of the current Vulkan context
        *   @param p_image is the texture from wich the descriptor will be created
        *   @param p_samplerCreateInfo is the texture sampler info previously setup
        *   @param p_format is the texture format
        *   @param p_layout is the texture image layout
        *   @return Returns the newly setup image descriptor info
        */
        VkDescriptorImageInfo CreateTextureDescriptor(const VkDevice& p_device, TextureData& p_image, const VkSamplerCreateInfo& p_samplerCreateInfo, const VkFormat& p_format, const VkImageLayout& p_layout);

        /**
        *   @brief Updates or creates the given entity with the new data sent
        *   @param p_id is the entity id
        *   @param p_transform is the new entity transform
        *   @param p_mesh (if new object) is the mesh associated with the new object
        *   @param p_texID is the texture id that you want to link with the object
        *   @param p_normID is the normalMapID id that you want to link with the object
        *   @param p_albedo is the material color
        *   @param p_roughness is the material roughness
        *   @param p_metallic is the material mettalic
        *   @param p_reflectance is the material reflectance (currently unused)
        *   @param p_type is the material type
        *   @note in fact we need to send the material data by parameters as the pipeline needs a different data structure to be used by the shaders
        */
        void UpdateObject(uint64_t p_id, const glm::mat4& p_transform, Mesh* p_mesh, std::string p_texID,
            const char* p_normID,
            glm::vec4 p_albedo, float p_roughness, float p_ior, glm::vec4 p_specular, glm::vec4 p_emissive, int p_type);

        void DestroyObject(uint64_t p_id);

        void DestroyLight(uint64_t p_id);

        void DestroyAllObjects();
        /**
        *   @brief Updates the selected material with the new data
        *   @param p_id is the entity id on wich we will update the material
        *   @param p_albedo is the material color
        *   @param p_roughness is the material roughness
        *   @param p_metallic is the material mettalic
        *   @param p_reflectance is the material reflectance (currently unused)
        *   @param p_type is the material type
        */
        void UpdateMaterial(uint64_t p_id, glm::vec4 p_albedo, float p_roughness, float p_ior, glm::vec4 p_specular, glm::vec4 p_emissive, int p_type, int p_texID, int p_normID);

        void UpdateLight(uint64_t p_id, glm::vec4 p_position, glm::vec4 p_color, glm::vec4 p_direction, int p_type);

        int GetTexture(const char* p_tex);
        int GetNormalMap(const char* p_norm);
#pragma endregion

#pragma region Vulkan Helpers

        /**
        *   @brief Sends the commandbuffer to the queue, and wait for it to be signaled, then, it gets freed
        *   @param p_commandBuffer is the command buffer that we will submit to the queue
        *   @param p_queue is the queue that the commandbuffer needs to be sent on
        *   @param p_free does the commandbuffer needs to be freed? (default = true)
        */
        void QueueCmdBufferAndFlush(VkCommandBuffer p_commandBuffer, VkQueue p_queue, bool p_free = true) const;

        /**
        *   @brief Render the frame to the screen
        */
        void RenderFrame();

        /**
        *   @brief Destroys the whole pipeline and its variables for shutdown
        */
        void CleanPipeline();

        /**
        *   @brief Destroys the shader buffers and its binding table
        *   @param p_resizedWindow if window is resized, it is instead destroying only required buffers
        */
        void DestroyShaderBuffers(bool p_resizedWindow);

        /*
        *   @brief  Resize the window with the new dimensions, it involves recreating the pipeline
        */
        void ResizeWindow();

        /**
        *   @brief Prepare ImGui to draw a new frame
        */
        void InitImGuiFrame();

        /**
        *   @brief Render the editor, ready to be displayed
        */
        void RenderEditor();

        /**
        *   @brief Setup and prepare the Editor to be rendered
        */
        void SetupEditor();

        /**
        *   @brief Initialize a renderpass so that the scene can be displayed on the editor
        *   @param renderPass is the RenderPass to be initialized
        */
        void SetupRenderPass(ORenderPass& renderPass);

        /**
        *   @brief Loads the raytracing shaders
        *   @param p_piepeline is the pipeline where we will load the shaders in
        */
        void LoadShaders();

        /**
        *   @brief Gets the ImGui context
        *   @returns the imgui context, if not found, returns nullptr
        *   @note InitImGui and SetupImGui have to be called before calling this function
        */
        inline ImGuiContext* GetUIContext();

        /**
        *   @brief Generic function to check for vulkan errors
        *   @param p_result is the function we want to validate
        *   @note if vulkan error appears, throww a runtime error blocking the program
        */
        static void CHECK_ERROR(VkResult p_result);

        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice& p_gpu);

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<struct VkSurfaceFormatKHR>& p_availableFormats);

        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& p_availablePresentModes);

        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& p_capabilities) const;

        VkImageView CreateImageView(VkImage p_image, VkFormat p_format, VkImageAspectFlags p_aspectFlags, uint32_t p_mipLevels) const;

        /**
        *   @brief Set the new image layout
        *   @param p_cmdBuffer is the commandbuffer to record the command in
        *   @param p_image is the image on wich we change the layout
        *   @param p_oldImageLayout is the actual image layout
        *   @param p_newImageLayout is the new image layout
        *   @param p_subresourceRange is the subresource set for the image conversion
        *   @param p_srcStageMask is the mask used by the acutal image
        *   @param p_dstStageMask is the mask that will be used by the acutal image
        */
        static void SetImageLayout(VkCommandBuffer p_cmdbuffer, VkImage p_image, VkImageLayout p_oldImageLayout, VkImageLayout p_newImageLayout, VkImageSubresourceRange p_subresourceRange);

        /**
        *   @brief Set the new image layout with an aspect mask
        *   @param p_cmdBuffer is the commandbuffer to record the command in
        *   @param p_image is the image on wich we change the layout
        *   @param p_aspectMask is the image mask
        *   @param p_oldImageLayout is the actual image layout
        *   @param p_newImageLayout is the new image layout
        *   @param p_subresourceRange is the subresource set for the image conversion
        *   @param p_srcStageMask is the mask used by the acutal image
        *   @param p_dstStageMask is the mask that will be used by the acutal image
        */
        static void SetImageLayout(VkCommandBuffer p_cmdbuffer, VkImage p_image, VkImageAspectFlags p_aspectMask, VkImageLayout p_oldImageLayout, VkImageLayout p_newImageLayout, VkPipelineStageFlags p_srcStageMask, VkPipelineStageFlags p_dstStageMask);

        /**
        *   @brief Get the depth format supported by the gpu
        *   @param p_physicalDevice is the gpu to get the format from
        *   @param p_depthFormat is the depth format returned after getting the gpu available format
        */
        static VkBool32 GetSupportedDepthFormat(VkPhysicalDevice p_physicalDevice, VkFormat* p_depthFormat);

        /**
        *   @brief Create a commandbuffer to be used to record commands
        *   @param p_level is the priority level of the commandbuffer
        *   @param p_begin tells is the commandbuffer starts recording or not
        */
        VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel p_level, bool p_begin) const;

        /**
        *   @brief returns the memory type used by the gpu
        *   @param p_typeBits is the memory type bits coming from the memory requierements
        *   @param p_properties are the memory properties
        *   @param p_memTypeFound is a check returning if the memory type is found
        *   @returns the type found by the function
        */
        uint32_t GetMemoryType(uint32_t p_typeBits, VkMemoryPropertyFlags p_properties, VkBool32* p_memTypeFound = nullptr) const;

        /**
        *   @brief loads a shader from a file and returns its stage creation info
        *   @param p_fileName is the file path to the shader
        *   @param p_stage is the shader stage flag
        *   @returns the newly setup shader stage info
        */
        VkPipelineShaderStageCreateInfo LoadShader(std::string p_fileName, VkShaderStageFlagBits p_stage);

        /**
        *   @brief Creates the shader identifier that will be used by the shader binding table
        *   @param p_data is the data pointer to the shaderbinding table buffer
        *   @param p_shaderHandleStorage is the storage handle for the shader
        *   @param p_groupIndex is the shader category group index
        *   @returns the shader group handle size
        */
        VkDeviceSize CopyShaderIdentifier(uint8_t* p_data, const uint8_t* p_shaderHandleStorage, uint8_t p_groupIndex) const;

        /**
        *   @brief Gets a new, ready image from the swapchain and prepare it to be rendered
        *   @param p_presentCompleteSemaphore is the semaphore that validates and synchronizes the acquire with the rendering state
        *   @param p_imageIndex it the image index from the swapchain returned
        *   @returns function validation result
        */
        VkResult AcquireNextImage(uint32_t* p_imageIndex) const;

        /**
        *   @brief diplay the selected image on screen
        *   @param p_queue is the queue on wich we want to present the image
        *   @param p_imageIndex is the image index from the swap chain acquired previously
        *   @param p_waitSemaphore is the synchronization smeaphore with the gpu
        *   @returns function validation result
        */
        VkResult QueuePresent(VkQueue p_queue, uint32_t p_imageIndex, VkSemaphore p_waitSemaphore);

        /**
        *   @brief Creates a buffer with a size and its data
        *   @param p_usageFlags is the flag defining how the buffer will be used as
        *   @param p_memoryPropertyFlags are the memory propertie flags
        *   @param p_buffer is the actual buffer that we will fill
        *   @param p_size is the size that the buffer will take
        *   @param p_data is the actual data that we will put in the buffer
        *   @returns function validation result
        */
        VkResult CreateBuffer(VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memoryPropertyFlags, Buffer* p_buffer, VkDeviceSize p_size, void* p_data = nullptr) const;
        
        uint32_t GetAlignedSize(uint32_t value, uint32_t alignment);

        int FindObjectID(uint64_t p_id);

        int CheckForExistingMesh(Mesh* p_mesh);

#pragma endregion

#pragma region Pipeline Core Variables
        GLFWwindow* m_window{};
        Device m_vulkanDevice{};

        SwapChain m_swapChain;
        ORenderPass m_mainRenderPass;
        DepthStencil m_depthStencil{};
        StorageImage m_storageImage{};
        GameViewProperties m_gameViewProps{};
        UniformData m_cameraData{};
        ShaderData m_shaderData{};
        ImTextureID m_sceneID;
        uint32_t m_currentFrame{ 0 };
        uint32_t m_width{ 0 };
        uint32_t m_height{ 0 };
        uint32_t m_minImageCount{ 0 };
        uint32_t m_sceneReswidth{ 1920 };
        uint32_t m_sceneResheight{ 1080 };
        bool isRefreshing{ true };
#pragma endregion

#pragma region Pipeline Buffers
        Buffer m_shaderBindingTable;
        Buffer m_cameraBuffer;
        Buffer m_instancesBuffer;
#pragma endregion

#pragma region Pipeline Generic Variables
        Camera m_camera{};

        std::vector<VkShaderModule> m_shaderModules;
        std::vector<VkCommandBuffer> m_commandBuffers;
        std::vector<VkCommandBuffer> m_ImGUIcommandBuffers;
        std::vector<VkFramebuffer> m_ImGUIframeBuffers;
        std::vector<uint32_t> m_objectBlasIDs;
        std::vector<AccelerationStructure> m_BLAS;
        std::vector<std::shared_ptr<Mesh>> m_BLASmeshes;
        std::vector<Model> m_objects;
        std::vector<uint64_t> m_objectIDs;
        std::vector<RTMaterial> m_materials;

        uint32_t maxInstances;
        AccelerationStructure m_topLevelAcceleration;

        std::vector<uint32_t> m_textureIDs;
        std::vector<TextureData> m_textures;
        std::vector<std::string> m_textureCtr;

        std::vector<uint32_t> m_normalMapIDs;
        std::vector<TextureData> m_normalMaps;
        std::vector<std::string> m_normalMapsCtr;

        std::vector<std::pair<Mesh*, int>> m_instanceTracker;

        std::vector<RTLight> m_lights;
        std::vector<uint64_t> m_lightsIDs;

#pragma endregion

#pragma region Vulkan Direct Variables

        VkQueue m_graphicsQueue{};
        VkQueue m_presentQueue{};
        VkCommandPool m_commandPool{};
        VkCommandPool m_ImGUIcommandPool{};
        VkRenderPass m_ImGUIrenderPass{};
        VkPipeline m_pipeline{};
        VkPipelineLayout m_pipelineLayout{};
        VkDescriptorSet m_descriptorSet{};
        VkDescriptorSetLayout m_descriptorSetLayout{};
        std::vector<VkWriteDescriptorSet> m_writeDescriptorSets;
        std::vector<VkDescriptorSetLayoutBinding> m_descriptorSetBindings;
        //ImGui
        VkDescriptorSet m_UIDescriptorSet{};
        VkDescriptorPool m_UIDescriptorPool{};
        VkDescriptorSetLayout m_UIDescriptorLayout{};
        VkPipelineLayout m_UIPipelineLayout{};
        TextureData m_UIimage{};

        VkDescriptorPool m_descriptorPool{};
        VkDescriptorPool m_ImGUIdescriptorPool{};

        VkFormat m_depthFormat{};
        VkPhysicalDeviceRayTracingPropertiesNV m_raytracingProperties{};
        VkPipelineCache m_pipelineCache{};
        VkSubmitInfo m_submitInfo{};
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight;

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
