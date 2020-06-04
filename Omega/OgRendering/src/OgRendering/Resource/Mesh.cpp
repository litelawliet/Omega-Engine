#include <OgRendering/Resource/Mesh.h>

#include <utility>

OgEngine::Mesh::Mesh()
= default;

OgEngine::Mesh::Mesh(const Mesh & p_other)
{
	if (&p_other == this)
	{
		return;
	}

	m_vertices = p_other.m_vertices;
	m_indices = p_other.m_indices;
	m_hashID = p_other.m_hashID;
	m_meshName = p_other.m_meshName;
	m_parentMeshName = p_other.m_parentMeshName;
	m_meshFilepath = p_other.m_meshFilepath;
	m_subMeshes = p_other.m_subMeshes;
	m_subMeshIndex = p_other.m_subMeshIndex;
	m_isSubmesh = p_other.m_isSubmesh;
}

OgEngine::Mesh::Mesh(Mesh && p_other) noexcept
{
	m_vertices = std::move(p_other.m_vertices);
	m_indices = std::move(p_other.m_indices);
	m_hashID = p_other.m_hashID;
	m_meshName = std::move(p_other.m_meshName);
	m_parentMeshName = std::move(p_other.m_parentMeshName);
	m_meshFilepath = std::move(p_other.m_meshFilepath);
	m_subMeshes = std::move(p_other.m_subMeshes);
	m_subMeshIndex = p_other.m_subMeshIndex;
	m_isSubmesh = p_other.m_isSubmesh;
}

OgEngine::Mesh::Mesh(std::vector<Vertex> p_vertices, std::vector<uint32_t> p_indices)
	:m_vertices(std::move(p_vertices)), m_indices(std::move(p_indices))
{
}

OgEngine::Mesh::~Mesh()
{
	m_indices.clear();
	m_vertices.clear();
}

void OgEngine::Mesh::FillData(const std::shared_ptr<Mesh> & p_other)
{
	if (p_other != nullptr)
	{
		m_vertices = std::move(p_other->m_vertices);
		m_indices = std::move(p_other->m_indices);
		m_subMeshes = std::move(p_other->m_subMeshes);
	}
}

void OgEngine::Mesh::SetMeshName(const std::string & p_meshName)
{
	m_meshName = p_meshName;
}

void OgEngine::Mesh::SetParentMeshName(const std::string& p_parentMeshName)
{
	m_parentMeshName = p_parentMeshName;
}

void OgEngine::Mesh::SetMeshFilepath(const std::string & p_meshFilepath)
{
	m_meshFilepath = p_meshFilepath;
}

const std::vector<OgEngine::Vertex>& OgEngine::Mesh::Vertices() const
{
	return m_vertices;
}

const std::vector<uint32_t>& OgEngine::Mesh::Indices() const
{
	return m_indices;
}

std::string OgEngine::Mesh::MeshName() const
{
	return m_meshName;
}

std::string OgEngine::Mesh::ParentMeshName() const
{
	return m_parentMeshName;
}

std::string OgEngine::Mesh::MeshFilepath() const
{
	return m_meshFilepath;
}

uint64_t OgEngine::Mesh::HashID() const
{
	return m_hashID;
}

std::vector<std::shared_ptr<OgEngine::Mesh>>& OgEngine::Mesh::SubMeshes()
{
	return m_subMeshes;
}

bool OgEngine::Mesh::IsSubMesh() const
{
	return m_isSubmesh;
}

int OgEngine::Mesh::SubMeshIndex() const
{
	return m_subMeshIndex;
}

OgEngine::Mesh& OgEngine::Mesh::operator=(const Mesh & p_other)
{
	if (&p_other == this)
		return *this;

	m_vertices = p_other.m_vertices;
	m_indices = p_other.m_indices;
	m_hashID = p_other.m_hashID;
	m_meshName = p_other.m_meshName;
	m_parentMeshName = p_other.m_parentMeshName;
	m_meshFilepath = p_other.m_meshFilepath;
	m_subMeshes = p_other.m_subMeshes;
	m_subMeshIndex = p_other.m_subMeshIndex;
	m_isSubmesh = p_other.m_isSubmesh;

	return *this;
}

OgEngine::Mesh& OgEngine::Mesh::operator=(Mesh && p_other) noexcept
{
	m_vertices = std::move(p_other.m_vertices);
	m_indices = std::move(p_other.m_indices);
	m_hashID = p_other.m_hashID;
	m_meshName = std::move(p_other.m_meshName);
	m_parentMeshName = std::move(p_other.m_parentMeshName);
	m_meshFilepath = std::move(p_other.m_meshFilepath);
	m_subMeshes = std::move(p_other.m_subMeshes);
	m_subMeshIndex = p_other.m_subMeshIndex;
	m_isSubmesh = p_other.m_isSubmesh;
	
	return *this;
}

void OgEngine::Mesh::SetHashID(const uint64_t p_hashID)
{
	m_hashID = p_hashID;
}

void OgEngine::Mesh::AddSubMesh(const std::shared_ptr<Mesh>& p_newSubMesh)
{
	m_subMeshes.emplace_back(p_newSubMesh);
}

void OgEngine::Mesh::SetAsSubmesh(const bool p_isSubMesh)
{
	m_isSubmesh = p_isSubMesh;
}

void OgEngine::Mesh::SetIndexSubmesh(const int p_subMeshIndex)
{
	m_subMeshIndex = p_subMeshIndex;
}
