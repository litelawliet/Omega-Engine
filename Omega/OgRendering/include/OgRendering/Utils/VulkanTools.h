#pragma once
#include <iostream>
#include <OgRendering/Utils/Initializers.h>
#include <glm/glm.hpp>
#include <GPM/GPM.h>

struct GeometryInstance
{
	glm::mat3x4 transform;
	uint32_t instanceId : 24;
	uint32_t mask : 8;
	uint32_t instanceOffset : 24;
	uint32_t flags : 8;
	uint64_t accelerationStructureHandle;
};

inline glm::vec3 ConvertToGLM(GPM::Vector3F p)
{
	return glm::vec3(p.x, p.y, p.z);
}

inline glm::mat4 ConvertToGLM(GPM::Matrix4F p)
{
	/*return glm::mat4(
		p(0, 0), p(1, 0), p(2, 0), p(3, 0),
		p(0, 1), p(1, 1), p(2, 1), p(3, 1),
		p(0, 2), p(1, 2), p(2, 2), p(3, 2),
		p(0, 3), p(1, 3), p(2, 3), p(3, 3)
		);*/

	return glm::mat4(
	                 p(0, 0), p(0, 1), p(0, 2), p(0, 3),
	                 p(1, 0), p(1, 1), p(1, 2), p(1, 3),
	                 p(2, 0), p(2, 1), p(2, 2), p(2, 3),
	                 p(3, 0), p(3, 1), p(3, 2), p(3, 3)
	                );
}

struct AccelerationStructure
{
    VkDeviceMemory memory{};
    VkAccelerationStructureNV accelerationStructure{};
    uint64_t handle{ 0u };
};

inline void setImageLayout(
		VkCommandBuffer         cmdbuffer,
		VkImage                 image,
		VkImageLayout           oldImageLayout,
		VkImageLayout           newImageLayout,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags    srcStageMask,
		VkPipelineStageFlags    dstStageMask)
{
	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier = Initializers::imageMemoryBarrier();
	imageMemoryBarrier.oldLayout            = oldImageLayout;
	imageMemoryBarrier.newLayout            = newImageLayout;
	imageMemoryBarrier.image                = image;
	imageMemoryBarrier.subresourceRange     = subresourceRange;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldImageLayout)
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
	switch (newImageLayout)
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
	                     cmdbuffer,
	                     srcStageMask,
	                     dstStageMask,
	                     0,
	                     0, nullptr,
	                     0, nullptr,
	                     1, &imageMemoryBarrier);
}

/*void setImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask)
{
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
}*/

struct Buffer
{
	VkDevice               device{};
	VkBuffer               buffer = nullptr;
	VkDeviceMemory         memory = nullptr;
	VkDescriptorBufferInfo descriptor{};
	VkDeviceSize           size      = 0;
	VkDeviceSize           alignment = 0;
	void*                  mapped    = nullptr;

	/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
	VkBufferUsageFlags usageFlags{};
	/** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
	VkMemoryPropertyFlags memoryPropertyFlags{};

	/**
	* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	*
	* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the buffer mapping call
	*/
	VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		return vkMapMemory(device, memory, offset, size, 0, &mapped);
	}

	/**
	* Unmap a mapped memory range
	*
	* @note Does not return a result as vkUnmapMemory can't fail
	*/
	void unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(device, memory);
			mapped = nullptr;
		}
	}

	/**
	* Attach the allocated memory block to the buffer
	*
	* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
	*
	* @return VkResult of the bindBufferMemory call
	*/
	VkResult bind(VkDeviceSize offset = 0) const
	{
		return vkBindBufferMemory(device, buffer, memory, offset);
	}

	/**
	* Setup the default descriptor for this buffer
	*
	* @param size (Optional) Size of the memory range of the descriptor
	* @param offset (Optional) Byte offset from beginning
	*
	*/
	void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		descriptor.offset = offset;
		descriptor.buffer = buffer;
		descriptor.range  = size;
	}

	/**
	* Copies the specified data to the mapped buffer
	*
	* @param data Pointer to the data to copy
	* @param size Size of the data to copy in machine units
	*
	*/
	void copyTo(void* data, VkDeviceSize size) const
	{
		assert(mapped);
		memcpy_s(mapped, size, data, size);
	}

	/**
	* Flush a memory range of the buffer to make it visible to the device
	*
	* @note Only required for non-coherent memory
	*
	* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the flush call
	*/
	VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory              = memory;
		mappedRange.offset              = offset;
		mappedRange.size                = size;
		return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
	}

	/**
	* Invalidate a memory range of the buffer to make it visible to the host
	*
	* @note Only required for non-coherent memory
	*
	* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the invalidate call
	*/
	VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory              = memory;
		mappedRange.offset              = offset;
		mappedRange.size                = size;
		return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
	}

	/**
	* Release all Vulkan resources held by this buffer
	*/
	void destroy() const
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

static VkShaderModule LoadShaderFile(const char* fileName, VkDevice device)
{
	std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

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
		moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode    = reinterpret_cast<uint32_t*>(shaderCode);

		vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule);

		delete[] shaderCode;

		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << std::endl;
		return nullptr;
	}
}
