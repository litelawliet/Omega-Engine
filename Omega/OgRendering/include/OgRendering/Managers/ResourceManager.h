#pragma once
#include <OgRendering/Export.h>

#include <memory>

#include <OgRendering/Managers/Services/MeshService.h>
#include <OgRendering/Managers/Services/TextureService.h>

#include <OgRendering/Utils/TemplateTypename.h>


namespace OgEngine
{
	class RENDERING_API ResourceManager final
	{
	public:
		static ResourceManager& Instance();

		template<typename ResourceType>
		static inline void Add(std::string_view p_resourceName);

		template<typename ResourceType>
		[[nodiscard]] static inline ResourceType* Get(std::string_view p_resourceName);

		template<typename ResourceType>
		static inline void WaitForResource(std::string_view p_resourceName);

		static inline std::vector<Texture*>& GetAllTextures();
		
		static inline void WaitForAll() {
			m_textureService.WaitForAll();
			m_meshService.WaitForAll();
		}

		ResourceManager(ResourceManager const&) = delete;
		void operator=(ResourceManager const&) = delete;

		ResourceManager(ResourceManager&&) = delete;
		void operator=(ResourceManager&&) = delete;

	private:
		ResourceManager() = default;
		~ResourceManager() = default;

		static Services::MeshService m_meshService;
		static Services::TextureService m_textureService;
		static bool m_raytracingEnable;
	};

#pragma region Mesh
	template<>
	inline void ResourceManager::Add<Mesh>(std::string_view p_resourceName);

	template<>
	[[nodiscard]] inline Mesh* ResourceManager::Get(std::string_view p_resourceName);

	template<>
	inline void ResourceManager::WaitForResource<Mesh>(std::string_view p_resourceName);
#pragma endregion

#pragma region Texture
	template<>
	inline void ResourceManager::Add<Texture>(std::string_view p_resourceName);

	template<>
	[[nodiscard]] inline Texture* ResourceManager::Get(std::string_view p_resourceName);

	template<>
	inline void ResourceManager::WaitForResource<Texture>(std::string_view p_resourceName);
#pragma endregion 
}

#include <OgRendering/Managers/ResourceManager.inl>