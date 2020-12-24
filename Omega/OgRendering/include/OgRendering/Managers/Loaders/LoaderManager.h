#pragma once
#include <OgRendering/Export.h>
#include <string_view>
#include <OgRendering/Resource/Mesh.h>
#include <OgRendering/Utils/TemplateTypename.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace OgEngine
{
	class RENDERING_API LoaderManager
	{
	public:
		template<typename ResourceType>
		static inline std::shared_ptr<ResourceType> Load(std::string_view p_file);

		static inline bool CheckValidMesh(std::string_view p_file);

	private:
		LoaderManager() = default;
		
		template<typename ResourceType>
		static inline std::shared_ptr<ResourceType> AssimpLoad(const std::string_view p_file);

		template<typename ResourceType>
		static inline std::shared_ptr<ResourceType> GltfLoad(const std::string_view p_file);

		template<typename ResourceType>
		static inline std::shared_ptr<ResourceType> ObjLoad(const std::string_view p_file);
		
		static std::string GetFilePathExtension(const std::string &p_filename);
	};

	template<>
	inline std::shared_ptr<OgEngine::Mesh> OgEngine::LoaderManager::Load<OgEngine::Mesh>(std::string_view p_file);
}

#include <OgRendering/Managers/Loaders/LoaderManager.inl>