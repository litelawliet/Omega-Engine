#pragma once
#include <OgCore/Export.h>
#include <GPM/GPM.h>

namespace OgEngine
{
	struct Transform;

	struct CORE_API Material final
	{
		/**
		 * @brief The color of the material (read-only)
		 */
		const Vector4F& color = _color;

		/**
		* @brief The specular of the material (read-only)
		*/
		const Vector4F& specular = _specular;

		/**
		 * @brief The emissive of the material (read-only)
		 */
		const Vector4F& emissive = _emissive;

		/**
		 * @brief The roughness of the material (read-only)
		 */
		const float& roughness = _roughness;

		/**
		 * @brief The ior of the material (read-only)
		 */
		const float& ior = _ior;

		/**r
		 * @brief The type of the material (read-only)
		 */
		const int& type = _materialType;

		/**
		 * @brief The texture ID of the material (read-only)
		 */
		const std::string& texName = _texName;

		/**
		 * @brief The texture filepath of the material (read-only)
		 */
		const std::string& texPath = _texPath;

		/**
		 * @brief The normalMap ID of the material (read-only)
		 */
		const std::string& normName = _normName;

		/**
		 * @brief The texture filepath of the material (read-only)
		 */
		const std::string& normPath = _normPath;
		
		/**
		 * @brief Default constructor of the material
		 */
		Material();

		/**
		 * @brief Copy constructor of the material
		 * @param p_other The other material to copy
		 */
		Material(const Material& p_other);

		/**
		 * @brief Move constructor of the material
		 * @param p_other The other material to move the data from
		 */
		Material(Material&& p_other) noexcept;

		/**
		 * @brief Destructor
		 */
		~Material();

		/**
		 * @brief Define the color of the material
		 * @param p_color The new color
		 */
		void SetColor(const GPM::Vector4F& p_color);

		/**
		 * @brief Define the specular of the material
		 * @param p_specular The new color
		 */
		void SetSpecular(const GPM::Vector4F& p_specular);

		/**
		 * @brief Define the roughness of the material
		 * @param p_roughness The new roughness between [0,1].
		 */
		void SetRoughness(const float p_roughness);

		/**
		 * @brief Define the texture of the material
		 * @param p_texID is the ID of the texture.
		 */
		void SetTextureID(const std::string& p_texID, const std::string& p_texPath);

		/**
		 * @brief Define the normalMap of the material
		 * @param p_normID is the ID of the normalMap.
		 */
		void SetNormalMapID(const std::string& p_normID, const std::string& p_normPath);

		/**
		 * @brief Define the index of refraction of the material
		 * @param p_ior The new ior between [0,2].
		 */
		void SetIOR(const float p_ior);

		/**
		 * @brief Define the emissiveness of the material
		 * @param p_emissive The new emissiveness.
		 */
		void SetEmissive(const GPM::Vector4F& p_emissive);

		/**
		 * @brief Change the material type
		 * @param p_type Type of the material.
		*/
		void SetType(const int p_type);

		/**
		 * @brief Set the transform of this material. Serves as linking all components together to the same transform for ease of use.
		 * @param p_transform GameObject transform.
		*/
		void SetLocalTransform(Transform& p_transform);

		[[nodiscard]] std::string Serialize(const int p_depth) const;

		/**
		 * @brief Copy assignment
		 * @param p_other The material
		 * @return The current material modified
		 */
		inline Material& operator=(const Material& p_other);

		/**
		 * @brief Move assignment
		 * @param p_other The other material
		 * @return The current material modified
		 */
		inline Material& operator=(Material&& p_other) noexcept;

	private:
		Vector4F _color;
		Vector4F _specular;
		Vector4F _emissive;
		float _ior;
		float _roughness;
		int _materialType;
		std::string _texName;
		std::string _texPath;
		std::string _normName;
		std::string _normPath;
		Transform* m_materialTransform = nullptr;

		[[nodiscard]] static std::string DepthIndent(const int p_depth);
	};
}
