#include <OgRendering/Resource/ModelRasterization.h>

OgEngine::ModelRasterization::ModelRasterization(OgEngine::Mesh* p_mesh, OgEngine::Texture* p_texture, const glm::mat4 p_modelMatrix)
	: m_modelMatrix(p_modelMatrix), m_mesh(p_mesh)
{
}

OgEngine::ModelRasterization::~ModelRasterization()
{
	m_mesh = nullptr;
}

void OgEngine::ModelRasterization::SetMesh(OgEngine::Mesh* p_newMesh)
{
	m_mesh = p_newMesh;
}

void OgEngine::ModelRasterization::SetTexture(OgEngine::Texture* p_newTexture)
{
	m_texture = p_newTexture;
}

void OgEngine::ModelRasterization::UpdateModelMatrix(const glm::mat4& p_worldMatrix)
{
	m_modelMatrix = p_worldMatrix;
}

void OgEngine::ModelRasterization::UpdateMaterial(const MaterialRS& p_newMaterial)
{
	m_material = p_newMaterial;
}

void OgEngine::ModelRasterization::ChangeColor(const glm::vec4& p_newBaseColor)
{
	m_material.color = p_newBaseColor;
}

void OgEngine::ModelRasterization::ChangeSpecularColor(const glm::vec4& p_newSpecularColor)
{
	m_material.specular = p_newSpecularColor;
}

void OgEngine::ModelRasterization::ChangeEmissiveColor(const glm::vec4& p_newEmissiveColor)
{
	m_material.emissive = p_newEmissiveColor;
}

OgEngine::Mesh* OgEngine::ModelRasterization::Mesh() const
{
	return m_mesh;
}

OgEngine::Texture* OgEngine::ModelRasterization::Texture() const
{
	return m_texture;
}

glm::mat4 OgEngine::ModelRasterization::ModelMatrix() const
{
	return m_modelMatrix;
}

OgEngine::MaterialRS OgEngine::ModelRasterization::Material() const
{
	return m_material;
}

bool OgEngine::ModelRasterization::operator==(const ModelRasterization& p_other) const
{
	return m_mesh == p_other.m_mesh && m_modelMatrix == p_other.m_modelMatrix && m_material == p_other.m_material;
}

bool OgEngine::ModelRasterization::operator!=(const ModelRasterization& p_other) const
{
	return m_mesh != p_other.m_mesh || m_modelMatrix != p_other.m_modelMatrix || m_material != p_other.m_material;
}
