#include <OgRendering/Rendering/RasterizerPipeline.h>
#include <OgRendering/Managers/Loaders/ShaderLoader.h>
#include <chrono>
#include <OgRendering/Managers/ResourceManager.h>
#include "OgRendering/Rendering/VulkanContext.h"
#include <OgRendering/Utils/Debugger.h>

OgEngine::RasterizerPipeline::RasterizerPipeline(GLFWwindow* p_window, Device& p_vulkanDevice,
	VkQueue& p_graphicQueue, VkQueue& p_presentQueue,
	const uint32_t p_width, const uint32_t  p_height)
	: m_window(p_window), m_vulkanDevice(p_vulkanDevice), m_graphicsQueue(p_graphicQueue),
	m_presentQueue(p_presentQueue), m_width(p_width), m_height(p_height)
{
}

void OgEngine::RasterizerPipeline::SetupPipeline()
{
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateCommandPool();

	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
	LoadModel();
	//CreateVertexBuffer();
	//CreateIndexBuffer();
	CreateUniformBuffers();

	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
	CreateSynchronizedObjects();

	InitImGUI();
	SetupImGUIFrameBuffers();
	SetupImGUI();
	std::cout << "imgui init successful";
}

void OgEngine::RasterizerPipeline::CleanPipeline()
{
	CleanupSwapChain();

	vkDestroySampler(m_vulkanDevice.m_logicalDevice, m_textureSampler, nullptr);
	vkDestroyImageView(m_vulkanDevice.m_logicalDevice, m_textureImageView, nullptr);

	vkDestroyImage(m_vulkanDevice.m_logicalDevice, m_textureImage, nullptr);
	vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_textureImageMemory, nullptr);

	vkDestroyDescriptorSetLayout(m_vulkanDevice.m_logicalDevice, m_descriptorSetLayout, nullptr);

	vkDestroyBuffer(m_vulkanDevice.m_logicalDevice, m_indexBuffer, nullptr);
	vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_indexBufferMemory, nullptr);

	vkDestroyBuffer(m_vulkanDevice.m_logicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_vertexBufferMemory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_vulkanDevice.m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_vulkanDevice.m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_vulkanDevice.m_logicalDevice, m_inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_vulkanDevice.m_logicalDevice, m_commandPool, nullptr);
}

void OgEngine::RasterizerPipeline::Update(
	const float                  p_dt,
	const GPM::Matrix4F& p_modelTransform,
	const std::shared_ptr<Mesh>& p_mesh)
{
	// Update les buffer
	const auto iterator = m_buffers.find(p_mesh);
	if (iterator == m_buffers.end())
	{
		m_buffers.insert(std::make_pair(p_mesh, BufferArrayOfMesh{}));
		CreateVertexBuffer(p_mesh);
		CreateIndexBuffer(p_mesh);
		//CreateTextureImage();
		//CreateTextureImageView();
		//CreateTextureSampler();
		//CreateUniformBuffers();
		//CreateDescriptorPool();
		//CreateDescriptorSets();
		CreateCommandBuffers();
		//CreateSynchronizedObjects();
		m_buffers.find(p_mesh)->second.model = p_modelTransform;
		UpdateUniformBuffer(m_currentFrame, p_modelTransform);
	}
	else
	{
		iterator->second.model = p_modelTransform;
		UpdateUniformBuffer(m_currentFrame, p_modelTransform);
	}
}

void OgEngine::RasterizerPipeline::DrawFrame()
{
	vkWaitForFences(m_vulkanDevice.m_logicalDevice, 1u, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_vulkanDevice.m_logicalDevice, m_chain, UINT64_MAX,
		m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	//UpdateUniformBuffer(imageIndex, Matrix4F::identity);

	if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(m_vulkanDevice.m_logicalDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore          waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1u;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1u;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_vulkanDevice.m_logicalDevice, 1u, &m_inFlightFences[m_currentFrame]);

	if (vkQueueSubmit(m_graphicsQueue, 1u, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	RenderUI(imageIndex);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1u;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_chain };
	presentInfo.swapchainCount = 1u;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || VulkanContext::framebufferResized)
	{
		VulkanContext::framebufferResized = false;
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	m_currentFrame = (m_currentFrame + 1u) % MAX_FRAMES_IN_FLIGHT;
}

#pragma region Helpers
OgEngine::SwapChainSupportDetails OgEngine::RasterizerPipeline::QuerySwapChainSupport(VkPhysicalDevice& p_gpu) const
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_gpu, m_vulkanDevice.m_surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_gpu, m_vulkanDevice.m_surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(p_gpu, m_vulkanDevice.m_surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_gpu, m_vulkanDevice.m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(p_gpu, m_vulkanDevice.m_surface, &presentModeCount,
			details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR OgEngine::RasterizerPipeline::ChooseSwapSurfaceFormat(
	const std::vector<struct VkSurfaceFormatKHR>& p_availableFormats)
{
	for (const auto& availableFormat : p_availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace ==
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return p_availableFormats[0];
}

VkPresentModeKHR OgEngine::RasterizerPipeline::ChooseSwapPresentMode(
	const std::vector<VkPresentModeKHR>& p_availablePresentModes)
{
	for (const auto& availablePresentMode : p_availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D OgEngine::RasterizerPipeline::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& p_capabilities) const
{
	if (p_capabilities.currentExtent.width != UINT32_MAX)
	{
		return p_capabilities.currentExtent;
	}
	else
	{
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(p_capabilities.minImageExtent.width,
			std::min(p_capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(p_capabilities.minImageExtent.height,
			std::min(p_capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VkImageView OgEngine::RasterizerPipeline::CreateImageView(VkImage            p_image, VkFormat       p_format,
	VkImageAspectFlags p_aspectFlags, uint32_t p_mipLevels) const
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = p_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = p_format;
	viewInfo.subresourceRange.aspectMask = p_aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = p_mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(m_vulkanDevice.m_logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

VkFormat OgEngine::RasterizerPipeline::FindDepthFormat() const
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat OgEngine::RasterizerPipeline::FindSupportedFormat(const std::vector<VkFormat>& p_candidates,
	VkImageTiling                p_tiling,
	VkFormatFeatureFlags         p_features) const
{
	for (VkFormat format : p_candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_vulkanDevice.m_gpu, format, &props);

		if (p_tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & p_features) == p_features)
			return format;
		if (p_tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & p_features) == p_features)
			return format;
	}

	throw std::runtime_error("failed to find supported format!");
}

void OgEngine::RasterizerPipeline::FindQueueFamilies(VkPhysicalDevice p_physicalDevice)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(p_physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(p_physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_vulkanDevice.m_gpuGraphicFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(p_physicalDevice, i, m_vulkanDevice.m_surface, &presentSupport);

		if (presentSupport)
		{
			m_vulkanDevice.m_gpuPresentFamily = i;
		}

		if (m_vulkanDevice.m_gpuGraphicFamily.has_value() && m_vulkanDevice.m_gpuPresentFamily.has_value())
		{
			break;
		}

		i++;
	}
}

void OgEngine::RasterizerPipeline::CreateImage(uint32_t              p_width, uint32_t p_height,
	uint32_t              p_mipLevels,
	VkSampleCountFlagBits p_numSamples, VkFormat      p_format,
	VkImageTiling         p_tiling, VkImageUsageFlags p_usage,
	VkMemoryPropertyFlags p_properties, VkImage& p_image,
	VkDeviceMemory& p_imageMemory) const
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = p_width;
	imageInfo.extent.height = p_height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = p_mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = p_format;
	imageInfo.tiling = p_tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = p_usage;
	imageInfo.samples = p_numSamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(m_vulkanDevice.m_logicalDevice, &imageInfo, nullptr, &p_image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_vulkanDevice.m_logicalDevice, p_image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, p_properties);

	if (vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &allocInfo, nullptr, &p_imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(m_vulkanDevice.m_logicalDevice, p_image, p_imageMemory, 0);
}

uint32_t OgEngine::RasterizerPipeline::FindMemoryType(const uint32_t              p_typeFilter,
	const VkMemoryPropertyFlags p_properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_vulkanDevice.m_gpu, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((p_typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & p_properties) == p_properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VkCommandBuffer OgEngine::RasterizerPipeline::BeginSingleTimeCommands() const
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_vulkanDevice.m_logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void OgEngine::RasterizerPipeline::EndSingleTimeCommands(VkCommandBuffer p_commandBuffer) const
{
	vkEndCommandBuffer(p_commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &p_commandBuffer;
	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = Initializers::fenceCreateInfo(0);
	VkFence VIVELAFRANCE;
	CHECK_ERROR(vkCreateFence(m_vulkanDevice.m_logicalDevice, &fenceInfo, nullptr, &VIVELAFRANCE));
	CHECK_ERROR(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VIVELAFRANCE));
	CHECK_ERROR(vkWaitForFences(m_vulkanDevice.m_logicalDevice, 1, &VIVELAFRANCE, VK_TRUE, 100000000000));
	vkQueueWaitIdle(m_graphicsQueue);
	vkDestroyFence(m_vulkanDevice.m_logicalDevice, VIVELAFRANCE, nullptr);

	vkFreeCommandBuffers(m_vulkanDevice.m_logicalDevice, m_commandPool, 1, &p_commandBuffer);
}

void OgEngine::RasterizerPipeline::CopyBuffer(const VkBuffer     p_srcBuffer, const VkBuffer p_dstBuffer,
	const VkDeviceSize p_size) const
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = p_size;
	vkCmdCopyBuffer(commandBuffer, p_srcBuffer, p_dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void OgEngine::RasterizerPipeline::CopyBufferToImage(const VkBuffer p_buffer, const VkImage p_image,
	const uint32_t p_width, const uint32_t p_height) const
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		p_width,
		p_height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, p_buffer, p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(commandBuffer);
}

void OgEngine::RasterizerPipeline::CreateBuffer(const VkDeviceSize    p_size, const VkBufferUsageFlags p_usage,
	VkMemoryPropertyFlags p_properties, VkBuffer& p_buffer,
	VkDeviceMemory& p_bufferMemory, const size_t p_dynamicOffset) const
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = p_size;
	bufferInfo.usage = p_usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_vulkanDevice.m_logicalDevice, &bufferInfo, nullptr, &p_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_vulkanDevice.m_logicalDevice, p_buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, p_properties);

	if (vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &allocInfo, nullptr, &p_bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(m_vulkanDevice.m_logicalDevice, p_buffer, p_bufferMemory, p_dynamicOffset);
}

VkResult OgEngine::RasterizerPipeline::CreateBuffer(VkBufferUsageFlags    p_usageFlags,
	VkMemoryPropertyFlags p_memoryPropertyFlags, Buffer* p_buffer,
	VkDeviceSize          p_size, void* p_data) const
{
	p_buffer->device = m_vulkanDevice.m_logicalDevice;

	// Create the p_buffer handle
	VkBufferCreateInfo bufferCreateInfo = Initializers::bufferCreateInfo(p_usageFlags, p_size);
	vkCreateBuffer(m_vulkanDevice.m_logicalDevice, &bufferCreateInfo, nullptr, &p_buffer->buffer);

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memory_requirements;
	VkMemoryAllocateInfo memAlloc = Initializers::memoryAllocateInfo();
	vkGetBufferMemoryRequirements(m_vulkanDevice.m_logicalDevice, p_buffer->buffer, &memory_requirements);
	memAlloc.allocationSize = memory_requirements.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, p_memoryPropertyFlags);
	vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &memAlloc, nullptr, &p_buffer->memory);

	p_buffer->alignment = memory_requirements.alignment;
	p_buffer->size = memAlloc.allocationSize;
	p_buffer->usageFlags = p_usageFlags;
	p_buffer->memoryPropertyFlags = p_memoryPropertyFlags;

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (p_data != nullptr)
	{
		p_buffer->map();
		memcpy_s(p_buffer->mapped, p_size, p_data, p_size);
		if ((p_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			DBG_ASSERT_VULKAN_MSG(p_buffer->flush(), "Cannot create vertex buffer");

		p_buffer->unmap();
	}

	// Initialize a default descriptor that covers the whole buffer size
	p_buffer->setupDescriptor();

	// Attach the memory to the buffer object
	return p_buffer->bind();
}

void OgEngine::RasterizerPipeline::TransitionImageLayout(const VkImage       p_image, VkFormat p_format,
	const VkImageLayout p_oldLayout,
	const VkImageLayout p_newLayout,
	const uint32_t      p_mipLevels) const
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = p_oldLayout;
	barrier.newLayout = p_newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = p_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = p_mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (p_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && p_newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (p_oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && p_newLayout ==
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(commandBuffer);
}

void OgEngine::RasterizerPipeline::UpdateUniformBuffer(const uint32_t p_currentImage, const Matrix4F& p_modelMatrix)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	const auto  currentTime = std::chrono::high_resolution_clock::now();
	const float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	ubo.model = p_modelMatrix;
	//ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.model = GPM::Matrix4F::Rotate(GPM::Matrix4F::identity, time * Tools::Utils::ToRadians(90.0f),
	//	GPM::Vector3F(0.0f, 2.0f, 0.0f));
	//ubo.model *= Matrix4F::CreateTranslation(Vector3F(0.0f, -20.0f, 7.f)).Transpose();
	//ubo.model *= Matrix4F::CreateScale(Vector3F(0.06f, 0.06f, 0.06f));
	// At the end, transform will be stocked in the m_buffers and we will use the according transform
	//ubo.view = glm::lookAt(glm::vec3(1.5f, 1.5f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	//ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
	ubo.view = GPM::Matrix4F::LookAt(GPM::Vector3F(0.0f, 1.0f, -4.0), GPM::Vector3F(0.0f, 0.0f, 0.0f),
		GPM::Vector3F(0.0f, 1.0f, 0.0f));
	ubo.proj = GPM::Matrix4F::Perspective((45.0f), m_chainExtent.width / static_cast<float>(m_chainExtent.height), 0.1f,
		1000.0f);

	void* data;
	vkMapMemory(m_vulkanDevice.m_logicalDevice, m_uniformBuffersMemory[p_currentImage], 0, sizeof(ubo), 0, &data);
	memcpy_s(data, sizeof(ubo), &ubo, sizeof(ubo));
	vkUnmapMemory(m_vulkanDevice.m_logicalDevice, m_uniformBuffersMemory[p_currentImage]);

	UniformLightInfo lightUniform = {};
	lightUniform.diffuse = GPM::Vector4F{ 1.0f, 1.0f, 1.0f, 1.0 };
	lightUniform.ambient = GPM::Vector4F{ 0.2f, 0.0f, 0.0f, 1.0 };
	lightUniform.specular = GPM::Vector4F{ 1.0f, 1.0f, 1.0f, 1.0 };
	lightUniform.position = GPM::Vector4F{ 0.0f, 0.0f, 1.0f, 1.0f };
	void* lightData;
	vkMapMemory(m_vulkanDevice.m_logicalDevice, m_lightsBuffersMemory[p_currentImage], 0, sizeof(lightUniform), 0, &lightData);
	memcpy_s(lightData, sizeof(lightUniform), &lightUniform, sizeof(lightUniform));
	vkUnmapMemory(m_vulkanDevice.m_logicalDevice, m_lightsBuffersMemory[p_currentImage]);

	unsigned int nbLight = 1;
	void* lightNumberData;
	vkMapMemory(m_vulkanDevice.m_logicalDevice, m_lightNumberBuffersMemory[p_currentImage], 0, sizeof(unsigned int), 0, &lightNumberData);
	memcpy_s(lightNumberData, sizeof(unsigned int), &nbLight, sizeof(unsigned int));
	vkUnmapMemory(m_vulkanDevice.m_logicalDevice, m_lightNumberBuffersMemory[p_currentImage]);

	UniformMaterialInfo materialUniform = {};
	materialUniform.color = GPM::Vector3F{ 0.0f, 1.0f, 0.0f };
	materialUniform.rough = 1.0f;
	materialUniform.metal = true;
	void* materialData;
	vkMapMemory(m_vulkanDevice.m_logicalDevice, m_materialsBuffersMemory[p_currentImage], 0, sizeof(materialUniform), 0, &materialData);
	memcpy_s(materialData, sizeof(materialUniform), &materialUniform, sizeof(materialUniform));
	vkUnmapMemory(m_vulkanDevice.m_logicalDevice, m_materialsBuffersMemory[p_currentImage]);
}

void OgEngine::RasterizerPipeline::GenerateMipmaps(VkImage p_image, VkFormat     p_imageFormat, int32_t p_texWidth,
	int32_t p_texHeight, uint32_t p_mipLevels) const
{
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_vulkanDevice.m_gpu, p_imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = p_image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = p_texWidth;
	int32_t mipHeight = p_texHeight;

	for (uint32_t i = 1; i < p_mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			p_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = p_mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	EndSingleTimeCommands(commandBuffer);
}
#pragma endregion

void OgEngine::RasterizerPipeline::CreateSwapChain()
{
	const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_vulkanDevice.m_gpu);

	const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	const VkPresentModeKHR   presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	const VkExtent2D         extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0u && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	m_minImageCount = imageCount;


	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_vulkanDevice.m_surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1u;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = {
		m_vulkanDevice.m_gpuPresentFamily.value(), m_vulkanDevice.m_gpuGraphicFamily.value()
	};

	if (m_vulkanDevice.m_gpuGraphicFamily != m_vulkanDevice.m_gpuPresentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(m_vulkanDevice.m_logicalDevice, &createInfo, nullptr, &m_chain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(m_vulkanDevice.m_logicalDevice, m_chain, &imageCount, nullptr);
	m_images.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vulkanDevice.m_logicalDevice, m_chain, &imageCount, m_images.data());

	m_chainColorFormat = surfaceFormat.format;
	m_chainExtent = extent;
}

void OgEngine::RasterizerPipeline::CreateImageViews()
{
	m_imageViews.resize(m_images.size());

	for (uint32_t i = 0; i < m_images.size(); ++i)
	{
		m_imageViews[i] = CreateImageView(m_images[i], m_chainColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1u);
	}
}

void OgEngine::RasterizerPipeline::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_chainColorFormat;
	colorAttachment.samples = m_msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = m_msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = m_chainColorFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0u;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1u;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2u;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1u;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0u;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0u;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	//std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1u;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1u;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_vulkanDevice.m_logicalDevice, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}


}

void OgEngine::RasterizerPipeline::CreateGraphicsPipeline()
{
	VkShaderModule vertShaderModule = ShaderLoader::LoadShader("Resources/shaders/bin/rast_vert.spv",
		m_vulkanDevice.m_logicalDevice);
	VkShaderModule fragShaderModule = ShaderLoader::LoadShader("Resources/shaders/bin/rast_frag.spv",
		m_vulkanDevice.m_logicalDevice);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1u;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_chainExtent.width);
	viewport.height = static_cast<float>(m_chainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = m_chainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1u;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1u;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = m_msaaSamples;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1u;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1u;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

	if (vkCreatePipelineLayout(m_vulkanDevice.m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2u;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0u;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_vulkanDevice.m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
		&m_graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(m_vulkanDevice.m_logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_vulkanDevice.m_logicalDevice, vertShaderModule, nullptr);

	CreatePipelineCache();
}

void OgEngine::RasterizerPipeline::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0u;
	uboLayoutBinding.descriptorCount = 1u;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1u;
	samplerLayoutBinding.descriptorCount = 1u;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding lightsLayoutBinding = {};
	lightsLayoutBinding.binding = 2u;
	lightsLayoutBinding.descriptorCount = 1;
	lightsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	lightsLayoutBinding.pImmutableSamplers = nullptr;
	lightsLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding lightNumberLayoutBinding = {};
	lightNumberLayoutBinding.binding = 3u;
	lightNumberLayoutBinding.descriptorCount = 1;
	lightNumberLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightNumberLayoutBinding.pImmutableSamplers = nullptr;
	lightNumberLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding materialsLayoutBinding = {};
	materialsLayoutBinding.binding = 4u;
	materialsLayoutBinding.descriptorCount = 1u;
	materialsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialsLayoutBinding.pImmutableSamplers = nullptr;
	materialsLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 5> bindings = { uboLayoutBinding, samplerLayoutBinding, lightsLayoutBinding, lightNumberLayoutBinding, materialsLayoutBinding };
	VkDescriptorSetLayoutCreateInfo             layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_vulkanDevice.m_logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void OgEngine::RasterizerPipeline::CreateFramebuffers()
{
	m_chainFrameBuffers.resize(m_imageViews.size());

	for (size_t i = 0; i < m_imageViews.size(); ++i)
	{
		std::array<VkImageView, 3> attachments =
		{
			m_colorImageView,
			m_depthImageView,
			m_imageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_chainExtent.width;
		framebufferInfo.height = m_chainExtent.height;
		framebufferInfo.layers = 1u;

		if (vkCreateFramebuffer(m_vulkanDevice.m_logicalDevice, &framebufferInfo, nullptr, &m_chainFrameBuffers[i]) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void OgEngine::RasterizerPipeline::CreateTextureImage()
{
	const auto texture = ResourceManager::Get<Texture>("default.png");
	if (texture != nullptr)
	{
		m_mipLevels = texture->MipmapLevels();
		VkBuffer       stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(texture->ImageSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(m_vulkanDevice.m_logicalDevice, stagingBufferMemory, 0, texture->ImageSize(), 0, &data);
		memcpy_s(data, static_cast<size_t>(texture->ImageSize()), texture->Pixels(),
			static_cast<size_t>(texture->ImageSize()));
		vkUnmapMemory(m_vulkanDevice.m_logicalDevice, stagingBufferMemory);

		CreateImage(texture->Width(), texture->Height(), texture->MipmapLevels(), VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory);

		TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->MipmapLevels());
		CopyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texture->Width()),
			static_cast<uint32_t>(texture->Height()));
		//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

		vkDestroyBuffer(m_vulkanDevice.m_logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_vulkanDevice.m_logicalDevice, stagingBufferMemory, nullptr);

		GenerateMipmaps(m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, texture->Width(), texture->Height(), texture->MipmapLevels());

	}
}

VkDescriptorImageInfo OgEngine::RasterizerPipeline::Create2DDescriptor(const VkImage& p_image,
	const VkSamplerCreateInfo& p_samplerCreateInfo,
	const VkFormat& p_format,
	const VkImageLayout& p_layout) const
{
	VkImageViewCreateInfo viewInfo = Initializers::imageViewCreateInfo();
	viewInfo.image = p_image;
	viewInfo.format = p_format;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, ~0u, 0, 1 };

	VkImageView view;
	vkCreateImageView(m_vulkanDevice.m_logicalDevice, &viewInfo, nullptr, &view);

	VkSampler sampler;
	vkCreateSampler(m_vulkanDevice.m_logicalDevice, &p_samplerCreateInfo, nullptr, &sampler);

	const VkDescriptorImageInfo info = Initializers::descriptorImageInfo(sampler, view, p_layout);
	return info;
}

void OgEngine::RasterizerPipeline::CreateTextureImageView()
{
	m_textureImageView = CreateImageView(m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,
		m_mipLevels);
}

void OgEngine::RasterizerPipeline::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0;
	samplerInfo.maxLod = static_cast<float>(m_mipLevels);
	samplerInfo.mipLodBias = 0;

	if (vkCreateSampler(m_vulkanDevice.m_logicalDevice, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void OgEngine::RasterizerPipeline::LoadModel()
{
	//ResourceManager::WaitForResource<Mesh>("cube.obj");
}

void OgEngine::RasterizerPipeline::CreateVertexBuffer(const std::shared_ptr<Mesh>& p_mesh)
{
	if (p_mesh != nullptr && !p_mesh->Vertices().empty())
	{
		const VkDeviceSize verticesBufferSize = sizeof(p_mesh->Vertices()[0]) * p_mesh->Vertices().size();

		CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_buffers[p_mesh].vertexBuffer, verticesBufferSize, (void*)(p_mesh->Vertices().data()));
	}
}

void OgEngine::RasterizerPipeline::CreateIndexBuffer(const std::shared_ptr<Mesh>& p_mesh)
{
	if (p_mesh != nullptr && !p_mesh->Indices().empty())
	{
		const VkDeviceSize indicesBufferSize = sizeof(p_mesh->Indices()[0]) * p_mesh->Indices().size();

		CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_buffers[p_mesh].indexBuffer, indicesBufferSize, (void*)(p_mesh->Indices().data()));
	}
}

void OgEngine::RasterizerPipeline::CreateUniformBuffers()
{
	const VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	const VkDeviceSize lightsSize = sizeof(UniformLightInfo);
	const VkDeviceSize materialsSize = sizeof(UniformMaterialInfo);

	m_uniformBuffers.resize(m_images.size());
	m_uniformBuffersMemory.resize(m_images.size());

	m_lightsBuffers.resize(m_images.size());
	m_lightsBuffersMemory.resize(m_images.size());

	m_lightNumberBuffers.resize(m_images.size());
	m_lightNumberBuffersMemory.resize(m_images.size());

	m_materialsBuffers.resize(m_images.size());
	m_materialsBuffersMemory.resize(m_images.size());

	for (size_t i = 0; i < m_images.size(); ++i)
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i],
			m_uniformBuffersMemory[i]);

		CreateBuffer(lightsSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_lightsBuffers[i],
			m_lightsBuffersMemory[i]);

		CreateBuffer(sizeof(unsigned int), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_lightNumberBuffers[i],
			m_lightNumberBuffersMemory[i]);

		CreateBuffer(materialsSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_materialsBuffers[i],
			m_materialsBuffersMemory[i]);
	}
}

void OgEngine::RasterizerPipeline::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 5> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(m_images.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(m_images.size());
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(m_images.size());
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = static_cast<uint32_t>(m_images.size());
	poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[4].descriptorCount = static_cast<uint32_t>(m_images.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(m_images.size());

	if (vkCreateDescriptorPool(m_vulkanDevice.m_logicalDevice, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void OgEngine::RasterizerPipeline::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(m_images.size(), m_descriptorSetLayout);
	VkDescriptorSetAllocateInfo        allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_images.size());
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(m_images.size());
	if (vkAllocateDescriptorSets(m_vulkanDevice.m_logicalDevice, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < m_images.size(); ++i)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_textureImageView;
		imageInfo.sampler = m_textureSampler;

		VkDescriptorBufferInfo lightsInfo;
		lightsInfo.buffer = m_lightsBuffers[i];
		lightsInfo.offset = 0;
		lightsInfo.range = sizeof(UniformLightInfo);

		VkDescriptorBufferInfo lightNumberInfo = {};
		lightNumberInfo.buffer = m_lightNumberBuffers[i];
		lightNumberInfo.offset = 0;
		lightNumberInfo.range = sizeof(unsigned int);

		VkDescriptorBufferInfo materialsInfo = {};
		materialsInfo.buffer = m_materialsBuffers[i];
		materialsInfo.offset = 0;
		materialsInfo.range = sizeof(UniformMaterialInfo);

		std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = m_descriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &lightsInfo;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = m_descriptorSets[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pBufferInfo = &lightNumberInfo;

		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = m_descriptorSets[i];
		descriptorWrites[4].dstBinding = 4;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pBufferInfo = &materialsInfo;

		vkUpdateDescriptorSets(m_vulkanDevice.m_logicalDevice, static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(), 0, nullptr);
	}
}

void OgEngine::RasterizerPipeline::CreateCommandPool()
{
	FindQueueFamilies(m_vulkanDevice.m_gpu);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_vulkanDevice.m_gpuGraphicFamily.value();

	if (vkCreateCommandPool(m_vulkanDevice.m_logicalDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics command pool!");
	}
}

void OgEngine::RasterizerPipeline::CreateColorResources()
{
	const VkFormat colorFormat = m_chainColorFormat;

	CreateImage(m_chainExtent.width, m_chainExtent.height, 1, m_msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colorImage, m_colorImageMemory);
	m_colorImageView = CreateImageView(m_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void OgEngine::RasterizerPipeline::CreateDepthResources()
{
	const VkFormat depthFormat = FindDepthFormat();

	CreateImage(m_chainExtent.width, m_chainExtent.height, 1, m_msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage,
		m_depthImageMemory);
	m_depthImageView = CreateImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void OgEngine::RasterizerPipeline::CreateCommandBuffers()
{
	m_commandBuffers.resize(m_chainFrameBuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	if (vkAllocateCommandBuffers(m_vulkanDevice.m_logicalDevice, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < m_commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_chainFrameBuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_chainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { {0.4f, 0.4f, 0.4f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

		VkDeviceSize offsets[] = { 0 };
		// For loop over m_buffers
		int j = 0;
		for (auto iterator = m_buffers.begin(); iterator != m_buffers.end(); ++iterator)
		{
			/*vkCmdPushConstants(
				m_commandBuffers[i],
				m_pipelineLayout,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(m_pushConstants),
				m_pushConstants.data());
				*/

			UpdateUniformBuffer(m_currentFrame, iterator->second.model);

			VkBuffer vertexBuffers[] = { iterator->second.vertexBuffer.buffer };
			vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(m_commandBuffers[i], iterator->second.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
				&m_descriptorSets[i], 0, nullptr);


			vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(iterator->first->Indices().size()), 1, 0, 0, 0);
			++j;
		}

		// End 
		vkCmdEndRenderPass(m_commandBuffers[i]);

		if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void OgEngine::RasterizerPipeline::CreateSynchronizedObjects()
{
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imagesInFlight.resize(m_imageViews.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(m_vulkanDevice.m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i])
			!= VK_SUCCESS ||
			vkCreateSemaphore(m_vulkanDevice.m_logicalDevice, &semaphoreInfo, nullptr,
				&m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_vulkanDevice.m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void OgEngine::RasterizerPipeline::CleanupSwapChain()
{
	vkDestroyImageView(m_vulkanDevice.m_logicalDevice, m_depthImageView, nullptr);
	vkDestroyImage(m_vulkanDevice.m_logicalDevice, m_depthImage, nullptr);
	vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_depthImageMemory, nullptr);

	vkDestroyImageView(m_vulkanDevice.m_logicalDevice, m_colorImageView, nullptr);
	vkDestroyImage(m_vulkanDevice.m_logicalDevice, m_colorImage, nullptr);
	vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_colorImageMemory, nullptr);

	for (auto framebuffer : m_chainFrameBuffers)
	{
		vkDestroyFramebuffer(m_vulkanDevice.m_logicalDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(m_vulkanDevice.m_logicalDevice, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()),
		m_commandBuffers.data());

	vkDestroyPipeline(m_vulkanDevice.m_logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_vulkanDevice.m_logicalDevice, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_vulkanDevice.m_logicalDevice, m_renderPass, nullptr);

	for (auto imageView : m_imageViews)
	{
		vkDestroyImageView(m_vulkanDevice.m_logicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_vulkanDevice.m_logicalDevice, m_chain, nullptr);

	for (size_t i = 0; i < m_images.size(); i++)
	{
		// ubo
		vkDestroyBuffer(m_vulkanDevice.m_logicalDevice, m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_uniformBuffersMemory[i], nullptr);
		// lights
		vkDestroyBuffer(m_vulkanDevice.m_logicalDevice, m_lightsBuffers[i], nullptr);
		vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_lightsBuffersMemory[i], nullptr);
		// light count
		vkDestroyBuffer(m_vulkanDevice.m_logicalDevice, m_lightNumberBuffers[i], nullptr);
		vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_lightNumberBuffersMemory[i], nullptr);
		// materials
		vkDestroyBuffer(m_vulkanDevice.m_logicalDevice, m_materialsBuffers[i], nullptr);
		vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_materialsBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(m_vulkanDevice.m_logicalDevice, m_descriptorPool, nullptr);
}

void OgEngine::RasterizerPipeline::RecreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_vulkanDevice.m_logicalDevice);

	CleanupSwapChain();

	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
}

void OgEngine::RasterizerPipeline::InitImGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	SetupImGUIStyle();
	//ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForVulkan(m_window, true);

	VkAttachmentDescription attachment = {};
	attachment.format = m_chainColorFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment = {};
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	if (vkCreateRenderPass(m_vulkanDevice.m_logicalDevice, &info, nullptr, &m_ImGUIrenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create Dear ImGui's render pass");
	}

	const std::vector<VkDescriptorPoolSize> ImGUIpoolSizes =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1500 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfoIMGUI = Initializers::descriptorPoolCreateInfo(ImGUIpoolSizes, 15000);
	vkCreateDescriptorPool(m_vulkanDevice.m_logicalDevice, &descriptorPoolCreateInfoIMGUI, nullptr, &m_ImGUIdescriptorPool);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_vulkanDevice.m_instance;
	init_info.PhysicalDevice = m_vulkanDevice.m_gpu;
	init_info.Device = m_vulkanDevice.m_logicalDevice;
	init_info.QueueFamily = m_vulkanDevice.m_gpuGraphicFamily.value();
	init_info.Queue = m_graphicsQueue;
	init_info.PipelineCache = m_pipelineCache;
	init_info.DescriptorPool = m_ImGUIdescriptorPool;
	init_info.Allocator = nullptr;
	init_info.MinImageCount = m_minImageCount;
	init_info.ImageCount = m_minImageCount;
	init_info.CheckVkResultFn = CHECK_ERROR;
	ImGui_ImplVulkan_Init(&init_info, m_ImGUIrenderPass);

	VkCommandBuffer cmdBuffer = BeginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);


	EndSingleTimeCommands(cmdBuffer);
}

void OgEngine::RasterizerPipeline::SetupImGUIStyle()
{
	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
	colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
	colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	//colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
	//colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

	style->ChildRounding = 4.0f;
	style->FrameBorderSize = 1.0f;
	style->FrameRounding = 2.0f;
	style->GrabMinSize = 7.0f;
	style->PopupRounding = 2.0f;
	style->ScrollbarRounding = 12.0f;
	style->ScrollbarSize = 13.0f;
	style->TabBorderSize = 1.0f;
	style->TabRounding = 0.0f;
	style->WindowRounding = 4.0f;
}

void OgEngine::RasterizerPipeline::SetupImGUI()
{
	m_ImGUIcommandBuffers.resize(m_imageViews.size());
	for (size_t i = 0; i < m_imageViews.size(); i++)
	{
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = m_vulkanDevice.m_gpuGraphicFamily.value();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		CHECK_ERROR(vkCreateCommandPool(m_vulkanDevice.m_logicalDevice, &cmdPoolInfo, nullptr, &m_ImGUIcommandPool));

		m_ImGUIcommandBuffers.resize(1);

		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			Initializers::commandBufferAllocateInfo(
				m_ImGUIcommandPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				static_cast<uint32_t>(m_ImGUIcommandBuffers.size()));

		CHECK_ERROR(vkAllocateCommandBuffers(m_vulkanDevice.m_logicalDevice, &cmdBufAllocateInfo, m_ImGUIcommandBuffers.data()));
	}

	m_sceneID = nullptr;//ImGui_ImplVulkan_AddTexture(m_offScreenPass.sampler, m_offScreenPass.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vkQueueWaitIdle(m_graphicsQueue);
}

void OgEngine::RasterizerPipeline::SetupImGUIFrameBuffers()
{
	VkImageView attachment[1];

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = nullptr;
	frameBufferCreateInfo.renderPass = m_ImGUIrenderPass;
	frameBufferCreateInfo.attachmentCount = 1;
	frameBufferCreateInfo.pAttachments = attachment;
	frameBufferCreateInfo.width = m_width;
	frameBufferCreateInfo.height = m_height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	m_ImGUIframeBuffers.resize(m_images.size());
	for (uint32_t i = 0; i < m_ImGUIframeBuffers.size(); i++)
	{
		attachment[0] = m_imageViews[i];
		CHECK_ERROR(vkCreateFramebuffer(m_vulkanDevice.m_logicalDevice, &frameBufferCreateInfo, nullptr, &m_ImGUIframeBuffers[i]));
	}
}

void OgEngine::RasterizerPipeline::RescaleImGUI()
{
}

void OgEngine::RasterizerPipeline::RenderUI(uint32_t p_id)
{
	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = m_ImGUIrenderPass;
	info.framebuffer = m_ImGUIframeBuffers[p_id];
	info.renderArea.extent.width = m_width;
	info.renderArea.extent.height = m_height;
	info.clearValueCount = 1;

	VkClearValue clear;
	clear.color = { {0.0, 0.0, 0.0, 1.0f} };
	clear.depthStencil = { 1,0 };
	info.pClearValues = &clear;

	VkCommandBuffer cmdBuffer = BeginSingleTimeCommands();
	vkCmdBeginRenderPass(cmdBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	vkCmdEndRenderPass(cmdBuffer);

	EndSingleTimeCommands(cmdBuffer);
}

ImGuiContext* OgEngine::RasterizerPipeline::GetUIContext()
{
	return ImGui::GetCurrentContext();
}

void OgEngine::RasterizerPipeline::PrepareIMGUIFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void OgEngine::RasterizerPipeline::DrawUI()
{
	ImGui::Render();
}

void OgEngine::RasterizerPipeline::DrawEditor()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		//ImGuiID dockspaceID = ImGui::GetID("MyDockSpace");
		//ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	}
	//ImGui::SetNextWindowPos({ 0, 0 });
	//ImGui::SetNextWindowSize(ImVec2(m_vulkanContext->GetRTPipeline()->m_width, m_vulkanContext->GetRTPipeline()->m_height ));

	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background 
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	bool open = true;
	ImGui::Begin("Editor", &open, window_flags);
	{

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}
	}
	ImGui::End();
	ImGui::ShowDemoWindow();
	ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX }, CustomConstraints::Wide);

	ImGui::Begin("Scene");
	{
		ImVec2 newSize = ImGui::GetContentRegionAvail();
		//ResizeOffPass(newSize.x, newSize.y);
		// FOR NOW : ImGui::Image(m_sceneID, newSize);
	}
	ImGui::End();
}

void OgEngine::RasterizerPipeline::CHECK_ERROR(VkResult p_result)
{
	if (p_result != VK_SUCCESS)
	{
		std::cout << p_result << '\n';
		throw std::runtime_error(("Error with Vulkan function"));
	}
}

void OgEngine::RasterizerPipeline::CreatePipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	vkCreatePipelineCache(m_vulkanDevice.m_logicalDevice, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
}