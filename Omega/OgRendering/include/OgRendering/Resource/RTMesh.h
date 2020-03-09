#pragma once
#include <OgRendering/Export.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <OgRendering/Utils/Initializers.h>
#include <OgRendering/Utils/VulkanTools.h>

#include <vector>

/*struct VertexInfo
{
    GPM::Vector3F pos;
    GPM::Vector3F normal;
};*/
namespace OgEngine
{
    class RENDERING_API RTMesh
    {
    public:

        RTMesh() = default;
        ~RTMesh() = default;

        void PushVertex(const Vertex p_vert) { m_vertices.push_back(p_vert); }
        void PushIndex(const uint32_t p_index) { m_indices.push_back(p_index); }
        void SetIndices(std::vector<uint32_t> p_indices) { m_indices = p_indices; }

        const std::vector<Vertex>& GetVertices() const { return m_vertices; }
        const std::vector<uint32_t>& GetIndices() const { return m_indices; }

        std::vector<uint32_t> m_indices;
        std::vector<Vertex> m_vertices;
    };
}