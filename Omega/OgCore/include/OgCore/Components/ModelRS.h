#pragma once
#include <string_view>
#include <memory>
#include <OgRendering/Resource/Mesh.h>
#include <OgCore/Components/Transform.h>

#include <OgCore/Components/Material.h>

namespace OgEngine
{
	class ModelRS
	{
	public:
		ModelRS();
		explicit ModelRS(std::string_view p_meshName);
		explicit ModelRS(std::shared_ptr<Mesh> p_mesh);
		ModelRS(const ModelRS& p_other);
		ModelRS(ModelRS&& p_other) noexcept;

		void SetMesh(const std::shared_ptr<Mesh>& p_mesh);
		void SetMesh(std::string_view p_meshName);

		void SetMaterial(const Material& p_material);

		[[nodiscard]] const std::shared_ptr<Mesh>& Mesh() const;
		[[nodiscard]] const Material& Material() const;
		[[nodiscard]] Matrix4F ModelMatrix() const;

		ModelRS& operator=(const ModelRS& p_model);
		ModelRS& operator=(ModelRS&& p_model) noexcept;
		void     SetLocalTransform(Transform& p_transform);
  
	private:
		std::shared_ptr<OgEngine::Mesh> m_mesh;
		OgEngine::Material m_material;
		Transform*                      m_meshTransform;
	};
}
