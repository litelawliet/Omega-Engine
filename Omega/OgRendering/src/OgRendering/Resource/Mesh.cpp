#include <OgRendering/Resource/Mesh.h>

#include <utility>

OgEngine::Mesh::Mesh()
= default;

OgEngine::Mesh::Mesh(const Mesh& p_other)
{
	if (&p_other == this)
	{
		return;
	}

	m_vertices = p_other.m_vertices;
	m_indices  = p_other.m_indices;
	m_hashID   = p_other.m_hashID;
}

OgEngine::Mesh::Mesh(Mesh&& p_other) noexcept
{
	m_vertices = p_other.m_vertices;
	m_indices  = p_other.m_indices;
}

OgEngine::Mesh::Mesh(std::vector<Vertex> p_vertices, std::vector<uint32_t> p_indices)
	: m_vertices{std::move(p_vertices)}, m_indices{std::move(p_indices)}
{
}

OgEngine::Mesh::~Mesh()
{
	m_indices.clear();
	m_vertices.clear();
}

void OgEngine::Mesh::FillData(std::vector<Vertex> p_vertices, std::vector<uint32_t> p_indices)
{
	m_vertices = std::move(p_vertices);
	m_indices  = std::move(p_indices);
}

void OgEngine::Mesh::FillData(const std::shared_ptr<Mesh>& p_other)
{
	if (p_other != nullptr)
	{
		m_vertices = std::move(p_other->m_vertices);
		m_indices = std::move(p_other->m_indices);
	}
}

const std::vector<Vertex>& OgEngine::Mesh::Vertices() const
{
	return m_vertices;
}

const std::vector<uint32_t>& OgEngine::Mesh::Indices() const
{
	return m_indices;
}

uint64_t OgEngine::Mesh::HashID() const
{
	return m_hashID;
}

OgEngine::Mesh& OgEngine::Mesh::operator=(const Mesh& p_other)
{
	if (&p_other == this)
		return *this;

	m_vertices = p_other.m_vertices;
	m_indices  = p_other.m_indices;
	m_hashID   = p_other.m_hashID;

	return *this;
}

OgEngine::Mesh& OgEngine::Mesh::operator=(Mesh&& p_other) noexcept
{
	m_vertices = p_other.m_vertices;
	m_indices  = p_other.m_indices;
	m_hashID   = p_other.m_hashID;

	return *this;
}

void OgEngine::Mesh::SetHashID(const uint64_t p_hashID)
{
	m_hashID = p_hashID;
}
