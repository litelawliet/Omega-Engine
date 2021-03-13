#pragma once
#include <OgRendering/Export.h>

#include <concurrent_unordered_map.h>
#include <memory>
#include <optional>
#include <unordered_map>
#include <functional>
#include <list>
#include <sstream>

#include <OgRendering/Resource/Mesh.h>
#include <OgRendering/Utils/ThreadPool.h>

namespace OgEngine::Services
{
	class RENDERING_API MeshService final
	{
	public:
		MeshService() = default;
		~MeshService();

		void Add(std::string_view p_filePath);
		
		[[nodiscard]] std::shared_ptr<Mesh> Get(std::string_view p_meshName) const;
		void WaitForAll() { m_pool.WaitForWorkers(); }
		void WaitForResource(std::string_view p_meshName);

	private:
		void MultithreadedLoading(std::string_view p_filePath);

		Concurrency::concurrent_unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;
		Utils::ThreadPool m_pool;

		std::hash<std::string> m_hashValueFromName;
		std::list<std::pair<uint64_t, std::string>> m_workerToMesh;
	};
}
