#pragma once

#include <OgRendering/Export.h>
#include <OgRendering/Utils/VulkanTools.h>
#include <OgRendering/Rendering/SwapChainSupportDetails.h>
#include <OgRendering/Resource/Vertex.h>
#include <OgRendering/Rendering/Device.h>
#include <OgRendering/Resource/Mesh.h>
#include <vector>
#include <map>


namespace OgEngine
{
	const uint32_t MAX_FRAMES_IN_FLIGHT = 2u;

	struct RENDERING_API BufferArrayOfMesh
	{
		Buffer vertexBuffer{};
		Buffer indexBuffer{};
		GPM::Matrix4F model{ Matrix4F::identity };

		BufferArrayOfMesh()
			= default;
	};

	class RENDERING_API RasterizerPipeline final
	{
	public:
		RasterizerPipeline(GLFWwindow* p_window, Device& p_device, VkQueue& p_graphicsQueue, VkQueue& p_presentQueue, const uint32_t p_width, const uint32_t p_height);

		void SetupPipeline();
		void CleanPipeline();
		void Update(const float p_dt, const GPM::Matrix4F& p_modelTransform, const std::shared_ptr<Mesh>& p_mesh);
		void DrawFrame();

	private:
#pragma region Helpers
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice& p_gpu) const;
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
		void GenerateMipmaps(VkImage p_image, VkFormat p_imageFormat, int32_t p_texWidth, int32_t p_texHeight, uint32_t p_mipLevels) const;
		void CreateBuffer(const VkDeviceSize p_size, const VkBufferUsageFlags p_usage, VkMemoryPropertyFlags p_properties, VkBuffer& p_buffer, VkDeviceMemory& p_bufferMemory, const size_t p_dynamicOffset = 0) const;
		VkResult CreateBuffer(VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memoryPropertyFlags, Buffer* p_buffer, VkDeviceSize p_size, void* p_data = nullptr) const;
		void TransitionImageLayout(const VkImage p_image, VkFormat p_format, const VkImageLayout p_oldLayout, const VkImageLayout p_newLayout, const uint32_t p_mipLevels) const;
		void UpdateUniformBuffer(const uint32_t p_currentImage, const Matrix4F& p_modelMatrix);
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
		void CreateTextureImage();
		void CreateTextureImageView();
		void CreateTextureSampler();
		VkDescriptorImageInfo Create2DDescriptor(const VkImage& p_image, const VkSamplerCreateInfo& p_samplerCreateInfo, const VkFormat& p_format, const VkImageLayout& p_layout) const;

		void LoadModel();
		void CreateVertexBuffer(const std::shared_ptr<Mesh>& p_mesh);
		void CreateIndexBuffer(const std::shared_ptr<Mesh>& p_mesh);
		void CreateUniformBuffers();

		void CreateDescriptorPool();
		void CreateDescriptorSets();
		void CreateCommandBuffers();
		void CreateSynchronizedObjects();
		// Cleanup
		void CleanupSwapChain();
		void RecreateSwapChain();
#pragma endregion

		//Referenced from Context
		GLFWwindow* m_window;
		Device m_vulkanDevice;
		VkQueue m_graphicsQueue{};
		VkQueue m_presentQueue{};

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

		VkDescriptorPool m_descriptorPool{};
		std::vector<VkDescriptorSet> m_descriptorSets;

		VkCommandPool m_commandPool{};
		std::vector<VkCommandBuffer> m_commandBuffers;

		VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_4_BIT;

		VkBuffer m_vertexBuffer{};
		VkDeviceMemory m_vertexBufferMemory{};
		VkBuffer m_indexBuffer{};
		VkDeviceMemory m_indexBufferMemory{};

		VkImage m_colorImage{};
		VkDeviceMemory m_colorImageMemory{};
		VkImageView m_colorImageView{};

		VkImage m_depthImage{};
		VkDeviceMemory m_depthImageMemory{};
		VkImageView m_depthImageView{};

		uint32_t m_mipLevels{};
		VkImage m_textureImage{};
		VkDeviceMemory m_textureImageMemory{};
		VkImageView m_textureImageView{};
		VkSampler m_textureSampler{};

		std::vector<VkBuffer> m_uniformBuffers;
		std::vector<VkDeviceMemory> m_uniformBuffersMemory;

		std::vector<VkBuffer> m_lightsBuffers;
		std::vector<VkDeviceMemory> m_lightsBuffersMemory;

		std::vector<VkBuffer> m_materialsBuffers;
		std::vector<VkDeviceMemory> m_materialsBuffersMemory;

		std::vector<VkBuffer> m_lightNumberBuffers;
		std::vector<VkDeviceMemory> m_lightNumberBuffersMemory;

		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;
		std::vector<VkFence> m_imagesInFlight;

		std::array<uint64_t, 1> m_pushConstants;

		size_t m_currentFrame{ 0u };

		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;

		std::map<std::shared_ptr<Mesh>, BufferArrayOfMesh> m_buffers;
		//Window size
		uint32_t m_width{ 0u };
		uint32_t m_height{ 0u };
	};
}
