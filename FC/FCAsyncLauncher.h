#ifndef _FCASYNCLAUNCHER_H
#define _FCASYNCLAUNCHER_H

#include <functional>
#include <future>

//Async call wrapper - simple handling of argument passing and return type
//May be only used once, further calls to member functions have no effect
template <typename RetType, typename ...Args>
class FCAsyncLauncher
{
public:
	FCAsyncLauncher() :
		m_initialized(false),
		m_running(false),
		m_returnValue(false),
		m_returnValueSet(false),
		m_hasFinished(false) { }
	~FCAsyncLauncher() { }

	//Pass function object to member functor
	//Only first call takes effect
	template <typename ...Args>
	bool init(std::function<RetType(Args...)> function)
	{
		if (function == nullptr)
			return false;
		if (m_initialized == false)
		{
			m_function = std::function<RetType(Args...)>(function);
			m_initialized = true;
		}
		return true;
	}

	//Pass function pointer to member function
	//Only first call takes effect
	template <typename ...Args>
	bool init(RetType(*function)(Args...))
	{
		if (function == nullptr)
			return false;
		if (m_initialized == false)
		{
			m_function = std::function<RetType(Args...)>(function);
			m_initialized = true;
		}
		return true;
	}

	//Launch the asynchronous thread - arguments may be passed here
	//Only first call takes effect
	template <typename ...Args>
	void launch(Args... args)
	{
		if (m_initialized == true && m_running == false)
		{
			m_future = std::async(std::launch::async, std::bind(m_function, std::forward<Args>(args)...));
			m_running = true;
		}
	}

	//Check if wrapped thread has finished its work
	bool has_finished()
	{
		if (m_running == false)
			return false;
		if (m_hasFinished == true)
			return true;
		else if (m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			//Underlying thread has finished work, set flag
			m_hasFinished = true;
			return m_hasFinished;
		}
		else
			return false;
	}

	//Get return value of underlying thread, if finished
	bool get_return(RetType& returnValue)
	{
		if (m_running == false)
			return false;
		if (m_returnValueSet == true)
		{
			returnValue = m_returnValue;
			return true;
		}
		else if (has_finished() == true)
		{
			//Future.get() may be called only once, so set flag to save return value
			m_returnValue = m_future.get();
			m_returnValueSet = true;
			returnValue = m_returnValue;
			return true;
		}
		else
			return false;
	}

	//No copy constructor, no move assignment
	FCAsyncLauncher(FCAsyncLauncher&&) = delete;
	FCAsyncLauncher(const FCAsyncLauncher&) = delete;
	FCAsyncLauncher& operator=(FCAsyncLauncher&&) = delete;
	FCAsyncLauncher& operator=(const FCAsyncLauncher&) = delete;

private:
	//Function for thread creation
	std::function<RetType(Args...)> m_function;
	//Future for return value extraction
	std::future<RetType> m_future;
	//Return value
	RetType m_returnValue;
	//Binary flags for controlling behaviour
	bool m_initialized;	//Indicates that init() has been called successfully
	bool m_running; //Indicates that wrapped thread has been started
	bool m_returnValueSet; //Indicates that return value has been set
	bool m_hasFinished; //Indicates that wrapped thread has been finished
};

//Class template specialization for void type
template<typename ...Args>
class FCAsyncLauncher<void, Args...>
{
public:
	FCAsyncLauncher() :
		m_initialized(false),
		m_running(false),
		m_returnValueSet(false),
		m_hasFinished(false) { }
	~FCAsyncLauncher() { }

	//Pass function object to member functor
	//Only first call takes effect
	template <typename ...Args>
	bool init(std::function<void(Args...)> function)
	{
		if (function == nullptr)
			return false;
		if (m_initialized == false)
		{
			m_function = std::function<void(Args...)>(function);
			m_initialized = true;
		}
		return true;
	}

	//Pass function pointer to member function
	//Only first call takes effect
	template <typename ...Args>
	bool init(void(*function)(Args...))
	{
		if (function == nullptr)
			return false;
		if (m_initialized == false)
		{
			m_function = std::function<void(Args...)>(function);
			m_initialized = true;
		}
		return true;
	}

	//Launch the asynchronous thread - arguments may be passed here
	//Only first call takes effect
	template <typename ...Args>
	void launch(Args... args)
	{
		if (m_initialized == true && m_running == false)
		{
			m_future = std::async(std::launch::async, std::bind(m_function, std::forward<Args>(args)...));
			m_running = true;
		}
	}

	//Check if wrapped thread has finished its work
	bool has_finished()
	{
		if (m_running == false)
			return false;
		if (m_hasFinished == true)
			return true;
		else if (m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			//Underlying thread has finished work, set flag
			m_hasFinished = true;
			return m_hasFinished;
		}
		else
			return false;
	}

	//No copy constructor, no move assignment
	FCAsyncLauncher(FCAsyncLauncher&&) = delete;
	FCAsyncLauncher(const FCAsyncLauncher&) = delete;
	FCAsyncLauncher& operator=(FCAsyncLauncher&&) = delete;
	FCAsyncLauncher& operator=(const FCAsyncLauncher&) = delete;

private:
	//Function for thread creation
	std::function<void(Args...)> m_function;
	//Future for return value extraction
	std::future<void> m_future;
	//Binary flags for controlling behaviour
	bool m_initialized;	//Indicates that init() has been called successfully
	bool m_running; //Indicates that wrapped thread has been started
	bool m_returnValueSet; //Indicates that return value has been set
	bool m_hasFinished; //Indicates that wrapped thread has been finished
};

#endif
