#ifndef _FCTHREADPOOL_H
#define _FCTHREADPOOL_H

#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "FCQueue.h"

class FCThreadPool
{
public:
	explicit FCThreadPool(const int n_threads) :
		m_threads(std::vector<std::thread>(n_threads)), m_shutdown(false) { }

	~FCThreadPool() { }

	//Initializes the thread pool
	void init()
	{
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			//Invoke FCThreadWorker's function operator 
			m_threads[i] = std::thread(FCThreadWorker(this, i));
		}
	}

	//Wait until threads finish their tasks, then shut down pool
	void shutdown()
	{
		m_shutdown = true;
		m_conditional_lock.notify_all();

		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (m_threads.at(i).joinable() == true)
			{
				//Joinable means, thread is active
				m_threads.at(i).join();
			}
		}
	}

	//Send a function to the pool to be executed asynchronously
	template <typename F, typename ...Args>
	auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
	{
		//Create a function with bound parameters ready to execute
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		//Put it into a shared ptr in order to be able to copy construct and assign
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
		//Wrap packaged task into void function - uses lambda
		std::function<void()> wrapper_func = [task_ptr]() { (*task_ptr)(); };
		//Enqueue generic wrapper function
		m_queue.enqueue(wrapper_func);
		//Wake up one thread to do the job
		m_conditional_lock.notify_one();
		//Return the future from the promise
		return task_ptr->get_future();
	}

	//No copy constructor, no move assignment
	FCThreadPool(FCThreadPool&&) = delete;
	FCThreadPool(const FCThreadPool&) = delete;
	FCThreadPool& operator=(FCThreadPool&&) = delete;
	FCThreadPool& operator=(const FCThreadPool&) = delete;

private:
	//Private helper class
	class FCThreadWorker
	{
	public:
		explicit FCThreadWorker(FCThreadPool* pool, const int id) :
			m_pool(pool), m_id(id) { }

		~FCThreadWorker() { }

		//Function call operator used in init() method
		void operator()()
		{
			std::function<void()> func;
			bool dequeued;
			while (m_pool->m_shutdown == false)
			{
				std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
				if (m_pool->m_queue.is_empty() == true)
				{
					//Block thread until condition variable is notified
					m_pool->m_conditional_lock.wait(lock);
				}
				dequeued = m_pool->m_queue.dequeue(func);
				if (dequeued == true)
				{
					func();
				}
			}
		}

	private:
		int m_id;
		FCThreadPool* m_pool;
	};

	//Uses a thread safe queue
	FCQueue<std::function<void()>> m_queue;
	bool m_shutdown;
	std::vector<std::thread> m_threads;
	std::mutex m_conditional_mutex;
	std::condition_variable m_conditional_lock;
};

#endif
