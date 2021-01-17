#pragma once
#include <OgRendering/Export.h>

#include <vector>
#include <unordered_map>

#include <OgRendering/UI/imgui/imgui.h>
#include <OgRendering/Utils/VulkanTools.h>
#include <OgRendering/Rendering/SwapChainSupportDetails.h>
#include <OgRendering/Resource/Vertex.h>
#include <OgRendering/Rendering/Device.h>
#include <OgRendering/Resource/Mesh.h>
#include <OgRendering/Resource/ObjectInstance.h>
#include <OgRendering/Resource/Texture.h>
#include <OgRendering/Resource/TextureData.h>
#include <OgRendering/Resource/Camera.h>
#include <OgRendering/Utils/vk_mem_alloc.h>


#define MAX_TEXTURES_RS 64
#define MAX_OBJECTS_RS 500
#define MAX_FRAMES_IN_FLIGHT 2

namespace OgEngine
{
	
	struct FrameBufferAttachment
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	};

	struct OffscreenPass
	{
		int32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment color, depth;
		VkRenderPass renderPass;
		VkSampler sampler;
		VkDescriptorImageInfo descriptor;
	};

	struct Semaphore {
		// Swap chain image presentation
		VkSemaphore presentComplete;
		// Command buffer submission and execution
		VkSemaphore renderComplete;
	};

	class RENDERING_API RasterizerPipeline final
	{
	public:
		/**
		 * @brief Constructor
		 * @param p_window The GLFW window
		 * @param p_device Wrapper of vulkan device
		 * @param p_graphicsQueue Graphics queue of the gpu
		 * @param p_presentQueue Present queue of the gpu
		 * @param p_width Width of the window
		 * @param p_height Height of the window
		 */
		RasterizerPipeline(GLFWwindow* p_window, Device& p_device, VkQueue& p_graphicsQueue, VkQueue& p_presentQueue, const uint32_t p_width, const uint32_t p_height);

		/**
		 * @brief Setup the pipeline of the rasterizer
		 */
		void SetupPipeline();

		/**
		* @brief Clean all the resources of the pipeline
		*/
		void CleanPipeline();

		/**
		 * @brief Update the internal resources
		 * @param p_dt The time elapsed between two frames
		 * @param p_objectID The entity we want to update
		 * @param p_modelTransform The transform matrix of a mesh
		 * @param p_mesh The mesh to update
		 */
		void Update(const float p_dt, const std::uint64_t p_objectID, const glm::mat4& p_modelTransform, Mesh* p_mesh, const std::string& p_texture, const std::string& p_normalMap, const glm::vec4& p_color);

		/**
		 * @brief Draw the frame to the screen
		 */
		void RenderFrame();

		/**
		 * @brief Return the GUI context
		 * @return A pointer to ImGuiContext
		 */
		ImGuiContext* GetUIContext();

		/**
		 * @brief Prepare a frame for ImGui
		 */
		void PrepareIMGUIFrame();

		/**
		 * @brief Draw the UI (internally used in RenderFrame)
		 */
		void DrawUI();

		/**
		 * @brief Submit the editor layout to an ImGuiFrame
		 */
		void DrawEditor();

		/**
		 * @brief Destroy a mesh reference of the rendering using object ID (Entity of ECS)
		 * @param p_objectID The object ID
		 */
		void DestroyObject(const std::uint64_t p_objectID);

		/**
		 * @brief Destroy all mesh reference of the rendering
		 */
		void CleanAllObjectInstance();

		/**
		* @brief Loads an image from path in an understandable format for ImGui
		* @param p_texturePath is the path of the texture to load
		*/
        ImTextureID AddUITexture(const char* p_texturePath);

		/**
		* @brief Add a texture handle in the renderer using the path of the texture.
		* @param p_texture The name of the texture to add
		* @param p_textureType The type of the texture. Albedo (aka basic texture) map or normal map
		*/
		void CreateTexture(const std::string& p_texture, const TEXTURE_TYPE p_textureType);

		/**
		* @brief Add a texture handle in the renderer using the handle of the resource manager
		* @param p_textureAddr is the handle of the texture to load from the resource manager
		* @param p_textureType The type of the texture. Albedo (aka basic texture) map or normal map
		*/
		void CreateTexture(Texture* p_textureAddr, const TEXTURE_TYPE p_textureType);

		/**
		* @brief Add a texture handle in the renderer using the handle of the resource manager
		* @param p_position is the handle of the texture to load from the resource manager
		* @param p_rotation The type of the texture. Albedo (aka basic texture) map or normal map
		*/
		void UpdateCamera(const glm::vec3& p_position, const glm::vec3& p_rotation);

		/**
		* @brief Return the current camera
		*/
		Camera& GetCurrentCamera();
	private:
#pragma region Helpers
		/// <summary>
		/// Initialize VMA allocator
		/// </summary>
		void InitializeVMA();

		/**
		 * @brief Return all the swap chain capabilities for a gpu
		 * @param p_gpu The gpu device to check on
		 * @return The swapchain support details
		 */
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice& p_gpu) const;

		/**
		 * @brief Find the possible type of family for a gpu
		 * @param p_physicalDevice The gpu device to check on
		 */
		void FindQueueFamilies(VkPhysicalDevice p_physicalDevice);
		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<struct VkSurfaceFormatKHR>& p_availableFormats);
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& p_availablePresentModes);
		[[nodiscard]] VkFormat FindDepthFormat() const;
		[[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat>& p_candidates, VkImageTiling p_tiling, VkFormatFeatureFlags p_features) const;
		[[nodiscard]] VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& p_capabilities) const;
		[[nodiscard]] VkImageView CreateImageView(VkImage p_image, VkFormat p_format, VkImageAspectFlags p_aspectFlags, uint32_t p_mipLevels) const;
		[[nodiscard]] uint32_t FindMemoryType(uint32_t p_typeFilter, VkMemoryPropertyFlags p_properties) const;
		[[nodiscard]] VkCommandBuffer BeginSingleTimeCommands() const;
		void EndSingleTimeCommands(VkCommandBuffer p_commandBuffer) const;
		void CreateImage(uint32_t p_width, uint32_t p_height, uint32_t p_mipLevels, VkSampleCountFlagBits p_numSamples, VkFormat p_format, VkImageTiling p_tiling, VkImageUsageFlags p_usage, VkMemoryPropertyFlags p_properties, VkImage& p_image, VkDeviceMemory& p_imageMemory) const;
		void CopyBuffer(VkBuffer p_srcBuffer, VkBuffer p_dstBuffer, VkDeviceSize p_size) const;
		void CopyBufferToImage(const VkBuffer p_buffer, const VkImage p_image, const uint32_t p_width, const uint32_t p_height) const;
		void GenerateMipmaps(TextureData& p_textureData, VkFormat p_imageFormat, int32_t p_texWidth, int32_t p_texHeight, uint32_t p_mipLevels) const;
		void CreateBuffer(const VkDeviceSize p_size, const VkBufferUsageFlags p_usage, VkMemoryPropertyFlags p_properties, VkBuffer& p_buffer, VkDeviceMemory& p_bufferMemory, const size_t p_dynamicOffset = 0) const;
		VkResult CreateBuffer(VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memoryPropertyFlags, Buffer* p_buffer, VkDeviceSize p_size, void* p_data = nullptr) const;
		void TransitionImageLayout(const VkImage p_image, VkFormat p_format, const VkImageLayout p_oldLayout, const VkImageLayout p_newLayout, const uint32_t p_mipLevels) const;
		static void CHECK_ERROR(VkResult p_result);
		void SetupOffScreenPass();
		void CopyImage(VkImage p_source);

		void InitFrame();
		void DisplayFrame();
		VkResult QueuePresent(VkQueue p_queue, uint32_t p_imageIndex, VkSemaphore p_waitSemaphore = nullptr) const;

		//void HandleSurfaceChanges();
#pragma endregion

#pragma region PipelineMethods
		void CreateSwapChain();
		void CreateImageViews();
		void CreateRenderPass();
		void CreateGraphicsPipeline();
		void CreateDescriptorSetLayout();
		void CreateCommandPool();
		void CreateColorResources();
		void CreateDepthResources();
		void CreateFramebuffers();

		[[nodiscard]] VkDescriptorImageInfo Create2DDescriptor(const VkImage& p_image, const VkSamplerCreateInfo& p_samplerCreateInfo, const VkFormat& p_format, const VkImageLayout& p_layout) const;
		void CreatePipelineCache();

		void CreateVertexBuffer(Mesh* p_mesh, Buffer* p_objectInstance) const;
		void CreateIndexBuffer(Mesh* p_mesh, Buffer* p_objectInstance) const;
		void AllocateBufferArray(ObjectInstance& p_objectInstance) const;
		void AllocateDescriptorSet(ObjectInstance& p_objectInstance) const;
		void BindDescriptorSet(ObjectInstance& p_objectInstance) const;
		void UpdateUniformBuffer(ObjectInstance& p_objectInstance) const;
		void DestroyObjectInstance(ObjectInstance& p_objectInstance) const;

		void CreateTextureImage(TextureData& p_textureData, const Texture* p_loadedTexture) const;
		void CreateTextureImageView(TextureData& p_textureData) const;
		void CreateTextureSampler(TextureData& p_textureData);
		
		void CreateDescriptorPool();
		void CreateCommandBuffers();
		void CreateSynchronizedObjects();

		// Cleanup
		void CleanupSwapChain();
		void RecreateSwapChain();

		void InitImGUI();
		void SetupImGUIStyle();
		void SetupImGUI();
		void SetupImGUIFrameBuffers();
		void RescaleImGUI();
		void RenderUI(uint32_t p_id);
		void FreeImGUIContext();
		
#pragma endregion

		//Referenced from Context
		GLFWwindow* m_window;
		Device m_device;
		VkQueue m_graphicsQueue{};
		VkQueue m_presentQueue{};
		VmaAllocator m_allocator;

		//Swap Chain
		VkFormat m_chainColorFormat;
		VkExtent2D m_chainExtent{};
		VkSwapchainKHR m_chain{};
		std::vector<VkImage> m_images;
		std::vector<VkImageView> m_imageViews;
		std::vector<VkFramebuffer> m_chainFrameBuffers;

		VkRenderPass m_renderPass{};
		VkDescriptorSetLayout m_descriptorSetLayout{};
		VkPipelineLayout m_pipelineLayout{};
		VkPipeline m_graphicsPipeline{};

		// ImGUI
#pragma region IMGUIMembers
		VkPipelineCache m_pipelineCache{};
		VkRenderPass m_ImGUIrenderPass{};
		VkCommandPool m_ImGUIcommandPool{};
		VkDescriptorPool m_ImGUIdescriptorPool{};
		std::vector<VkCommandBuffer> m_ImGUIcommandBuffers;
		std::vector<VkFramebuffer> m_ImGUIframeBuffers;
#pragma endregion

		VkDescriptorPool m_descriptorPool{};

		VkCommandPool m_commandPool{};
		std::vector<VkCommandBuffer> m_commandBuffers;


		VkImage m_colorImage{};
		VkDeviceMemory m_colorImageMemory{};
		VkImageView m_colorImageView{};

		VkImage m_depthImage{};
		VkDeviceMemory m_depthImageMemory{};
		VkImageView m_depthImageView{};

		// TODO : pass those images to a map to store multiple images
		// map using objectID as key and the handling of the Image (texture) buffers
		std::unordered_map<Texture*, TextureData> m_textures;
		uint32_t m_mipLevels{};
		VkImage m_textureImage{};
		VkDeviceMemory m_textureImageMemory{};
		VkImageView m_textureImageView{};
		VkSampler m_textureSampler{};
		
		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;
		std::vector<VkFence> m_imagesInFlight;

		uint32_t m_currentFrame{ 0u };
		uint32_t m_imageIndex{ 0u };

		OffscreenPass m_offScreenPass;

		std::unordered_map<std::uint64_t, ObjectInstance> m_buffers;
		std::unordered_map<Mesh*, std::pair<Buffer, Buffer>> m_meshesBuffers;
		//Window size
		uint32_t m_width{ 0u };
		uint32_t m_height{ 0u };
		uint32_t m_minImageCount{ 0u };
		Camera m_camera;

		bool m_prepared = false;

	public:
		ImTextureID m_sceneID;

	};
}
