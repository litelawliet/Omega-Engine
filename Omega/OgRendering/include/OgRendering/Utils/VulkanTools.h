#pragma once
#include <iostream>
#include <OgRendering/Utils/Initializers.h>
#include <glm/glm.hpp>
#include <GPM/GPM.h>
#include <fstream>

struct GeometryInstance
{
	glm::mat3x4 transform;
	uint32_t instanceId : 24;
	uint32_t mask : 8;
	uint32_t instanceOffset : 24;
	uint32_t flags : 8;
	uint64_t accelerationStructureHandle;
};

enum TEXTURE_TYPE : std::uint8_t
{
	TEXTURE = 0u,
	NORMAL = 1u
};

/**
 * @brief Convert a GPM::Vector3<float> to a glm::vec3<float>
 * @param p_vector The vector to convert : GPM::Vector3<float>
 * @return The converted vector 3 of type glm::vec3<float>
 */
inline glm::vec3 ConvertToGLM(const GPM::Vector3F& p_vector)
{
	return glm::vec3(p_vector.x, p_vector.y, p_vector.z);
}

/**
 * @brief Convert a GPM::Matrix4<float> to a glm::mat4<float>
 * @param p_matrix The matrix to convert : GPM::Matrix4<float>
 * @return The converted matrix 4x4 of type glm::mat4<float>
 */
inline glm::mat4 ConvertToGLM(GPM::Matrix4F p_matrix)
{
	/*return glm::mat4(
		p_matrix(0, 0), p_matrix(1, 0), p_matrix(2, 0), p_matrix(3, 0),
		p_matrix(0, 1), p_matrix(1, 1), p_matrix(2, 1), p_matrix(3, 1),
		p_matrix(0, 2), p_matrix(1, 2), p_matrix(2, 2), p_matrix(3, 2),
		p_matrix(0, 3), p_matrix(1, 3), p_matrix(2, 3), p_matrix(3, 3)
		);*/

	return glm::mat4(
		p_matrix(0, 0), p_matrix(0, 1), p_matrix(0, 2), p_matrix(0, 3),
		p_matrix(1, 0), p_matrix(1, 1), p_matrix(1, 2), p_matrix(1, 3),
		p_matrix(2, 0), p_matrix(2, 1), p_matrix(2, 2), p_matrix(2, 3),
		p_matrix(3, 0), p_matrix(3, 1), p_matrix(3, 2), p_matrix(3, 3)
	);
}

struct AccelerationStructure
{
	VkDeviceMemory memory{};
	VkAccelerationStructureNV accelerationStructure{};
	uint64_t handle{ 0u };
};

/**
 * @brief Change the layout of a VkImage to a new layout
 * @param p_commandBuffer The command buffer to use to change the layout
 * @param p_image The image on which we modify the layout
 * @param p_oldImageLayout The old layout of the image
 * @param p_newImageLayout The new layout of the image
 * @param p_subresourceRange The resource range to use
 * @param p_srcStageMask Type of flag used for the source
 * @param p_dstStageMask Type of flag to use as destination
 */
inline void setImageLayout(
	VkCommandBuffer         p_commandBuffer,
	VkImage                 p_image,
	const VkImageLayout           p_oldImageLayout,
	const VkImageLayout           p_newImageLayout,
	const VkImageSubresourceRange p_subresourceRange,
	const VkPipelineStageFlags    p_srcStageMask,
	const VkPipelineStageFlags    p_dstStageMask)
{
	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier = Initializers::imageMemoryBarrier();
	imageMemoryBarrier.oldLayout = p_oldImageLayout;
	imageMemoryBarrier.newLayout = p_newImageLayout;
	imageMemoryBarrier.image = p_image;
	imageMemoryBarrier.subresourceRange = p_subresourceRange;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (p_oldImageLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		imageMemoryBarrier.srcAccessMask = 0;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source 
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (p_newImageLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (imageMemoryBarrier.srcAccessMask == 0)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		p_commandBuffer,
		p_srcStageMask,
		p_dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
}

struct Buffer
{
	VkDevice               device{};
	VkBuffer               buffer = nullptr;
	VkDeviceMemory         memory = nullptr;
	VkDescriptorBufferInfo descriptor{};
	VkDeviceSize           size = 0;
	VkDeviceSize           alignment = 0;
	void* mapped = nullptr;

	/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
	VkBufferUsageFlags usageFlags{};
	/** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
	VkMemoryPropertyFlags memoryPropertyFlags{};

	/**
	* @brief Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	* @return VkResult of the buffer mapping call
	*/
	VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		return vkMapMemory(device, memory, offset, size, 0, &mapped);
	}

	/**
	* @brief Unmap a mapped memory range
	* @note Does not return a result as vkUnmapMemory can't fail
	*/
	void Unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(device, memory);
			mapped = nullptr;
		}
	}

	/**
	* @brief Attach the allocated memory block to the buffer
	* @param p_offset (Optional) Byte offset (from the beginning) for the memory region to bind
	* @return VkResult of the bindBufferMemory call
	*/
	[[nodiscard]] VkResult Bind(VkDeviceSize p_offset = 0) const
	{
		return vkBindBufferMemory(device, buffer, memory, p_offset);
	}

	/**
	* @brief Setup the default descriptor for this buffer
	* @param p_size (Optional) Size of the memory range of the descriptor
	* @param p_offset (Optional) Byte offset from beginning
	*
	*/
	void SetupDescriptor(VkDeviceSize p_size = VK_WHOLE_SIZE, VkDeviceSize p_offset = 0)
	{
		descriptor.offset = p_offset;
		descriptor.buffer = buffer;
		descriptor.range = p_size;
	}

	/**
	* @brief Copies the specified data to the mapped buffer
	* @param p_data Pointer to the data to copy
	* @param p_size Size of the data to copy in machine units
	*
	*/
	void CopyTo(void* p_data, VkDeviceSize p_size) const
	{
		assert(mapped);
		memcpy_s(mapped, p_size, p_data, p_size);
	}

	/**
	* @brief Flush a memory range of the buffer to make it visible to the device
	* @note Only required for non-coherent memory
	* @param p_size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
	* @param p_offset (Optional) Byte offset from beginning
	* @return VkResult of the flush call
	*/
	[[nodiscard]] VkResult Flush(VkDeviceSize p_size = VK_WHOLE_SIZE, VkDeviceSize p_offset = 0) const
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = p_offset;
		mappedRange.size = p_size;
		return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
	}

	/**
	* @brief Invalidate a memory range of the buffer to make it visible to the host
	* @note Only required for non-coherent memory
	* @param p_size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
	* @param p_offset (Optional) Byte offset from beginning
	* @return VkResult of the invalidate call
	*/
	[[nodiscard]] VkResult Invalidate(VkDeviceSize p_size = VK_WHOLE_SIZE, VkDeviceSize p_offset = 0) const
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = p_offset;
		mappedRange.size = p_size;
		return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
	}

	/**
	* @brief Release all Vulkan resources held by this buffer
	*/
	void Destroy() const
	{
		if (buffer)
		{
			vkDestroyBuffer(device, buffer, nullptr);
		}
		if (memory)
		{
			vkFreeMemory(device, memory, nullptr);
		}
		if (mapped)
		{
			delete mapped;
		}
	}
};

/**
 * @brief Load a shader from a source file
 * @param p_fileName The source file
 * @param p_device The device to link the shader with
 * @return The VkShaderModule created
 */
static VkShaderModule LoadShaderFile(const char* p_fileName, VkDevice p_device)
{
	std::ifstream is(p_fileName, std::ios::binary | std::ios::in | std::ios::ate);

	if (is.is_open())
	{
		const size_t size = is.tellg();
		is.seekg(0, std::ios::beg);
		char* shaderCode = new char[size];
		is.read(shaderCode, size);
		is.close();

		assert(size > 0);

		VkShaderModule           shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode);

		vkCreateShaderModule(p_device, &moduleCreateInfo, nullptr, &shaderModule);

		delete[] shaderCode;

		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << p_fileName << "\"" << std::endl;
		return nullptr;
	}
}
