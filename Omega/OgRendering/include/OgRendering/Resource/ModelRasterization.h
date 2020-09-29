#pragma once
#include <OgRendering/Export.h>

#include <OgRendering/Resource/Mesh.h>
#include <OgRendering/Resource/Texture.h>
#include <glm/glm.hpp>

namespace OgEngine
{
	struct MaterialRS
	{
		alignas(16) glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		alignas(16) glm::vec4 specular = { 1.0f, 1.0f, 1.0f, 1.0f };
		alignas(16) glm::vec4 emissive = { 1.0f, 1.0f, 1.0f, 1.0f };
		
		bool operator==(const MaterialRS& p_other) const
		{
			return color == p_other.color && specular == p_other.specular && emissive == p_other.emissive;
		}
		bool operator!=(const MaterialRS& p_other) const
		{
			return !(*this == p_other);
		}
	};

	class RENDERING_API ModelRasterization
	{
	public:
		ModelRasterization(Mesh* p_mesh = nullptr, OgEngine::Texture* p_texture = nullptr, glm::mat4 p_modelMatrix = glm::mat4(1.0f));
		~ModelRasterization();

		void SetMesh(Mesh* p_newMesh);
		void SetTexture(Texture* p_newTexture);
		void UpdateModelMatrix(const glm::mat4& p_newModelMatrix);
		void UpdateMaterial(const MaterialRS& p_newMaterial);
		void ChangeColor(const glm::vec4& p_newBaseColor);
		void ChangeSpecularColor(const glm::vec4& p_newSpecularColor);
		void ChangeEmissiveColor(const glm::vec4& p_newEmissiveColor);

		[[nodiscard]] Mesh* Mesh() const;
		[[nodiscard]] Texture* Texture() const;
		[[nodiscard]] glm::mat4 ModelMatrix() const;
		[[nodiscard]] MaterialRS Material() const;

		bool operator==(const ModelRasterization& p_other) const;
		bool operator!=(const ModelRasterization& p_other) const;

	private:
		glm::mat4 m_modelMatrix;
		MaterialRS m_material;
		OgEngine::Texture* m_texture;
		OgEngine::Mesh* m_mesh;
	};
}
