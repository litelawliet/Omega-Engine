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
		inline static std::shared_ptr<ResourceType> Load(std::string_view p_file);

	private:
		LoaderManager() = default;
	};

	template<>
	inline std::shared_ptr<Mesh> LoaderManager::Load<Mesh>(std::string_view p_file);
}

#include <OgRendering/Managers/Loaders/LoaderManager.inl>