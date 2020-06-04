#pragma once
#include <OgRendering/Export.h>

#include <vector>
#include <thread>

namespace OgEngine::Utils
{
	class RENDERING_API ThreadPool
	{
	public:
		/**
		 * @brief Add a task into the pool
		 * @param p_function The function to do
		 * @param p_args All the arguments (remember to give the class reference as first argument if you use a method)
		 */
		template<typename Function, typename ... Args>
		void AddTask(Function&& p_function, Args&& ... p_args)
		{
			m_workers.emplace_back(std::forward<Function>(p_function), std::forward<Args>(p_args)...);
		}

		/**
		 * @brief Number of worker in use
		 * @return The number of worker in use
		 */
		[[nodiscard]] uint64_t WorkersInUse() const;

		/**
		 * @brief Wait for all the workers to finish their task
		 */
		void WaitForWorkers();

		/**
		 * @brief Wait for a specific worker to finish its task
		 * @param p_indexOfWorker The index of the worker to wait
		 */
		void WaitForWorker(const uint64_t p_indexOfWorker);

		/**
		 * @brief Default constructor
		 */
		ThreadPool() = default;
		ThreadPool(const ThreadPool& p_other) = delete;
		ThreadPool& operator=(const ThreadPool& p_other) = delete;
		ThreadPool(ThreadPool&& p_other) = default;
		ThreadPool& operator=(ThreadPool&& p_other) = default;

		/**
		* @brief Default destructor
		*/
		~ThreadPool();
		
	private:
		std::vector<std::thread> m_workers;
	};
}