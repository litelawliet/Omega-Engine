#pragma once
#include <OgRendering/Export.h>
#include <array>
#include <GPM/GPM.h>
#include <cstddef>
#include <vulkan/vulkan.h>

struct RENDERING_API Vertex {
	alignas(alignof(GPM::Vector3F)) GPM::Vector3F position{};
	alignas(alignof(GPM::Vector3F)) GPM::Vector3F normal{};
	alignas(alignof(GPM::Vector3F)) GPM::Vector3F tangent{};
	alignas(alignof(GPM::Vector2F)) GPM::Vector2F texCoord{};
    alignas(alignof(int)) int dummy{ 0 };

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tangent);

		/*attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, bitangent);*/

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return position == other.position && normal == other.normal && tangent == other.tangent && texCoord == other.texCoord;
	}
};