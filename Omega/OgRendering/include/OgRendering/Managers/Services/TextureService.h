#pragma once
#include <OgRendering/Export.h>

#include <concurrent_unordered_map.h>
#include <list>
#include <string>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>

#include <OgRendering/Resource/Texture.h>
#include <OgRendering/Utils/ThreadPool.h>

namespace OgEngine::Services
{
	class RENDERING_API TextureService final
	{
	public:
		TextureService() = default;
		~TextureService();
		
		void Add(std::string_view p_filePath);

		[[nodiscard]] std::shared_ptr<Texture> Get(std::string_view p_textureName) const;
		void WaitForAll() { m_pool.WaitForWorkers(); }
		void WaitForResource(std::string_view p_textureName);
		std::vector<Texture*>& GetAllTextures();

	private:
		void MultithreadedLoading(std::string_view p_filePath);

		Concurrency::concurrent_unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
		Utils::ThreadPool m_pool;

		const std::hash<std::string> m_hashValueFromName;
		std::list<std::pair<uint64_t, std::string>> m_workerToTexture;
		std::vector<Texture*> m_texturesRefs;
	};
}
