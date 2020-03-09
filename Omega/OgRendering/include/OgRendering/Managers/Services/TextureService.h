#pragma once

#include <concurrent_unordered_map.h>
#include <list>
#include <OgRendering/Export.h>
#include <string>
#include <OgRendering/Utils/ThreadPool.h>
#include <OgRendering/Resource/Texture.h>

namespace OgEngine::Services
{
	class RENDERING_API TextureService final
	{
	public:
		TextureService();
		~TextureService();
		
		inline void Add(std::string_view p_filePath);

		[[nodiscard]] inline std::shared_ptr<Texture> Get(std::string_view p_textureName) const;
		inline void WaitForAll() { m_pool.WaitForWorkers(); }
		inline void WaitForResource(std::string_view p_textureName);

	private:
		inline void MultithreadedLoading(std::string_view p_filePath);

		Concurrency::concurrent_unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
		Utils::ThreadPool m_pool;

		const std::hash<std::string> m_hashValueFromName;
		std::list<std::pair<uint64_t, std::string>> m_workerToTexture;
	};
}
