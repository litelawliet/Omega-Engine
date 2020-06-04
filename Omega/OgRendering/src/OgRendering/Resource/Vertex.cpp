#include <OgRendering/Resource/Vertex.h>

OgEngine::Vertex::Vertex()
	: position(Vector3F::zero), normal(Vector3F::zero), tangent(Vector3F::zero), texCoord(Vector2F::zero)
{
}

OgEngine::Vertex::Vertex(const Vertex& p_other)
	: position(p_other.position),
	normal(p_other.normal),
	tangent(p_other.tangent),
	texCoord(p_other.texCoord)
{
}

OgEngine::Vertex::Vertex(Vertex&& p_other) noexcept
	: position(p_other.position),
	normal(p_other.normal),
	tangent(p_other.tangent),
	texCoord(p_other.texCoord)
{
}

VkVertexInputBindingDescription OgEngine::Vertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> OgEngine::Vertex::getAttributeDescriptions()
{
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

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}

bool OgEngine::Vertex::operator<(const Vertex& p_other) const
{
	return position < p_other.position || normal < p_other.normal || tangent < p_other.tangent || texCoord <
		p_other.texCoord;
}

bool OgEngine::Vertex::operator==(const Vertex& p_other) const
{
	return position == p_other.position && normal == p_other.normal && tangent == p_other.tangent && texCoord
		== p_other.texCoord;
}

OgEngine::Vertex& OgEngine::Vertex::operator=(const Vertex& p_other)
{
	if (&p_other == this)
		return *this;

	position = p_other.position;
	normal = p_other.normal;
	tangent = p_other.tangent;
	texCoord = p_other.texCoord;

	return *this;
}

OgEngine::Vertex& OgEngine::Vertex::operator=(Vertex&& p_other) noexcept
{
	position = p_other.position;
	normal = p_other.normal;
	tangent = p_other.tangent;
	texCoord = p_other.texCoord;

	return *this;
}