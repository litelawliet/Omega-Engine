#pragma once
#include <OgRendering/Export.h>
#include <OgRendering/Resource/Mesh.h>
#include <OgRendering/Resource/RTMesh.h>
#include <OgRendering/Utils/VulkanTools.h>

#include <memory>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace OgEngine
{
	class RENDERING_API Model final
	{
	public:
		Model() = default;
		Model(OgEngine::Mesh* p_mesh, bool useRT)
		{
			m_mesh = p_mesh;

			m_geometry.instanceId = 0;
			m_geometry.mask = 0xff;
			m_geometry.instanceOffset = 0;
			m_geometry.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		}

		~Model()
		{
			if(m_geometryBuffer.mapped != nullptr)
				m_geometryBuffer.Destroy();
		}

		void Translate(GPM::Vector3F tr)
		{
			m_pos += tr;
			UpdateTransform();
		}

		void Rotate(GPM::Vector3F rotation)
		{
			m_rot += rotation;
			UpdateTransform();
		}

		void Scale(GPM::Vector3F scale)
		{
			m_geometry.transform[0].x = scale.x;
			m_geometry.transform[1].y = scale.y;
			m_geometry.transform[2].z = scale.z;
		}
		void UpdateTransform()
		{
			glm::vec3 position = ConvertToGLM(m_pos);
			glm::vec3 rotation = ConvertToGLM(m_rot);



			glm::mat4 transMat = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 rotationMat = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1, 0, 0));
			rotationMat *= glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0, 1, 0));
			rotationMat *= glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0, 0, 1));
			glm::mat4 convertedMat = transMat * rotationMat;
			m_geometry.transform = glm::transpose(convertedMat);

		}

		inline void ConvertTransform(GPM::Matrix4F ta)
		{
			glm::mat3x4 tb = glm::identity<glm::mat3x4>();
			tb[0].x = ta(0, 0); tb[0].y = ta(0, 1); tb[0].z = ta(0, 2); tb[0].w = ta(0, 3);
			tb[1].x = ta(1, 0); tb[1].y = ta(1, 1); tb[1].z = ta(1, 2); tb[1].w = ta(1, 3);
			tb[2].x = ta(2, 0); tb[2].y = ta(2, 1); tb[2].z = ta(2, 2); tb[2].w = ta(2, 3);

			m_pos = GPM::Vector3F({ tb[0].w, tb[1].w, tb[2].w });
			m_geometry.transform = tb;

		}

		GPM::Vector3F m_pos;
		GPM::Vector3F m_rot;

		OgEngine::Mesh* m_mesh;
		//std::shared_ptr<OgEngine::RTMesh> m_rtMesh;

		GeometryInstance m_geometry;
		uint64_t m_id;
		Buffer m_geometryBuffer;
		Buffer m_vertBuffer;
		Buffer m_indexBuffer;
	};

}