#include <OgCore/Components/ModelRS.h>
#include <OgCore/Components/Transform.h>
#include <OgRendering/Managers/ResourceManager.h>

OgEngine::ModelRS::ModelRS()
= default;

OgEngine::ModelRS::ModelRS(std::string_view p_meshName)
{
	m_mesh = ResourceManager::Get<OgEngine::Mesh>(p_meshName);
	m_meshName = p_meshName;
	if (m_mesh)
	{
		m_meshFilepath = m_mesh->MeshFilepath();
		m_parentMeshName = m_mesh->ParentMeshName();
	}
	else
	{
		m_meshFilepath = "";
		m_parentMeshName = "";
	}
}

OgEngine::ModelRS::ModelRS(OgEngine::Mesh * p_mesh)
{
	m_mesh = p_mesh;
	if (p_mesh)
	{
		m_meshName = p_mesh->MeshName();
		m_parentMeshName = m_mesh->ParentMeshName();
		m_meshFilepath = p_mesh->MeshFilepath();
	}
	else
	{
		m_meshName = "";
		m_parentMeshName = "";
		m_meshFilepath = "";
	}
}

OgEngine::ModelRS::ModelRS(const ModelRS & p_other)
{
	m_mesh = p_other.m_mesh;
	m_material = p_other.m_material;
	m_meshName = p_other.m_meshName;
	m_parentMeshName = p_other.m_parentMeshName;
	m_meshFilepath = p_other.m_meshFilepath;
}

OgEngine::ModelRS::ModelRS(ModelRS && p_other) noexcept
{
	m_mesh = p_other.m_mesh;
	m_material = std::move(p_other.m_material);
	m_meshName = std::move(p_other.m_meshName);
	m_parentMeshName = std::move(p_other.m_parentMeshName);
	m_meshFilepath = std::move(p_other.m_meshFilepath);
}

void OgEngine::ModelRS::SetMesh(Mesh * p_mesh)
{
	m_mesh = p_mesh;
	if (p_mesh)
	{
		m_meshName = p_mesh->MeshName();
		m_parentMeshName = p_mesh->ParentMeshName();
		m_meshFilepath = p_mesh->MeshFilepath();
	}
	else
	{
		m_meshName = "";
		m_parentMeshName = "";
		m_meshFilepath = "";
	}
}

void OgEngine::ModelRS::SetMesh(std::string_view p_meshName)
{
	m_mesh = ResourceManager::Get<Mesh>(p_meshName);
	m_meshName = p_meshName;
	if (m_mesh)
	{
		m_parentMeshName = m_mesh->ParentMeshName();
		m_meshFilepath = m_mesh->MeshFilepath();
	}
	else
	{
		m_parentMeshName = "";
		m_meshFilepath = "";
	}
}

void OgEngine::ModelRS::SetMaterial(const OgEngine::Material & p_material)
{
	m_material.SetColor(p_material.color);
	m_material.SetSpecular(p_material.specular);
	m_material.SetEmissive(p_material.emissive);
	m_material.SetIOR(p_material.ior);
	m_material.SetRoughness(p_material.roughness);
	m_material.SetType(p_material.type);
	m_material.SetTextureID(p_material.texName, p_material.texPath);
	m_material.SetNormalMapID(p_material.normName, p_material.normPath);
}

OgEngine::Mesh* OgEngine::ModelRS::GetMesh() const
{
	return m_mesh;
}

OgEngine::Material& OgEngine::ModelRS::Material()
{
	return m_material;
}

const std::string& OgEngine::ModelRS::MeshName() const
{
	return m_meshName;
}

const std::string& OgEngine::ModelRS::ParentMeshName() const
{
	return m_parentMeshName;
}

const std::string& OgEngine::ModelRS::MeshFilepath() const
{
	return m_meshFilepath;
}

glm::mat4 OgEngine::ModelRS::ModelMatrix() const
{
	return m_meshTransform ? m_meshTransform->worldMatrix : glm::mat4(1.0f);
}

OgEngine::ModelRS& OgEngine::ModelRS::operator=(const ModelRS & p_other)
{
	if (&p_other == this)
		return *this;

	m_mesh = p_other.m_mesh;
	m_material = p_other.m_material;
	m_meshName = p_other.m_meshName;
	m_parentMeshName = p_other.m_parentMeshName;
	m_meshFilepath = p_other.m_meshFilepath;

	return *this;
}

OgEngine::ModelRS& OgEngine::ModelRS::operator=(ModelRS && p_other) noexcept
{
	m_mesh = p_other.m_mesh;
	m_material = p_other.m_material;
	m_meshName = std::move(p_other.m_meshName);
	m_parentMeshName = std::move(p_other.m_parentMeshName);
	m_meshFilepath = std::move(p_other.m_meshFilepath);

	return *this;
}

void OgEngine::ModelRS::SetLocalTransform(Transform & p_transform)
{
	m_meshTransform = &(p_transform);
}

std::string OgEngine::ModelRS::Serialize(const int p_depth) const
{
	const bool isSubMesh = m_mesh ? m_mesh->IsSubMesh() : false;
	const uint32_t indexSubMesh = m_mesh ? m_mesh->SubMeshIndex() : 0;

	return std::string(DepthIndent(p_depth) + "<Model>\n"
		+ DepthIndent(p_depth + 1) + "<parentMeshName>" + m_parentMeshName + "</parentMeshName>\n"
		+ DepthIndent(p_depth + 1) + "<meshName>" + m_meshName + "</meshName>\n"
		+ DepthIndent(p_depth + 1) + "<meshFilepath>" + m_meshFilepath + "</meshFilepath>\n"
		+ DepthIndent(p_depth + 1) + "<subMesh>" + std::to_string(isSubMesh) + "</subMesh>\n"
		+ DepthIndent(p_depth + 1) + "<indexSubMesh>" + std::to_string(indexSubMesh) + "</indexSubMesh>\n"
		+ m_material.Serialize(p_depth + 1)
		+ DepthIndent(p_depth) + "</Model>\n");
}

std::string OgEngine::ModelRS::DepthIndent(const int p_depth)
{
	std::string depthCode;
	for (auto i = 0; i < p_depth; ++i)
	{
		depthCode += "\t";
	}

	return depthCode;
}
