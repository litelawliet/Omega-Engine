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

		void FillData(std::vector<Vertex> p_vertices, std::vector<uint32_t> p_indices);
		void FillData(const std::shared_ptr<Mesh>& p_other);
		void SetHashID(const uint64_t p_hashID);

		[[nodiscard]] const std::vector<Vertex>& Vertices() const;
		[[nodiscard]] const std::vector<uint32_t>& Indices() const;
		[[nodiscard]] uint64_t HashID() const;
		
		Mesh& operator=(const Mesh& p_other);
		Mesh& operator=(Mesh&& p_other) noexcept;

	private:
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;

		uint64_t m_hashID{};
	};
}
