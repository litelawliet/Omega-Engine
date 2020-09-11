#pragma once
#include <OgCore/Export.h>
#include <OgRendering/Resource/Mesh.h>

#include <OgCore/Components/Material.h>
#include <glm/glm.hpp>

namespace OgEngine
{
	struct Transform;

	class CORE_API ModelRS
	{
	public:
		ModelRS();
		explicit ModelRS(std::string_view p_meshName);
		explicit ModelRS(Mesh* p_mesh);
		ModelRS(const ModelRS& p_other);
		ModelRS(ModelRS&& p_other) noexcept;

		void SetMesh(Mesh* p_mesh);
		void SetMesh(std::string_view p_meshName);

		void SetMaterial(const Material& p_material);

		[[nodiscard]] Mesh* GetMesh() const;
		[[nodiscard]] Material& Material();
		[[nodiscard]] const std::string& MeshName() const;
		[[nodiscard]] const std::string& ParentMeshName() const;
		[[nodiscard]] const std::string& MeshFilepath() const;
		[[nodiscard]] glm::mat4 ModelMatrix() const;

		ModelRS& operator=(const ModelRS& p_other);
		ModelRS& operator=(ModelRS&& p_other) noexcept;
		void     SetLocalTransform(Transform& p_transform);
		[[nodiscard]] std::string Serialize(const int p_depth) const;

	private:
		OgEngine::Material m_material;
		OgEngine::Mesh* m_mesh = nullptr;
		std::string m_meshName;
		std::string m_parentMeshName;
		std::string m_meshFilepath;
		Transform* m_meshTransform = nullptr;

		[[nodiscard]] static std::string DepthIndent(const int p_depth);
	};
}
