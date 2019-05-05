#ifndef _FCTHREADMANAGER_H
#define _FCTHREADMANAGER_H

#include <map>
#include <functional>
#include <chrono>
#include <future>
#include <mutex>
#include <atomic>

#include "FCThreadPool.h"
#include "FCEventTable.h"
#include "FCFunctions.h"

//Thread manager class - handles event table and corresponding return data from events
class FCThreadManager
{
public:
	FCThreadManager(const int n_threads) :
		m_pool(n_threads) { m_shutdown.store(false); m_pool.init(); init_event_table(); }
	~FCThreadManager() { clear_return_data(); }

	//Submit event to table using std::function object
	template <typename T>
	bool submit_event(const int event_id, const std::function<T> function)
	{
		if (m_shutdown.load() == true)
			return false;
		//FCEventTable is already thread safe, so no mutex here
		return m_table.add(event_id, std::function<T>(function));
	}

	//Submit event to table using raw function pointer
	template <typename F, typename ...Args>
	bool submit_event(const int event_id, F&& f, Args&&... args)
	{
		if (m_shutdown.load() == true)
			return false;
		//FCEventTable is already thread safe, so no mutex here
		//Create std::function with bound arguments
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		return m_table.add(event_id, func);
	}

	//Send (trigger) event - optional arguments must be passed in here
	template <typename T, typename ...Args>
	bool send_event(const int event_id, Args&&... args)
	{
		if (m_shutdown.load() == true)
			return false;
		std::function<T> func;
		if (m_table.get(event_id, func))
		{
			std::unique_lock<std::mutex> lock(m_mutex); //Because return data map is not thread safe
			auto future = m_pool.submit(func, std::forward<Args>(args)...);
			typedef decltype(future.get()) future_type;
			auto result = m_return_data.emplace(event_id, new FCFuture<future_type>(std::move(future)));
			return result.second;
		}
		else
			return false;
	}

	//Method for getting corresponding event data
	//Future::get() method must be called only once, so a flag is set if already called
	template <typename RetType>
	bool get_data(const int event_id, RetType& data)
	{
		std::unique_lock<std::mutex> lock(m_mutex); //Because return data map is not thread safe
		FCIFuture* pFuture = m_return_data.find(event_id)->second;
		FCFuture<RetType>* pFutureType = dynamic_cast<FCFuture<RetType>*>(pFuture);
		//Check if the cast was successful
		if (pFutureType == nullptr)
			return false;
		if (pFutureType->m_ready == true)
		{
			data = pFutureType->m_returnValue;
			return true;
		}
		else if (is_ready(pFutureType->m_future) == true)
		{
			pFutureType->m_ready = true;
			pFutureType->m_returnValue = pFutureType->m_future.get(); //Must be only called once
			data = pFutureType->m_returnValue;
			return true;
		}
		else
		{
			return false;
		}
	}

	void shutdown()
	{
		//Shutdown flag needs to be set first
		m_shutdown.store(true);
		m_pool.shutdown();
	}

	//No copy constructor, no move assignment
	FCThreadManager(FCThreadManager&&) = delete;
	FCThreadManager(const FCThreadManager&) = delete;
	FCThreadManager& operator=(FCThreadManager&&) = delete;
	FCThreadManager& operator=(const FCThreadManager&) = delete;

private:
	//Private helper structures for wrapping future data in container (std::map)
	//Wrapper for future and return value, base type
	struct FCIFuture
	{
		virtual ~FCIFuture() { }
	};

	//Typed future wrapper for use in container (std::map)
	template <typename T>
	struct FCFuture : public FCIFuture
	{
		FCFuture(std::future<T> future) :
			m_future(std::move(future)) {
			m_ready = false;
		}
		~FCFuture() { }

		//Wrapped future object
		std::future<T> m_future;
		bool m_ready;
		//Return value is accessed only after m_ready has been set
		T m_returnValue;
	};

	FCThreadPool m_pool; //Thread safe thread pool
	FCEventTable m_table; //Thread safe event table
	std::map<const int, FCIFuture*> m_return_data; //Not thread safe
	std::atomic<bool> m_shutdown;
	std::mutex m_mutex;

	bool init_event_table()
	{
		//Create std::functions from methods
		std::function<void()>    func1(funktion1);
		std::function<void(int)> func2(funktion2);
		std::function<int(void)> func3(funktion3);
		std::function<int(int)>  func4(funktion4);
		std::function<FCReturnData()> func5(funktion5);
		std::function<int(int)> func6 = std::bind(&S::f4, &s6, std::placeholders::_1);

		//Add functions to event table
		bool is_successful = false;
		is_successful =
			submit_event(func1_id, func1) &&
			submit_event(func2_id, func2) &&
			submit_event(func3_id, func3) &&
			submit_event(func4_id, func4) &&
			submit_event(func5_id, func5) &&
			submit_event(func6_id, func6);

		return is_successful;
	}

	//Check if thread associated with future has finished
	//Static function, works on arbitrary futures
	template<typename R>
	static bool is_ready(std::future<R>& future)
	{
		return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}

	//Delete elements in container
	void clear_return_data()
	{
		for (std::map<const int, FCIFuture*>::iterator it = m_return_data.begin(); it != m_return_data.end(); it++)
			delete (it->second);
		m_return_data.clear();
	}
};

#endif
