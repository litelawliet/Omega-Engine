#pragma once
#include <OgRendering/Export.h>

#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <OgRendering/Resource/Mesh.h>
#include <OgRendering/Resource/RTMesh.h>
#include <OgRendering/Utils/VulkanTools.h>

namespace OgEngine
{
	class RENDERING_API Model final
	{
	public:
		Model() = default;
		Model(OgEngine::Mesh* p_mesh, const bool useRT)
		{
			m_mesh = p_mesh;

			m_geometry.instanceId = 0;
			m_geometry.mask = 0xff;
			m_geometry.instanceOffset = 0;
			m_geometry.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		}

		~Model()
		{
		}

		void Translate(const glm::vec3& tr)
		{
			m_pos += tr;
			UpdateTransform();
		}

		void Rotate(const glm::vec3& rotation)
		{
			m_rot += rotation;
			UpdateTransform();
		}

		void Scale(const glm::vec3& scale)
		{
			m_geometry.transform[0].x = scale.x;
			m_geometry.transform[1].y = scale.y;
			m_geometry.transform[2].z = scale.z;
		}
		void UpdateTransform()
		{
			glm::vec3 position = m_pos;
			glm::vec3 rotation = m_rot;

			glm::mat4 transMat = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 rotationMat = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1, 0, 0));
			rotationMat *= glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0, 1, 0));
			rotationMat *= glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0, 0, 1));
			glm::mat4 convertedMat = transMat * rotationMat;
			m_geometry.transform = convertedMat;

		}

		inline void SetTransform(const glm::mat4& ta)
		{
			m_geometry.transform = glm::transpose(ta);
		}

		glm::vec3 m_pos;
		glm::vec3 m_rot;

		OgEngine::Mesh* m_mesh;

		GeometryInstance m_geometry;
		uint64_t m_id;

		Buffer m_vertBuffer;
		Buffer m_indexBuffer;
	};

}