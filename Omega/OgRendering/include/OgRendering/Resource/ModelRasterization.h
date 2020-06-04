#pragma once
#include <OgRendering/Export.h>

#include <OgRendering/Resource/Mesh.h>
#include <OgRendering/Resource/Texture.h>

namespace OgEngine
{
	struct MaterialRS
	{
		alignas(16) GPM::Vector4F color = { 1.0f, 1.0f, 1.0f, 1.0f };
		alignas(16) GPM::Vector4F specular = { 1.0f, 1.0f, 1.0f, 1.0f };
		alignas(16) GPM::Vector4F emissive = { 1.0f, 1.0f, 1.0f, 1.0f };
		
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
		ModelRasterization(Mesh* p_mesh = nullptr, OgEngine::Texture* p_texture = nullptr, Matrix4F p_modelMatrix = Matrix4F::identity);
		~ModelRasterization();

		void SetMesh(Mesh* p_newMesh);
		void SetTexture(Texture* p_newTexture);
		void UpdateModelMatrix(const Matrix4F& p_newModelMatrix);
		void UpdateMaterial(const MaterialRS& p_newMaterial);
		void ChangeColor(const GPM::Vector4F& p_newBaseColor);
		void ChangeSpecularColor(const GPM::Vector4F& p_newSpecularColor);
		void ChangeEmissiveColor(const GPM::Vector4F& p_newEmissiveColor);

		[[nodiscard]] Mesh* Mesh() const;
		[[nodiscard]] Texture* Texture() const;
		[[nodiscard]] Matrix4F ModelMatrix() const;
		[[nodiscard]] MaterialRS Material() const;

		bool operator==(const ModelRasterization& p_other) const;
		bool operator!=(const ModelRasterization& p_other) const;

	private:
		Matrix4F m_modelMatrix;
		MaterialRS m_material;
		OgEngine::Texture* m_texture;
		OgEngine::Mesh* m_mesh;
	};
}
