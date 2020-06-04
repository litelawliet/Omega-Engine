#include <OgRendering/Resource/ModelRasterization.h>

#include <utility>

OgEngine::ModelRasterization::ModelRasterization(OgEngine::Mesh* p_mesh, OgEngine::Texture* p_texture, Matrix4F p_modelMatrix)
	: m_modelMatrix(std::move(p_modelMatrix)), m_mesh(p_mesh)
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

void OgEngine::ModelRasterization::UpdateModelMatrix(const GPM::Matrix4F& p_newModelMatrix)
{
	m_modelMatrix = p_newModelMatrix;
}

void OgEngine::ModelRasterization::UpdateMaterial(const MaterialRS& p_newMaterial)
{
	m_material = p_newMaterial;
}

void OgEngine::ModelRasterization::ChangeColor(const GPM::Vector4F& p_newBaseColor)
{
	m_material.color = p_newBaseColor;
}

void OgEngine::ModelRasterization::ChangeSpecularColor(const GPM::Vector4F& p_newSpecularColor)
{
	m_material.specular = p_newSpecularColor;
}

void OgEngine::ModelRasterization::ChangeEmissiveColor(const GPM::Vector4F& p_newEmissiveColor)
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

Matrix4F OgEngine::ModelRasterization::ModelMatrix() const
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
