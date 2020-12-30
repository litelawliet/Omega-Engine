#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

struct TextureData
{
	VkImage img;
	VkDescriptorImageInfo info;
	VkDeviceMemory memory;
	VkSampler sampler;
	VkImageView view;
	uint32_t mipLevels;
};