#include <OgCore/Components/ModelRS.h>

#include <OgRendering/Managers/ResourceManager.h>

OgEngine::ModelRS::ModelRS()
{
	//m_transform = &Scene::
}

OgEngine::ModelRS::ModelRS(std::string_view p_meshName)
{
	m_mesh = ResourceManager::Get<OgEngine::Mesh>(p_meshName);
}

OgEngine::ModelRS::ModelRS(std::shared_ptr<OgEngine::Mesh> p_mesh)
{
	m_mesh = p_mesh;
}

OgEngine::ModelRS::ModelRS(const ModelRS& p_other)
{
	m_mesh = p_other.m_mesh;
}

OgEngine::ModelRS::ModelRS(ModelRS&& p_other) noexcept
{
	m_mesh = p_other.m_mesh;
}

void OgEngine::ModelRS::SetMesh(const std::shared_ptr<OgEngine::Mesh>& p_mesh)
{
	m_mesh = p_mesh;
}

void OgEngine::ModelRS::SetMesh(std::string_view p_meshName)
{
	m_mesh = ResourceManager::Get<OgEngine::Mesh>(p_meshName);
}

void OgEngine::ModelRS::SetMaterial(const OgEngine::Material& p_material)
{
	m_material.SetColor(p_material.Color());
	m_material.SetRoughness(p_material.Roughness());
	m_material.SetType(p_material.Type());
}

const std::shared_ptr<OgEngine::Mesh>& OgEngine::ModelRS::Mesh() const
{
	return m_mesh;
}

OgEngine::Material& OgEngine::ModelRS::Material()
{
	return m_material;
}

GPM::Matrix4F OgEngine::ModelRS::ModelMatrix() const
{
	return m_meshTransform ? m_meshTransform->worldMatrix : Matrix4F::identity;
}

OgEngine::ModelRS& OgEngine::ModelRS::operator=(const ModelRS& p_model)
{
	if (&p_model == this)
		return *this;
	 
	m_mesh = p_model.m_mesh;

	return *this;
}

OgEngine::ModelRS& OgEngine::ModelRS::operator=(ModelRS&& p_model) noexcept
{
	m_mesh = p_model.m_mesh;

	return *this;
}

void OgEngine::ModelRS::SetLocalTransform(Transform& p_transform)
{
	m_meshTransform = &(p_transform);
}
