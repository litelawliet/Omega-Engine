#include <OgCore/Components/Material.h>

OgEngine::Material::Material()
	: _color(glm::vec4(1)), _specular(glm::vec4(1)), _emissive(glm::vec4(0)), _ior(0.0f), _roughness(0.0f), _materialType(1), _texName("default.png"), _texPath("Resources/textures/default.png"), _normName("NONE"), _normPath("NONE")
{
}

OgEngine::Material::Material(const Material& p_other)
	: _color(p_other._color), _specular(p_other._emissive), _emissive(p_other._emissive), _ior(p_other._ior), _roughness(p_other._roughness), _materialType(p_other._materialType), _texName(p_other._texName), _texPath(p_other._texPath), _normName(p_other._normName), _normPath(p_other._normPath)
{
}

OgEngine::Material::Material(Material&& p_other) noexcept
	: _color(std::move(p_other._color)), _specular(std::move(p_other._emissive)), _emissive(std::move(p_other._emissive)), _ior(p_other._ior), _roughness(p_other._roughness), _materialType(p_other._materialType), _texName(std::move(p_other._texName)), _texPath(std::move(p_other._texPath)), _normName(std::move(p_other._normName)), _normPath(std::move(p_other._normPath))
{
}

OgEngine::Material::~Material() = default;

void OgEngine::Material::SetColor(const glm::vec4& p_color)
{
	_color = p_color;

	// R value
	if (_color.x < 0.0f)
	{
		_color.x = 0.0f;
	}
	else if (_color.x > 1.0f)
	{
		_color.x = 1.0f;
	}

	// G value
	if (_color.y < 0.0f)
	{
		_color.y = 0.0f;
	}
	else if (_color.y > 1.0f)
	{
		_color.y = 1.0f;
	}

	// B value
	if (_color.z < 0.0f)
	{
		_color.z = 0.0f;
	}
	else if (_color.z > 1.0f)
	{
		_color.z = 1.0f;
	}

	// A value
	if (_color.w < 0.0f)
	{
		_color.w = 0.0f;
	}
	else if (_color.w > 1.0f)
	{
		_color.w = 1.0f;
	}
}

void OgEngine::Material::SetSpecular(const glm::vec4& p_specular)
{
	_specular = p_specular;

	// R value
	if (_specular.x < 0.0f)
	{
		_specular.x = 0.0f;
	}
	else if (_specular.x > 1.0f)
	{
		_specular.x = 1.0f;
	}

	// G value
	if (_specular.y < 0.0f)
	{
		_specular.y = 0.0f;
	}
	else if (_specular.y > 1.0f)
	{
		_specular.y = 1.0f;
	}

	// B value
	if (_specular.z < 0.0f)
	{
		_specular.z = 0.0f;
	}
	else if (_specular.z > 1.0f)
	{
		_specular.z = 1.0f;
	}

	// A value
	if (_specular.w < 0.0f)
	{
		_specular.w = 0.0f;
	}
	else if (_specular.w > 1.0f)
	{
		_specular.w = 1.0f;
	}
}

void OgEngine::Material::SetRoughness(const float p_roughness)
{
	_roughness = p_roughness;

	if (_roughness < 0.0f)
	{
		_roughness = 0.0f;
	}
	else if (_roughness > 1.0f)
	{
		_roughness = 1.0f;
	}
}

void OgEngine::Material::SetTextureID(const std::string& p_texID, const std::string& p_texPath)
{
	_texName = p_texID;
	_texPath = p_texPath;
}

void OgEngine::Material::SetNormalMapID(const std::string& p_normID, const std::string& p_normPath)
{
	_normName = p_normID;
	_normPath = p_normPath;
}

void OgEngine::Material::SetIOR(const float p_ior)
{
	_ior = p_ior;

	if (_ior < 0.0f)
	{
		_ior = 0.0f;
	}
	else if (_ior > 2.0f)
	{
		_ior = 2.0f;
	}
}

void OgEngine::Material::SetEmissive(const glm::vec4& p_emissive)
{
	_emissive = p_emissive;

	// R value
	if (_emissive.x < 0.0f)
	{
		_emissive.x = 0.0f;
	}
	else if (_emissive.x > 1.0f)
	{
		_emissive.x = 1.0f;
	}

	// G value
	if (_emissive.y < 0.0f)
	{
		_emissive.y = 0.0f;
	}
	else if (_emissive.y > 1.0f)
	{
		_emissive.y = 1.0f;
	}

	// B value
	if (_emissive.z < 0.0f)
	{
		_emissive.z = 0.0f;
	}
	else if (_emissive.z > 1.0f)
	{
		_emissive.z = 1.0f;
	}

	// A value
	if (_emissive.w < 0.0f)
	{
		_emissive.w = 0.0f;
	}
	else if (_emissive.w > 1.0f)
	{
		_emissive.w = 1.0f;
	}
}

void OgEngine::Material::SetType(const int p_type)
{
	_materialType = p_type;

	if (_materialType < 0)
	{
		_materialType = 0;
	}
	else if (_materialType > 5)
	{
		_materialType = 5;
	}
}

void OgEngine::Material::SetLocalTransform(Transform& p_transform)
{
	m_materialTransform = &(p_transform);
}

std::string OgEngine::Material::Serialize(const int p_depth) const
{
	return std::string(DepthIndent(p_depth) + "<Material>\n"
		+ DepthIndent(p_depth + 1) + "<color>" + std::to_string(_color.x) + ";" + std::to_string(_color.y) + ";" + std::to_string(_color.z) + ";" + std::to_string(_color.w) + "</color>\n"
		+ DepthIndent(p_depth + 1) + "<specular>" + std::to_string(_specular.x) + ";" + std::to_string(_specular.y) + ";" + std::to_string(_specular.z) + ";" + std::to_string(_specular.w) + "</specular>\n"
		+ DepthIndent(p_depth + 1) + "<emissive>" + std::to_string(_emissive.x) + ";" + std::to_string(_emissive.y) + ";" + std::to_string(_emissive.z) + ";" + std::to_string(_emissive.w) + "</emissive>\n"
		+ DepthIndent(p_depth + 1) + "<ior>" + std::to_string(_ior) + "</ior>\n"
		+ DepthIndent(p_depth + 1) + "<roughness>" + std::to_string(_roughness) + "</roughness>\n"
		+ DepthIndent(p_depth + 1) + "<type>" + std::to_string(_materialType) + "</type>\n"
		+ DepthIndent(p_depth + 1) + "<textureName>" + std::string(_texName) + "</textureName>\n"
		+ DepthIndent(p_depth + 1) + "<texturePath>" + std::string(_texPath) + "</texturePath>\n"
		+ DepthIndent(p_depth + 1) + "<normalName>" + std::string(_normName) + "</normalName>\n"
		+ DepthIndent(p_depth + 1) + "<normalPath>" + std::string(_normPath) + "</normalPath>\n"
		+ DepthIndent(p_depth) + "</Material>\n");
}

OgEngine::Material& OgEngine::Material::operator=(const Material& p_other)
{
	if (&p_other == this)
		return *this;

	_color = p_other._color;
	_specular = p_other._specular;
	_emissive = p_other._emissive;
	_ior = p_other._ior;
	_roughness = p_other._roughness;
	_materialType = p_other._materialType;
	_texName = p_other._texName;
	_texPath = p_other._texPath;
	_normName = p_other._normName;
	_normPath = p_other._normPath;

	return *this;
}

OgEngine::Material& OgEngine::Material::operator=(Material&& p_other) noexcept
{
	_color = p_other._color;
	_specular = p_other._specular;
	_emissive = p_other._emissive;
	_ior = p_other._ior;
	_roughness = p_other._roughness;
	_materialType = p_other._materialType;
	_texName = std::move(p_other._texName);
	_texPath = std::move(p_other._texPath);
	_normName = std::move(p_other._normName);
	_normPath = std::move(p_other._normPath);

	return *this;
}

std::string OgEngine::Material::DepthIndent(const int p_depth)
{
	std::string depthCode;
	for (auto i = 0; i < p_depth; ++i)
	{
		depthCode += "\t";
	}

	return depthCode;
}
