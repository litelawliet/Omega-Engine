#include <OgRendering/Rendering/RasterizerPipeline.h>
#include <OgRendering/Managers/Loaders/ShaderLoader.h>
#include <chrono>
#include <OgRendering/Managers/ResourceManager.h>
#include <OgRendering/Rendering/VulkanContext.h>
#include <OgRendering/Utils/Debugger.h>
#include <OgRendering/UI/imgui/imgui_impl_glfw.h>
#include <OgRendering/UI/imgui/imgui_impl_vulkan.h>

OgEngine::RasterizerPipeline::RasterizerPipeline(GLFWwindow* p_window, Device& p_vulkanDevice,
	VkQueue& p_graphicQueue, VkQueue& p_presentQueue,
	const uint32_t p_width, const uint32_t  p_height)
	: m_window(p_window), m_device(p_vulkanDevice), m_graphicsQueue(p_graphicQueue),
	m_presentQueue(p_presentQueue), m_width(p_width), m_height(p_height)
{
	m_camera.SetPerspective(60.0f,static_cast<float>(p_width)/static_cast<float>(p_height), 0.1f, 1000.0f);
}

void OgEngine::RasterizerPipeline::SetupPipeline()
{
	InitializeVMA();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateCommandPool();

	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();

	CreateDescriptorPool();

	SetupOffScreenPass();

	CreateCommandBuffers();
	CreateSynchronizedObjects();

	InitImGUI();
	SetupImGUIFrameBuffers();
	SetupImGUI();

	CreateTexture(ResourceManager::Get<Texture>("default.png"), TEXTURE_TYPE::TEXTURE);
	CreateTexture(ResourceManager::Get<Texture>("error.png"), TEXTURE_TYPE::TEXTURE);
}

void OgEngine::RasterizerPipeline::CleanPipeline()
{
	CleanupSwapChain();

	FreeImGUIContext();

	// Eventually adapts this to multiple lights
	for (auto& buffer : m_buffers)
	{
		auto& bufferArray = buffer.second;

		DestroyObjectInstance(bufferArray);
	}

	for (auto& meshBuffer : m_meshesBuffers)
	{
		meshBuffer.second.first.Destroy();
		meshBuffer.second.second.Destroy();
	}

	vkDestroySampler(m_device.logicalDevice, m_textureSampler, nullptr);
	vkDestroyImageView(m_device.logicalDevice, m_textureImageView, nullptr);

	vkDestroyImage(m_device.logicalDevice, m_textureImage, nullptr);
	vkFreeMemory(m_device.logicalDevice, m_textureImageMemory, nullptr);

	vkDestroyDescriptorSetLayout(m_device.logicalDevice, m_descriptorSetLayout, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_device.logicalDevice, m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_device.logicalDevice, m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_device.logicalDevice, m_inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_device.logicalDevice, m_commandPool, nullptr);

	vmaDestroyAllocator(m_allocator);
}

void OgEngine::RasterizerPipeline::Update(
	const float                  p_dt,
	const std::uint64_t p_objectID,
	const glm::mat4& p_modelTransform,
	Mesh* p_mesh,
	const std::string& p_texture,
	const std::string& p_normalMap,
	const glm::vec4& p_color)
{
	const auto iterator = m_buffers.find(p_objectID);
	// Object does not exists yet in OgRendering memory
	if (iterator == m_buffers.end())
	{
		if (p_mesh != nullptr)
		{
			if (m_meshesBuffers.find(p_mesh) == m_meshesBuffers.end())
			{
				m_meshesBuffers.try_emplace(p_mesh);
				CreateVertexBuffer(p_mesh, &m_meshesBuffers[p_mesh].first);
				CreateIndexBuffer(p_mesh, &m_meshesBuffers[p_mesh].second);
			}
		}
		m_buffers.insert(std::make_pair(p_objectID, ObjectInstance(p_mesh)));
		auto& buffer = m_buffers.at(p_objectID);
		buffer.instanceID = p_objectID;

		buffer.model.UpdateModelMatrix(p_modelTransform);
		buffer.model.ChangeColor(p_color);

		// Define the texture
		auto* tex = ResourceManager::Get<Texture>(p_texture);
		if (tex)
		{
			if (m_textures.find(tex) == m_textures.end())
			{
				CreateTexture(tex, TEXTURE_TYPE::TEXTURE);
			}
		}
		buffer.model.SetTexture(tex);

		AllocateBufferArray(buffer);
		AllocateDescriptorSet(buffer);
		BindDescriptorSet(buffer);
	}
	else
	{
		// Update the model matrix
		iterator->second.model.UpdateModelMatrix(p_modelTransform);
		iterator->second.model.ChangeColor(p_color);

		auto* tex = ResourceManager::Get<Texture>(p_texture);
		if (tex != iterator->second.model.Texture()) // Avoid rebinding the texture every frame for every objects
		{
			if (tex)
			{
				if (m_textures.find(tex) == m_textures.end())
				{
					CreateTexture(tex, TEXTURE_TYPE::TEXTURE);
				}
				iterator->second.model.SetTexture(tex);
				BindDescriptorSet(iterator->second);
			}
		}

		UpdateUniformBuffer(m_buffers.at(p_objectID));

		// Update the mesh and allocate the buffer if the mesh doesn't exist in the rendering memory
		if (p_mesh != nullptr)
		{
			if (m_meshesBuffers.find(p_mesh) == m_meshesBuffers.end())
			{
				m_meshesBuffers.try_emplace(p_mesh);
				CreateVertexBuffer(p_mesh, &m_meshesBuffers[p_mesh].first);
				CreateIndexBuffer(p_mesh, &m_meshesBuffers[p_mesh].second);
			}
		}
		// mesh might be nullptr or valid. Either case the RenderFrame will skip a nullptr mesh
		iterator->second.model.SetMesh(p_mesh);

	}
}

void OgEngine::RasterizerPipeline::RenderFrame()
{
	InitFrame();

	for (auto i = 0u; i < m_commandBuffers.size(); ++i)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VkResult buffersResult = vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo);
		DBG_ASSERT_VULKAN_MSG(buffersResult, "Failed to create command buffer begin info (drawing).");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_chainFrameBuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_chainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.3f, 0.3f, 0.3f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// bind the graphics pipeline to the command buffer
		// any VkDraw command afterwards is affected by this pipeline
		{
			vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

			VkViewport viewport = { 0,0,static_cast<float>(m_width), static_cast<float>(m_height), 0, 1 };
			vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = { 0, 0, {m_width, m_height} };
			vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

			size_t objectIndex = 0u;

			for (const auto& m_buffer : m_buffers)
			{
				if (m_buffer.second.model.Mesh() == nullptr)
				{
					continue; // Skip to the next model because this model has a nullptr mesh, might crash.
				}

				vkCmdBindDescriptorSets(
					m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_pipelineLayout, 0,
					1, &m_buffer.second.descriptorSet,
					0, nullptr);

				VkDeviceSize offsets = { 0 };
				VkBuffer vertexBuffers[] = { m_meshesBuffers[m_buffer.second.model.Mesh()].first.buffer };
				vkCmdBindVertexBuffers(
					m_commandBuffers[i],
					0,
					1,
					vertexBuffers,
					&offsets);

				vkCmdBindIndexBuffer(
					m_commandBuffers[i],
					m_meshesBuffers[m_buffer.second.model.Mesh()].second.buffer,
					0,
					VK_INDEX_TYPE_UINT32);

				vkCmdDrawIndexed(
					m_commandBuffers[i],
					static_cast<uint32_t>(m_buffer.second.model.Mesh()->Indices().size()),
					1,
					0,
					0,
					0);

				++objectIndex;
			}

			// End  the drawing
			vkCmdEndRenderPass(m_commandBuffers[i]);

			buffersResult = vkEndCommandBuffer(m_commandBuffers[i]);
			DBG_ASSERT_VULKAN_MSG(buffersResult, "failed to record command buffer!");
		}
	}

	// present :
	if (m_imagesInFlight[m_imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(m_device.logicalDevice, 1, &m_imagesInFlight[m_imageIndex], VK_TRUE, UINT64_MAX);
	}
	m_imagesInFlight[m_imageIndex] = m_inFlightFences[m_currentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[m_imageIndex];

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_device.logicalDevice, 1, &m_inFlightFences[m_currentFrame]);
	CopyImage(m_colorImage);
	VkResult result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to submit draw command buffer.");

	RenderUI(m_imageIndex);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_chain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &m_imageIndex;

	result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || VulkanContext::framebufferResized) {
		VulkanContext::framebufferResized = false;
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		DBG_ASSERT_VULKAN_MSG(result, "Failed to present swap chain image.");
	}
}

#pragma region Helpers
OgEngine::SwapChainSupportDetails OgEngine::RasterizerPipeline::QuerySwapChainSupport(VkPhysicalDevice& p_gpu) const
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_gpu, m_device.surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_gpu, m_device.surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(p_gpu, m_device.surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_gpu, m_device.surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(p_gpu, m_device.surface, &presentModeCount,
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
	if (vkCreateImageView(m_device.logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture image view in CreateImageView.");
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
		vkGetPhysicalDeviceFormatProperties(m_device.gpu, format, &props);

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
			m_device.graphicFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(p_physicalDevice, i, m_device.surface, &presentSupport);

		if (presentSupport)
		{
			m_device.presentFamily = i;
		}

		if (m_device.graphicFamily.has_value() && m_device.presentFamily.has_value())
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

	if (vkCreateImage(m_device.logicalDevice, &imageInfo, nullptr, &p_image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device.logicalDevice, p_image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, p_properties);

	if (vkAllocateMemory(m_device.logicalDevice, &allocInfo, nullptr, &p_imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(m_device.logicalDevice, p_image, p_imageMemory, 0);
}

uint32_t OgEngine::RasterizerPipeline::FindMemoryType(const uint32_t              p_typeFilter,
	const VkMemoryPropertyFlags p_properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_device.gpu, &memProperties);

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
	DBG_ASSERT_VULKAN_MSG(vkAllocateCommandBuffers(m_device.logicalDevice, &allocInfo, &commandBuffer), "Couldn't allocate command buffers.");

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	DBG_ASSERT_VULKAN_MSG(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Couldn't begin command buffer.");

	return commandBuffer;
}

void OgEngine::RasterizerPipeline::EndSingleTimeCommands(VkCommandBuffer p_commandBuffer) const
{
	DBG_ASSERT_VULKAN_MSG(vkEndCommandBuffer(p_commandBuffer), "Couldn't end command bufffer");

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &p_commandBuffer;
	// Create fence to ensure that the command buffer has finished executing
	//VkFenceCreateInfo fenceInfo = Initializers::fenceCreateInfo(0);
	//VkFence fence;
	//CHECK_ERROR(vkCreateFence(m_vulkanDevice.logicalDevice, &fenceInfo, nullptr, &fence));
	DBG_ASSERT_VULKAN_MSG(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "Couldn't submit single time command.");
	//CHECK_ERROR(vkWaitForFences(m_vulkanDevice.logicalDevice, 1, &fence, VK_TRUE, 100000000000));
	DBG_ASSERT_VULKAN_MSG(vkQueueWaitIdle(m_graphicsQueue), "Couldn't wait for QueueIdle.");
	//vkDestroyFence(m_vulkanDevice.logicalDevice, fence, nullptr);

	vkFreeCommandBuffers(m_device.logicalDevice, m_commandPool, 1, &p_commandBuffer);
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

	VkResult result = vkCreateBuffer(m_device.logicalDevice, &bufferInfo, nullptr, &p_buffer);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create buffer in CreateBuffer.");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_device.logicalDevice, p_buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, p_properties);

	result = vkAllocateMemory(m_device.logicalDevice, &allocInfo, nullptr, &p_bufferMemory);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to allocate buffer memory in CreateBuffer.");

	result = vkBindBufferMemory(m_device.logicalDevice, p_buffer, p_bufferMemory, p_dynamicOffset);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to bind buffer memory in CreateBuffer.");
}

VkResult OgEngine::RasterizerPipeline::CreateBuffer(VkBufferUsageFlags    p_usageFlags,
	VkMemoryPropertyFlags p_memoryPropertyFlags, Buffer* p_buffer,
	VkDeviceSize          p_size, void* p_data) const
{
	p_buffer->device = m_device.logicalDevice;

	// Create the p_buffer handle
	VkBufferCreateInfo bufferCreateInfo = Initializers::bufferCreateInfo(p_usageFlags, p_size);
	CHECK_ERROR(vkCreateBuffer(m_device.logicalDevice, &bufferCreateInfo, nullptr, &p_buffer->buffer));

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memory_requirements;
	VkMemoryAllocateInfo memAlloc = Initializers::memoryAllocateInfo();
	vkGetBufferMemoryRequirements(m_device.logicalDevice, p_buffer->buffer, &memory_requirements);
	memAlloc.allocationSize = memory_requirements.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, p_memoryPropertyFlags);
	CHECK_ERROR(vkAllocateMemory(m_device.logicalDevice, &memAlloc, nullptr, &p_buffer->memory));

	p_buffer->alignment = memory_requirements.alignment;
	p_buffer->size = memAlloc.allocationSize;
	p_buffer->usageFlags = p_usageFlags;
	p_buffer->memoryPropertyFlags = p_memoryPropertyFlags;

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (p_data != nullptr)
	{
		p_buffer->Map();
		memcpy_s(p_buffer->mapped, p_size, p_data, p_size);
		if ((p_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			DBG_ASSERT_VULKAN_MSG(p_buffer->Flush(), "Cannot create vertex buffer");

		p_buffer->Unmap();
	}

	// Initialize a default descriptor that covers the whole buffer size
	p_buffer->SetupDescriptor();

	// Attach the memory to the buffer object
	return p_buffer->Bind();
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

void OgEngine::RasterizerPipeline::UpdateUniformBuffer(ObjectInstance& p_objectInstance) const
{
	if (p_objectInstance == nullptr)
	{
		return;
	}

	UniformBufferObject ubo = {};

	ubo.model = p_objectInstance.model.ModelMatrix();
	ubo.view = m_camera.matrices.view;
		//glm::mat4::LookAt(glm::vec3(0.0f, 1.0f, -4.0), glm::vec3(0.0f, 0.0f, 0.0f),
		//glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = m_camera.matrices.perspective;
		//glm::mat4::Perspective((45.0f), m_chainExtent.width / static_cast<float>(m_chainExtent.height), 0.1f,
		//2000.0f);

	// Mapping a default mvp
	void* mvpMapped = nullptr;
	CHECK_ERROR(vkMapMemory(m_device.logicalDevice, p_objectInstance.uniformBufferMemory, 0, sizeof(ubo), 0, &mvpMapped));
	memcpy_s(mvpMapped, sizeof(ubo), &ubo, sizeof(ubo));
	vkUnmapMemory(m_device.logicalDevice, p_objectInstance.uniformBufferMemory);

	UniformLightInfo lightUniform = {};
	lightUniform.diffuse = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0 };
	lightUniform.ambient = glm::vec4{ 0.2f, 0.0f, 0.0f, 1.0 };
	lightUniform.specular = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0 };
	lightUniform.position = glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f };
	void* lightData;
	CHECK_ERROR(vkMapMemory(m_device.logicalDevice, p_objectInstance.lightsBufferMemory, 0, sizeof(lightUniform), 0, &lightData));
	memcpy_s(lightData, sizeof(lightUniform), &lightUniform, sizeof(lightUniform));
	vkUnmapMemory(m_device.logicalDevice, p_objectInstance.lightsBufferMemory);

	unsigned int nbLight = 1;
	void* lightNumberData;
	CHECK_ERROR(vkMapMemory(m_device.logicalDevice, p_objectInstance.lightNumberBufferMemory, 0, sizeof(unsigned int), 0, &lightNumberData));
	memcpy_s(lightNumberData, sizeof(unsigned int), &nbLight, sizeof(unsigned int));
	vkUnmapMemory(m_device.logicalDevice, p_objectInstance.lightNumberBufferMemory);

	UniformMaterialInfo materialUniform = {};
	materialUniform.color = p_objectInstance.model.Material().color;
	materialUniform.rough = 1.0f;
	materialUniform.metal = true;
	void* materialData;
	CHECK_ERROR(vkMapMemory(m_device.logicalDevice, p_objectInstance.materialsBufferMemory, 0, sizeof(materialUniform), 0, &materialData));
	memcpy_s(materialData, sizeof(materialUniform), &materialUniform, sizeof(materialUniform));
	vkUnmapMemory(m_device.logicalDevice, p_objectInstance.materialsBufferMemory);
}

void OgEngine::RasterizerPipeline::DestroyObjectInstance(ObjectInstance& p_objectInstance) const
{
	if (p_objectInstance == nullptr)
	{
		return;
	}

	vkDestroyBuffer(m_device.logicalDevice, p_objectInstance.uniformBuffer, nullptr);
	vkFreeMemory(m_device.logicalDevice, p_objectInstance.uniformBufferMemory, nullptr);

	vkDestroyBuffer(m_device.logicalDevice, p_objectInstance.lightsBuffer, nullptr);
	vkFreeMemory(m_device.logicalDevice, p_objectInstance.lightsBufferMemory, nullptr);

	vkDestroyBuffer(m_device.logicalDevice, p_objectInstance.lightNumberBuffer, nullptr);
	vkFreeMemory(m_device.logicalDevice, p_objectInstance.lightNumberBufferMemory, nullptr);

	vkDestroyBuffer(m_device.logicalDevice, p_objectInstance.materialsBuffer, nullptr);
	vkFreeMemory(m_device.logicalDevice, p_objectInstance.materialsBufferMemory, nullptr);
}

void OgEngine::RasterizerPipeline::CreateTexture(const std::string& p_texture, const TEXTURE_TYPE p_textureType)
{
	const std::string_view p_filePath = p_texture;
	const std::string_view fileName{ p_filePath.data() + (p_filePath.find_last_of('/') + 1) };

	auto* texture = ResourceManager::Get<Texture>(fileName);

	if (texture)
	{
		if (m_textures.find(texture) == m_textures.end())
		{
			m_textures.try_emplace(texture, TextureData{});

			auto& textureData = m_textures.at(texture);
			CreateTextureImage(textureData, texture);
			CreateTextureImageView(textureData);
			CreateTextureSampler(textureData);
		}
	}
}

void OgEngine::RasterizerPipeline::CreateTexture(Texture* p_textureAddr, const TEXTURE_TYPE p_textureType)
{
	if (p_textureAddr)
	{
		m_textures.try_emplace(p_textureAddr, TextureData{});

		auto& textureData = m_textures.at(p_textureAddr);
		CreateTextureImage(textureData, p_textureAddr);
		CreateTextureImageView(textureData);
		CreateTextureSampler(textureData);
	}
}

void OgEngine::RasterizerPipeline::UpdateCamera(const glm::vec3& p_position,
	const glm::vec3& p_rotation)
{
	m_camera.SetPosition(p_position);
	m_camera.SetRotation(p_rotation);
	m_camera.UpdateViewMatrix();
}

OgEngine::Camera& OgEngine::RasterizerPipeline::GetCurrentCamera()
{
	return m_camera;
}

void OgEngine::RasterizerPipeline::InitializeVMA()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorInfo.physicalDevice = m_device.gpu;
	allocatorInfo.device = m_device.logicalDevice;
	allocatorInfo.instance = m_device.instance;

	DBG_ASSERT_VULKAN_MSG(vmaCreateAllocator(&allocatorInfo, &m_allocator), "Couldn't start VMA.");
}

void OgEngine::RasterizerPipeline::CleanAllObjectInstance()
{
	for (auto& objectInstance : m_buffers)
	{
		vkDestroyBuffer(m_device.logicalDevice, objectInstance.second.uniformBuffer, nullptr);
		vkFreeMemory(m_device.logicalDevice, objectInstance.second.uniformBufferMemory, nullptr);

		vkDestroyBuffer(m_device.logicalDevice, objectInstance.second.lightsBuffer, nullptr);
		vkFreeMemory(m_device.logicalDevice, objectInstance.second.lightsBufferMemory, nullptr);

		vkDestroyBuffer(m_device.logicalDevice, objectInstance.second.lightNumberBuffer, nullptr);
		vkFreeMemory(m_device.logicalDevice, objectInstance.second.lightNumberBufferMemory, nullptr);

		vkDestroyBuffer(m_device.logicalDevice, objectInstance.second.materialsBuffer, nullptr);
		vkFreeMemory(m_device.logicalDevice, objectInstance.second.materialsBufferMemory, nullptr);
	}
	m_buffers.clear();
}


void OgEngine::RasterizerPipeline::DestroyObject(const std::uint64_t p_objectID)
{
	const auto& iterator = m_buffers.find(p_objectID);
	if (iterator != m_buffers.end())
	{
		m_buffers.erase(iterator);
	}

}

void OgEngine::RasterizerPipeline::GenerateMipmaps(TextureData& p_textureData, VkFormat     p_imageFormat, int32_t p_texWidth,
	int32_t p_texHeight, uint32_t p_mipLevels) const
{
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_device.gpu, p_imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = p_textureData.img;
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
			p_textureData.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			p_textureData.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

void OgEngine::RasterizerPipeline::SetupOffScreenPass()
{
	m_offScreenPass.width = m_width;
	m_offScreenPass.height = m_height;

	// Find a suitable depth format
	VkFormat fbDepthFormat;
	VkBool32 validate = RaytracingPipeline::GetSupportedDepthFormat(m_device.gpu, &fbDepthFormat);
	assert(validate);

	// Color attachment
	VkImageCreateInfo image = Initializers::imageCreateInfo();
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = VK_FORMAT_B8G8R8A8_UNORM;
	image.extent.width = m_offScreenPass.width;
	image.extent.height = m_offScreenPass.height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	// We will sample directly from the color attachment
	image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkMemoryAllocateInfo memAlloc = Initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	CHECK_ERROR(vkCreateImage(m_device.logicalDevice, &image, nullptr, &m_offScreenPass.color.image));
	vkGetImageMemoryRequirements(m_device.logicalDevice, m_offScreenPass.color.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	CHECK_ERROR(vkAllocateMemory(m_device.logicalDevice, &memAlloc, nullptr, &m_offScreenPass.color.mem));
	CHECK_ERROR(vkBindImageMemory(m_device.logicalDevice, m_offScreenPass.color.image, m_offScreenPass.color.mem, 0));

	VkImageViewCreateInfo colorImageView = Initializers::imageViewCreateInfo();
	colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageView.format = VK_FORMAT_B8G8R8A8_UNORM;
	colorImageView.subresourceRange = {};
	colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageView.subresourceRange.baseMipLevel = 0;
	colorImageView.subresourceRange.levelCount = 1;
	colorImageView.subresourceRange.baseArrayLayer = 0;
	colorImageView.subresourceRange.layerCount = 1;
	colorImageView.image = m_offScreenPass.color.image;
	CHECK_ERROR(vkCreateImageView(m_device.logicalDevice, &colorImageView, nullptr, &m_offScreenPass.color.view));

	// Create sampler to sample from the attachment in the fragment shader
	VkSamplerCreateInfo samplerInfo = Initializers::samplerCreateInfo();
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	CHECK_ERROR(vkCreateSampler(m_device.logicalDevice, &samplerInfo, nullptr, &m_offScreenPass.sampler));

	// Depth stencil attachment
	image.format = fbDepthFormat;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	CHECK_ERROR(vkCreateImage(m_device.logicalDevice, &image, nullptr, &m_offScreenPass.depth.image));
	vkGetImageMemoryRequirements(m_device.logicalDevice, m_offScreenPass.depth.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	CHECK_ERROR(vkAllocateMemory(m_device.logicalDevice, &memAlloc, nullptr, &m_offScreenPass.depth.mem));
	CHECK_ERROR(vkBindImageMemory(m_device.logicalDevice, m_offScreenPass.depth.image, m_offScreenPass.depth.mem, 0));

	VkImageViewCreateInfo depthStencilView = Initializers::imageViewCreateInfo();
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = fbDepthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = m_offScreenPass.depth.image;
	CHECK_ERROR(vkCreateImageView(m_device.logicalDevice, &depthStencilView, nullptr, &m_offScreenPass.depth.view));

	// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering

	std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
	// Color attachment
	attchmentDescriptions[0].format = VK_FORMAT_B8G8R8A8_UNORM;
	attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	// Depth attachment
	attchmentDescriptions[1].format = fbDepthFormat;
	attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies = { {} };

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
	renderPassInfo.pAttachments = attchmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	CHECK_ERROR(vkCreateRenderPass(m_device.logicalDevice, &renderPassInfo, nullptr, &m_offScreenPass.renderPass));

	VkImageView attachments[2];
	attachments[0] = m_offScreenPass.color.view;
	attachments[1] = m_offScreenPass.depth.view;

	VkFramebufferCreateInfo fbufCreateInfo = Initializers::framebufferCreateInfo();
	fbufCreateInfo.renderPass = m_offScreenPass.renderPass;
	fbufCreateInfo.attachmentCount = 2;
	fbufCreateInfo.pAttachments = attachments;
	fbufCreateInfo.width = m_offScreenPass.width;
	fbufCreateInfo.height = m_offScreenPass.height;
	fbufCreateInfo.layers = 1;

	CHECK_ERROR(vkCreateFramebuffer(m_device.logicalDevice, &fbufCreateInfo, nullptr, &m_offScreenPass.frameBuffer));

	// Fill a descriptor for later use in a descriptor set 
	m_offScreenPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_offScreenPass.descriptor.imageView = m_offScreenPass.color.view;
	m_offScreenPass.descriptor.sampler = m_offScreenPass.sampler;
}

void OgEngine::RasterizerPipeline::CopyImage(VkImage p_source)
{
	const VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	RaytracingPipeline::SetImageLayout(
		commandBuffer,
		m_offScreenPass.color.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresource_range);

	// Prepare ray tracing output image as transfer source
	RaytracingPipeline::SetImageLayout(
		commandBuffer,
		p_source,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		subresource_range);

	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { m_width, m_height, 1 };
	vkCmdCopyImage(commandBuffer, p_source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_offScreenPass.color.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	// Transition swap chain image back for presentation
	RaytracingPipeline::SetImageLayout(
		commandBuffer,
		m_offScreenPass.color.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		subresource_range);

	// Transition ray tracing output image back to general layout
	RaytracingPipeline::SetImageLayout(
		commandBuffer,
		p_source,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		subresource_range);

	EndSingleTimeCommands(commandBuffer);
}

void OgEngine::RasterizerPipeline::InitFrame()
{
	vkWaitForFences(m_device.logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
	//const VkResult result = AcquireNextImage(m_imageAvailableSemaphores[m_currentFrame], &m_imageIndex);
	VkResult result = vkAcquireNextImageKHR(m_device.logicalDevice, m_chain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
	}
	else
	{
		CHECK_ERROR(result);
	}
}

void OgEngine::RasterizerPipeline::DisplayFrame()
{
	const VkResult result = QueuePresent(m_graphicsQueue, m_currentFrame, m_renderFinishedSemaphores[m_currentFrame]);
	if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)))
	{
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			//windowResize();
			return;
		}
		CHECK_ERROR(result);
	}
	CHECK_ERROR(vkQueueWaitIdle(m_graphicsQueue));
}

VkResult OgEngine::RasterizerPipeline::QueuePresent(VkQueue p_queue, uint32_t p_imageIndex, VkSemaphore p_waitSemaphore) const
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_chain;
	presentInfo.pImageIndices = &p_imageIndex;

	/* if (p_waitSemaphore != nullptr)
	 {
		 presentInfo.pWaitSemaphores = &p_waitSemaphore;
		 presentInfo.waitSemaphoreCount = 1;
	 }*/

	return vkQueuePresentKHR(p_queue, &presentInfo);
}

/*void OgEngine::RasterizerPipeline::HandleSurfaceChanges()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	CHECK_ERROR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vulkanDevice.gpu,
		m_vulkanDevice.surface,
		&surface_properties));

	if (surface_properties.currentExtent.width != m_chainExtent.width ||
		surface_properties.currentExtent.height != m_chainExtent.height)
	{
		Resize(surface_properties.currentExtent.width, surface_properties.currentExtent.height);
	}
}*/

void OgEngine::RasterizerPipeline::CreateSwapChain()
{
	const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_device.gpu);

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
	createInfo.surface = m_device.surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1u;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	m_width = extent.width;
	m_height = extent.height;

	uint32_t queueFamilyIndices[] = {
		m_device.presentFamily.value(), m_device.graphicFamily.value()
	};

	if (m_device.graphicFamily != m_device.presentFamily)
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

	if (vkCreateSwapchainKHR(m_device.logicalDevice, &createInfo, nullptr, &m_chain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(m_device.logicalDevice, m_chain, &imageCount, nullptr);
	m_images.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device.logicalDevice, m_chain, &imageCount, m_images.data());

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
	colorAttachment.samples = m_device.msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = m_device.msaaSamples;
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
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0u;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

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

	if (vkCreateRenderPass(m_device.logicalDevice, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}


}

void OgEngine::RasterizerPipeline::CreateGraphicsPipeline()
{
	VkShaderModule vertShaderModule = ShaderLoader::LoadShader("Resources/shaders/bin/rast_vert.spv",
		m_device.logicalDevice);
	VkShaderModule fragShaderModule = ShaderLoader::LoadShader("Resources/shaders/bin/rast_frag.spv",
		m_device.logicalDevice);

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
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = m_device.msaaSamples;

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

	VkResult result = vkCreatePipelineLayout(m_device.logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create pipeline layout.");

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

	if (vkCreateGraphicsPipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
		&m_graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(m_device.logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_device.logicalDevice, vertShaderModule, nullptr);

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

	const VkResult result = vkCreateDescriptorSetLayout(
		m_device.logicalDevice,
		&layoutInfo,
		nullptr,
		&m_descriptorSetLayout);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create descriptor set layout.");
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
		framebufferInfo.width = m_chainExtent.width;
		framebufferInfo.height = m_chainExtent.height;
		framebufferInfo.layers = 1u;

		VkResult result = vkCreateFramebuffer(m_device.logicalDevice, &framebufferInfo, nullptr, &m_chainFrameBuffers[i]);
		DBG_ASSERT_VULKAN_MSG(result, "Failed to create framebuffer.");
	}
}

ImTextureID OgEngine::RasterizerPipeline::AddUITexture(const char* p_texturePath)
{
	int width = 0;
	int height = 0;
	int channels = 0;
	Texture* uiTexture = nullptr;

	std::string_view p_filePath = p_texturePath;
	const std::string_view fileName{ p_filePath.data() + (p_filePath.find_last_of('/') + 1) };

	uiTexture = ResourceManager::Get<Texture>(fileName);
	if (uiTexture == nullptr)
	{
		ResourceManager::Add<Texture>(p_filePath);
		ResourceManager::WaitForResource<Texture>(fileName);
		uiTexture = ResourceManager::Get<Texture>(fileName); // Try to get the added texture

		if (!uiTexture)
		{
			uiTexture = ResourceManager::Get<Texture>("error.png");
		}
		else
		{
			width = uiTexture->Width();
			height = uiTexture->Height();
		}
	}
	else
	{
		width = uiTexture->Width();
		height = uiTexture->Height();
	}


	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * sizeof(glm::u8vec4); // TODO : replace u8vec4 by gpm vec4
	VkExtent2D extent;
	extent.width = width;
	extent.height = height;
	auto imgSize = extent;
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;

	Buffer stagingBuffer;
	CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, bufferSize);

	stagingBuffer.Map();
	memcpy_s(stagingBuffer.mapped, bufferSize, uiTexture->Pixels(), bufferSize);
	stagingBuffer.Unmap();

	VkImageCreateInfo info = Initializers::imageCreateInfo();
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = format;
	info.mipLevels = mipLevels;
	info.arrayLayers = 1;
	info.extent = { extent.width, extent.height, 1 };
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	TextureData data{};
	VkResult result = vkCreateImage(m_device.logicalDevice, &info, nullptr, &data.img);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create image in AddUITexture");

	VkCommandBuffer cmdBuffer = BeginSingleTimeCommands();

	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = info.mipLevels;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device.logicalDevice, data.img, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(m_device.logicalDevice, &allocInfo, nullptr, &data.memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate image memory in AddUITexture");
	}

	result = vkBindImageMemory(m_device.logicalDevice, data.img, data.memory, 0);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to bind image memory in AddUITexture");

	TransitionImageLayout(data.img, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);

	VkBufferImageCopy copyRegion{};
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.layerCount = 1;

	VkExtent3D extend3D;
	extend3D.width = width;
	extend3D.height = height;
	extend3D.depth = 1;
	copyRegion.imageExtent = extend3D;

	vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.buffer, data.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyRegion);

	range.levelCount = 1;

	VkSamplerCreateInfo samplerInfo = Initializers::samplerCreateInfo();
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.maxLod = FLT_MAX;

	VkImageViewCreateInfo viewInfo = Initializers::imageViewCreateInfo();
	viewInfo.image = data.img;
	viewInfo.format = format;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, ~0u, 0, 1 };

	VkImageView view;
	result = vkCreateImageView(m_device.logicalDevice, &viewInfo, nullptr, &view);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create image view in AddUITexture");
	data.view = view;

	VkSampler sampler;
	result = vkCreateSampler(m_device.logicalDevice, &samplerInfo, nullptr, &sampler);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create sampler in AddUITexture");
	data.sampler = sampler;

	data.info = Initializers::descriptorImageInfo(sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	EndSingleTimeCommands(cmdBuffer);
	GenerateMipmaps(data, VK_FORMAT_R8G8B8A8_UNORM, width, height, mipLevels);

	return ImGui_ImplVulkan_AddTexture(data.sampler, data.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void OgEngine::RasterizerPipeline::CreateTextureImage(TextureData& p_textureData, const Texture* p_loadedTexture) const
{
	if (!p_loadedTexture)
		return;

	const auto* texture = p_loadedTexture;
	if (texture != nullptr)
	{
		p_textureData.mipLevels = texture->MipmapLevels();
		VkBuffer       stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(texture->ImageSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(m_device.logicalDevice, stagingBufferMemory, 0, texture->ImageSize(), 0, &data);
		memcpy_s(data, static_cast<size_t>(texture->ImageSize()), texture->Pixels(),
			static_cast<size_t>(texture->ImageSize()));
		vkUnmapMemory(m_device.logicalDevice, stagingBufferMemory); // Ok

		CreateImage(texture->Width(), texture->Height(), texture->MipmapLevels(), VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_textureData.img, p_textureData.memory);

		TransitionImageLayout(p_textureData.img, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->MipmapLevels());
		CopyBufferToImage(stagingBuffer, p_textureData.img, static_cast<uint32_t>(texture->Width()),
			static_cast<uint32_t>(texture->Height()));
		//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
		/*TransitionImageLayout(p_textureData.img, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->MipmapLevels());*/

		vkDestroyBuffer(m_device.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_device.logicalDevice, stagingBufferMemory, nullptr);

		GenerateMipmaps(p_textureData, VK_FORMAT_R8G8B8A8_UNORM, texture->Width(), texture->Height(), texture->MipmapLevels());
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
	VkResult result = vkCreateImageView(m_device.logicalDevice, &viewInfo, nullptr, &view);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create VkImageView on Create2DDescriptor");

	VkSampler sampler;
	result = vkCreateSampler(m_device.logicalDevice, &p_samplerCreateInfo, nullptr, &sampler);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create VkSampler on Create2DDescriptor");

	const VkDescriptorImageInfo info = Initializers::descriptorImageInfo(sampler, view, p_layout);
	return info;
}

void OgEngine::RasterizerPipeline::CreateTextureImageView(TextureData& p_textureData) const
{
	p_textureData.view = CreateImageView(p_textureData.img, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,
		p_textureData.mipLevels);
}

void OgEngine::RasterizerPipeline::CreateTextureSampler(TextureData& p_textureData)
{
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(m_device.gpu, &properties);

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(p_textureData.mipLevels);
	samplerInfo.mipLodBias = 0.0f;

	VkResult result = vkCreateSampler(m_device.logicalDevice, &samplerInfo, nullptr, &p_textureData.sampler);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create texture sampler.");
}

void OgEngine::RasterizerPipeline::CreateVertexBuffer(Mesh* p_mesh, Buffer* p_bufferArray) const
{
	if (p_bufferArray == nullptr)
	{
		return;
	}

	if (p_mesh != nullptr && !p_mesh->Vertices().empty())
	{
		const VkDeviceSize verticesBufferSize = sizeof(p_mesh->Vertices()[0]) * p_mesh->Vertices().size();

		CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			p_bufferArray, verticesBufferSize, (void*)(p_mesh->Vertices().data()));
	}
}

void OgEngine::RasterizerPipeline::CreateIndexBuffer(Mesh* p_mesh, Buffer* p_bufferArray) const
{
	if (p_bufferArray == nullptr)
	{
		return;
	}

	if (p_mesh != nullptr && !p_mesh->Indices().empty())
	{
		const VkDeviceSize indicesBufferSize = sizeof(p_mesh->Indices()[0]) * p_mesh->Indices().size();

		CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			p_bufferArray, indicesBufferSize, (void*)(p_mesh->Indices().data()));
	}
}

void OgEngine::RasterizerPipeline::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 5> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = MAX_OBJECTS;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = MAX_TEXTURES;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = MAX_OBJECTS;
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = MAX_OBJECTS;
	poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[4].descriptorCount = MAX_OBJECTS;

	VkDescriptorPoolCreateInfo poolInfo = {};

	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	//poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = MAX_OBJECTS;

	const VkResult result = vkCreateDescriptorPool(m_device.logicalDevice, &poolInfo, nullptr, &m_descriptorPool);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create descriptor pool.");
}

void OgEngine::RasterizerPipeline::AllocateDescriptorSet(ObjectInstance& p_objectBuffer) const
{
	if (p_objectBuffer == nullptr)
	{
		return;
	}

	VkDescriptorSetLayout layout = m_descriptorSetLayout;
	VkDescriptorSetAllocateInfo        allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	const VkResult result = vkAllocateDescriptorSets(m_device.logicalDevice, &allocInfo, &p_objectBuffer.descriptorSet);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to allocate descriptor set.");
}

void OgEngine::RasterizerPipeline::BindDescriptorSet(ObjectInstance& p_objectBuffer) const
{
	if (p_objectBuffer == nullptr)
	{
		return;
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = p_objectBuffer.uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = m_textures.at(p_objectBuffer.model.Texture()).view;
	imageInfo.sampler = m_textures.at(p_objectBuffer.model.Texture()).sampler;

	VkDescriptorBufferInfo lightsInfo;
	lightsInfo.buffer = p_objectBuffer.lightsBuffer;
	lightsInfo.offset = 0;
	lightsInfo.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo lightNumberInfo = {};
	lightNumberInfo.buffer = p_objectBuffer.lightNumberBuffer;
	lightNumberInfo.offset = 0;
	lightNumberInfo.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo materialsInfo = {};
	materialsInfo.buffer = p_objectBuffer.materialsBuffer;
	materialsInfo.offset = 0;
	materialsInfo.range = VK_WHOLE_SIZE;

	std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = p_objectBuffer.descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = p_objectBuffer.descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = p_objectBuffer.descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &lightsInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = p_objectBuffer.descriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &lightNumberInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = p_objectBuffer.descriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &materialsInfo;

	vkUpdateDescriptorSets(m_device.logicalDevice, static_cast<uint32_t>(descriptorWrites.size()),
		descriptorWrites.data(), 0, nullptr);
}

void OgEngine::RasterizerPipeline::AllocateBufferArray(ObjectInstance& p_objectBuffer) const
{
	if (p_objectBuffer == nullptr)
	{
		return;
	}

	const VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	const VkDeviceSize lightsSize = sizeof(UniformLightInfo);
	const VkDeviceSize materialsSize = sizeof(UniformMaterialInfo);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_objectBuffer.uniformBuffer,
		p_objectBuffer.uniformBufferMemory);

	CreateBuffer(lightsSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_objectBuffer.lightsBuffer,
		p_objectBuffer.lightsBufferMemory);

	CreateBuffer(sizeof(unsigned int), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_objectBuffer.lightNumberBuffer,
		p_objectBuffer.lightNumberBufferMemory);

	CreateBuffer(materialsSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_objectBuffer.materialsBuffer,
		p_objectBuffer.materialsBufferMemory);
}

void OgEngine::RasterizerPipeline::CreateCommandPool()
{
	FindQueueFamilies(m_device.gpu);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_device.graphicFamily.value();

	if (vkCreateCommandPool(m_device.logicalDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics command pool!");
	}
}

void OgEngine::RasterizerPipeline::CreateColorResources()
{
	const VkFormat colorFormat = m_chainColorFormat;

	CreateImage(m_chainExtent.width, m_chainExtent.height, 1, m_device.msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colorImage, m_colorImageMemory);
	m_colorImageView = CreateImageView(m_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void OgEngine::RasterizerPipeline::CreateDepthResources()
{
	const VkFormat depthFormat = FindDepthFormat();

	CreateImage(m_chainExtent.width, m_chainExtent.height, 1, m_device.msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
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

	const VkResult result = vkAllocateCommandBuffers(m_device.logicalDevice, &allocInfo, m_commandBuffers.data());
	DBG_ASSERT_VULKAN_MSG(result, "failed to allocate command buffers!");
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
		if (vkCreateSemaphore(m_device.logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i])
			!= VK_SUCCESS ||
			vkCreateSemaphore(m_device.logicalDevice, &semaphoreInfo, nullptr,
				&m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_device.logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void OgEngine::RasterizerPipeline::CleanupSwapChain()
{
	vkDestroyImageView(m_device.logicalDevice, m_depthImageView, nullptr);
	vkDestroyImage(m_device.logicalDevice, m_depthImage, nullptr);
	vkFreeMemory(m_device.logicalDevice, m_depthImageMemory, nullptr);

	vkDestroyImageView(m_device.logicalDevice, m_colorImageView, nullptr);
	vkDestroyImage(m_device.logicalDevice, m_colorImage, nullptr);
	vkFreeMemory(m_device.logicalDevice, m_colorImageMemory, nullptr);

	for (auto* framebuffer : m_chainFrameBuffers)
	{
		vkDestroyFramebuffer(m_device.logicalDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(m_device.logicalDevice, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()),
		m_commandBuffers.data());

	vkDestroyPipeline(m_device.logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device.logicalDevice, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_device.logicalDevice, m_renderPass, nullptr);

	for (auto* imageView : m_imageViews)
	{
		vkDestroyImageView(m_device.logicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_device.logicalDevice, m_chain, nullptr);

	const VkResult result = vkResetDescriptorPool(m_device.logicalDevice, m_descriptorPool, 0);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to reset descriptor pool.");

	vkDestroyDescriptorPool(m_device.logicalDevice, m_descriptorPool, nullptr);
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

	vkDeviceWaitIdle(m_device.logicalDevice);

	CleanupSwapChain();

	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateColorResources();
	CreateDepthResources();
	CreateFramebuffers();
	CreateDescriptorPool();
	for (auto& buffer : m_buffers)
	{
		AllocateDescriptorSet(buffer.second);
		BindDescriptorSet(buffer.second);
	}
	CreateCommandBuffers();
	RescaleImGUI();
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

	if (vkCreateRenderPass(m_device.logicalDevice, &info, nullptr, &m_ImGUIrenderPass) != VK_SUCCESS)
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
	vkCreateDescriptorPool(m_device.logicalDevice, &descriptorPoolCreateInfoIMGUI, nullptr, &m_ImGUIdescriptorPool);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_device.instance;
	init_info.PhysicalDevice = m_device.gpu;
	init_info.Device = m_device.logicalDevice;
	init_info.QueueFamily = m_device.graphicFamily.value();
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
	colors[ImGuiCol_Border] = ImVec4(0.2f, 0.2f, 0.2f, 0.000f);
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
		cmdPoolInfo.queueFamilyIndex = m_device.graphicFamily.value();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		CHECK_ERROR(vkCreateCommandPool(m_device.logicalDevice, &cmdPoolInfo, nullptr, &m_ImGUIcommandPool));

		m_ImGUIcommandBuffers.resize(1);

		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			Initializers::commandBufferAllocateInfo(
				m_ImGUIcommandPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				static_cast<uint32_t>(m_ImGUIcommandBuffers.size()));

		CHECK_ERROR(vkAllocateCommandBuffers(m_device.logicalDevice, &cmdBufAllocateInfo, m_ImGUIcommandBuffers.data()));
	}

	m_sceneID = ImGui_ImplVulkan_AddTexture(m_offScreenPass.sampler, m_offScreenPass.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
		CHECK_ERROR(vkCreateFramebuffer(m_device.logicalDevice, &frameBufferCreateInfo, nullptr, &m_ImGUIframeBuffers[i]));
	}
}

void OgEngine::RasterizerPipeline::RescaleImGUI()
{
	ImGui::SetNextWindowSize({ static_cast<float>(m_width), static_cast<float>(m_height) });
	for (auto* m_ImGUIframeBuffer : m_ImGUIframeBuffers)
	{
		vkDestroyFramebuffer(m_device.logicalDevice, m_ImGUIframeBuffer, nullptr);
	}
	SetupImGUIFrameBuffers();
}

void OgEngine::RasterizerPipeline::RenderUI(uint32_t p_id)
{
	const VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = m_ImGUIrenderPass;
	info.framebuffer = m_ImGUIframeBuffers[p_id];
	info.renderArea.extent.width = m_width;
	info.renderArea.extent.height = m_height;
	info.clearValueCount = 1;

	VkClearValue clear;
	clear.color = { {0.3f, 0.3f, 0.3f, 1.0f} };
	clear.depthStencil = { 1.0f,0u };
	info.pClearValues = &clear;

	VkCommandBuffer cmdBuffer = BeginSingleTimeCommands();

	/*for (auto& m_image : m_images)
	{
		RaytracingPipeline::SetImageLayout(cmdBuffer, m_image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			subresourceRange,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}*/

	vkCmdBeginRenderPass(cmdBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	vkCmdEndRenderPass(cmdBuffer);

	/*for (auto& m_image : m_images)
	{
		RaytracingPipeline::SetImageLayout(cmdBuffer, m_image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			subresourceRange,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}*/

	EndSingleTimeCommands(cmdBuffer);
}

void OgEngine::RasterizerPipeline::FreeImGUIContext()
{
	for (auto* m_ImGUIframeBuffer : m_ImGUIframeBuffers)
	{
		vkDestroyFramebuffer(m_device.logicalDevice, m_ImGUIframeBuffer, nullptr);
	}

	vkFreeCommandBuffers(m_device.logicalDevice, m_ImGUIcommandPool, static_cast<uint32_t>(m_ImGUIcommandBuffers.size()),
		m_ImGUIcommandBuffers.data());

	vkDestroyPipelineCache(m_device.logicalDevice, m_pipelineCache, nullptr);
	vkDestroyRenderPass(m_device.logicalDevice, m_ImGUIrenderPass, nullptr);

	const VkResult result = vkResetDescriptorPool(m_device.logicalDevice, m_ImGUIdescriptorPool, 0);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to reset imgui descriptor pool.");

	vkDestroyDescriptorPool(m_device.logicalDevice, m_ImGUIdescriptorPool, nullptr);
	vkDestroyCommandPool(m_device.logicalDevice, m_ImGUIcommandPool, nullptr);
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
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
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
	vkCreatePipelineCache(m_device.logicalDevice, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
}