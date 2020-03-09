#pragma once
#include <OgRendering/Export.h>

#include <vector>
#include <thread>

namespace OgEngine::Utils
{
	class RENDERING_API ThreadPool
	{
	public:
		template<typename Function, typename ... Args>
		void AddTask(Function&& p_function, Args&& ... p_args)
		{
			m_workers.emplace_back(std::forward<Function>(p_function), std::forward<Args>(p_args)...);
		}

		uint64_t WorkersInUse() const;
		void WaitForWorkers();
		void WaitForWorker(const uint64_t p_indexOfWorker);

		ThreadPool() = default;
		ThreadPool(const ThreadPool& p_other) = delete;
		ThreadPool& operator=(const ThreadPool& p_other) = delete;
		ThreadPool(ThreadPool&& p_other) = default;
		ThreadPool& operator=(ThreadPool&& p_other) = default;
		~ThreadPool();
		
	private:
		std::vector<std::thread> m_workers;
	};
}