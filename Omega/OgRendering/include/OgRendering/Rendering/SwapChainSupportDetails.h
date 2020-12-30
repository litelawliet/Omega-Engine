#pragma once
#include <OgRendering/Export.h>
#include <vector>
#include <vulkan/vulkan.h>

namespace OgEngine
{
	struct RENDERING_API SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
}
