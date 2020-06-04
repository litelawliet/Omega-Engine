#pragma once
#include <OgRendering/Export.h>
#include <OgRendering/Resource/ModelRasterization.h>

namespace OgEngine
{
	struct RENDERING_API ObjectInstance final
	{
		ModelRasterization model{};
		std::uint64_t instanceID;
		
		VkDescriptorSet descriptorSet = nullptr;

		VkBuffer uniformBuffer = nullptr;
		VkDeviceMemory uniformBufferMemory = nullptr;

		VkBuffer lightsBuffer = nullptr;
		VkDeviceMemory lightsBufferMemory = nullptr;

		VkBuffer materialsBuffer = nullptr;
		VkDeviceMemory materialsBufferMemory = nullptr;

		VkBuffer lightNumberBuffer = nullptr;
		VkDeviceMemory lightNumberBufferMemory = nullptr;

		ObjectInstance(Mesh* p_mesh = nullptr);
		~ObjectInstance() = default;

		bool operator==(const ObjectInstance& p_other) const;
		bool operator!=(const ObjectInstance& p_other) const;
	};
}
