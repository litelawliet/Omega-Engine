#pragma once
#include <OgRendering/Export.h>
#include <cstdint>
#include <vector>
#include <OgRendering/Resource/Vertex.h>

namespace OgEngine
{
	class RENDERING_API Mesh final
	{
	public:
		Mesh();
		Mesh(const Mesh& p_other);
		Mesh(Mesh&& p_other) noexcept;
		Mesh(std::vector<Vertex> p_vertices, std::vector<uint32_t> p_indices);
		~Mesh();

		void FillData(const std::shared_ptr<Mesh>& p_other);
		void SetMeshName(const std::string& p_meshName);
		void SetParentMeshName(const std::string& p_parentMeshName);
		void SetMeshFilepath(const std::string& p_meshFilepath);
		void SetHashID(const uint64_t p_hashID);
		void AddSubMesh(const std::shared_ptr<Mesh>& p_newSubMesh);
		void SetAsSubmesh(const bool p_isSubMesh);
		void SetIndexSubmesh(const int p_subMeshIndex);

		[[nodiscard]] const std::vector<OgEngine::Vertex>& Vertices() const;
		[[nodiscard]] const std::vector<uint32_t>& Indices() const;
		[[nodiscard]] std::string MeshName() const;
		[[nodiscard]] std::string ParentMeshName() const;
		[[nodiscard]] std::string MeshFilepath() const;
		[[nodiscard]] uint64_t HashID() const;
		[[nodiscard]] std::vector<std::shared_ptr<Mesh>>& SubMeshes();
		[[nodiscard]] bool IsSubMesh() const;
		[[nodiscard]] int SubMeshIndex() const;

		Mesh& operator=(const Mesh& p_other);
		Mesh& operator=(Mesh&& p_other) noexcept;

	private:
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
		std::vector<std::shared_ptr<Mesh>> m_subMeshes;
		std::string m_meshName;
		std::string m_parentMeshName;
		std::string m_meshFilepath;
		uint64_t m_hashID{};
		int m_subMeshIndex;
		bool m_isSubmesh;
	};
}
