#include <OgRendering/Utils/ThreadPool.h>

uint64_t OgEngine::Utils::ThreadPool::WorkersInUse() const
{
	return m_workers.size();
}

void OgEngine::Utils::ThreadPool::WaitForWorkers()
{
	for (auto& worker : m_workers)
	{
		if(worker.joinable())
			worker.join();
	}
}

void OgEngine::Utils::ThreadPool::WaitForWorker(const uint64_t p_indexOfWorker)
{
	if(m_workers[p_indexOfWorker].joinable())
		m_workers[p_indexOfWorker].join();
}

OgEngine::Utils::ThreadPool::~ThreadPool()
{
	WaitForWorkers();
}
