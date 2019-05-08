#ifndef _FCQUEUE_H
#define _FCQUEUE_H

#include <mutex>
#include <queue>

//Thread safe implementation of a queue
template <typename T>
class FCQueue
{
public:
	//Unlimited queue - max size is 0
	FCQueue() :
		m_max_size(0) { }

	//Limited queue
	FCQueue(int max_size) :
		m_max_size(max_size) { }

	~FCQueue() { }

	//Provide element access through operator()
	T& operator()(int index)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue[index];
	}

	bool is_empty()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}

	bool is_full()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return (m_max_size == 0 ? false : (m_queue.size() >= m_max_size));
	}

	int size()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

	int max_size()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_max_size;
	}

	bool enqueue(const T& element)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_max_size == 0)
		{
			m_queue.push_back(element);
			return true;
		}
		else
		{
			if (m_queue.size() < m_max_size)
			{
				m_queue.push_back(element);
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	bool dequeue(T& element)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_queue.empty() == true)
		{
			return false;
		}
		else
		{
			element = std::move(m_queue.front());
			m_queue.pop_front();
			return true;
		}
	}

	//No copy constructor, no move assignment
	FCQueue(FCQueue&&) = delete;
	FCQueue(const FCQueue&) = delete;
	FCQueue& operator=(FCQueue&&) = delete;
	FCQueue& operator=(const FCQueue&) = delete;

private:
	//Use deque, because it provides member access
	std::deque<T> m_queue;
	std::mutex m_mutex;
	size_t m_max_size;
};

#endif
