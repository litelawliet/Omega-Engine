#pragma once
#include <OgRendering/Export.h>

#include <concurrent_unordered_map.h>
#include <memory>

#include <OgRendering/Resource/Mesh.h>
#include <optional>
#include <unordered_map>
#include <OgRendering/Utils/ThreadPool.h>
#include <functional>
#include <list>

namespace OgEngine::Services
{
	class RENDERING_API MeshService final
	{
	public:
		MeshService();
		~MeshService();

		inline void Add(std::string_view p_filePath);
		
		[[nodiscard]] inline std::shared_ptr<Mesh> Get(std::string_view p_meshName) const;
		inline void WaitForAll() { m_pool.WaitForWorkers(); }
		inline void WaitForResource(std::string_view p_meshName);

	private:
		inline void MultithreadedLoading(std::string_view p_filePath);

		Concurrency::concurrent_unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;
		Utils::ThreadPool m_pool;

		const std::hash<std::string> m_hashValueFromName;
		std::list<std::pair<uint64_t, std::string>> m_workerToMesh;
	};
}
