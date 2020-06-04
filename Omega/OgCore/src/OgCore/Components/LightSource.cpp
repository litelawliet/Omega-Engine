#include <OgCore/Components/LightSource.h>
#include <OgCore/Components/Transform.h>

OgEngine::LightSource::LightSource(const LightSource& p_other)
{
	color = p_other.color;
	lightType = p_other.lightType;
	direction = p_other.direction;
}

OgEngine::LightSource::LightSource(LightSource&& p_other) noexcept
{
	color = p_other.color;
	lightType = p_other.lightType;
	direction = p_other.direction;
}

void OgEngine::LightSource::SetLocalTransform(Transform& p_transform)
{
	m_lightTransform = &(p_transform);
}

std::string OgEngine::LightSource::Serialize(const int p_depth) const
{
	return std::string(DepthIndent(p_depth) + "<LightSource>\n"
		+ DepthIndent(p_depth + 1) + "<color>" + std::to_string(color.x) + ";" + std::to_string(color.y) + ";" + std::to_string(color.z) + ";" + std::to_string(color.w) + "</color>\n"
		+ DepthIndent(p_depth + 1) + "<direction>" + std::to_string(direction .x) + ";" + std::to_string(direction.y) + ";" + std::to_string(direction.z) + ";" + std::to_string(direction.w) + "</direction>\n"
		+ DepthIndent(p_depth + 1) + "<lightType>" + std::to_string(lightType) + "</lightType>\n"
		+ DepthIndent(p_depth) + "</LightSource>\n");
}

OgEngine::LightSource& OgEngine::LightSource::operator=(const LightSource& p_other)
{
	if (&p_other == this)
		return *this;

	color = p_other.color;
	lightType = p_other.lightType;
	direction = p_other.direction;

	return *this;
}

OgEngine::LightSource& OgEngine::LightSource::operator=(LightSource&& p_other) noexcept
{
	color = p_other.color;
	lightType = p_other.lightType;
	direction = p_other.direction;

	return *this;
}

std::string OgEngine::LightSource::DepthIndent(const int p_depth)
{
	std::string depthCode;
	for (auto i = 0; i < p_depth; ++i)
	{
		depthCode += "\t";
	}

	return depthCode;
}
