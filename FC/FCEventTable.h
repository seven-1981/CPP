#ifndef _FCEVENTTABLE_H
#define _FCEVENTTABLE_H

#include <functional>
#include <map>
#include <mutex>

class FCEventTable
{
public:
	FCEventTable() { }
	~FCEventTable() { clear_event_table(); }

	//Add entry to event table
	template <typename T>
	bool add(const int event_id, const std::function<T> function)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		//Create new event entry and store function inside
		FCEvent<T>* pEvent = new FCEvent<T>();
		pEvent->function = function;
		//Evaluate result of emplace - meaning entry not already present
		auto result = m_event_table.emplace(event_id, pEvent);
		return result.second;
	}

	//Add entry to event table - raw function pointer
	template <typename F, typename ...Args>
	bool add(const int event_id, F&& f, Args&&... args)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		//Create std::function with bound arguments
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		return add(event_id, func);
	}

	//Get entry from event table
	template <typename T>
	bool get(const int event_id, std::function<T>& function)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		auto pMapEntry = m_event_table.find(event_id);
		//Check if entry with given index exists
		if (pMapEntry != m_event_table.end())
		{
			//Get generic event type and cast it to typed event
			FCIEvent* pFunc = pMapEntry->second;
			FCEvent<T>* pFuncType = dynamic_cast<FCEvent<T>*>(pFunc);
			//Check if the cast was successful
			if (pFuncType)
			{
				function = pFuncType->function;
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	//No copy constructor, no move assignment
	FCEventTable(FCEventTable&&) = delete;
	FCEventTable(const FCEventTable&) = delete;
	FCEventTable& operator=(FCEventTable&&) = delete;
	FCEventTable& operator=(const FCEventTable&) = delete;

private:
	//Base class for events
	struct FCIEvent
	{
		virtual ~FCIEvent() { }
	};

	//Wrapper class for std::function in order to use with container
	template <typename T>
	struct FCEvent : public FCIEvent
	{
		std::function<T> function;
	};

	//Container of event entries with associated event IDs
	std::map<const int, FCIEvent*> m_event_table;
	std::mutex m_mutex;

	//Delete elements in container
	void clear_event_table()
	{
		for (std::map<const int, FCIEvent*>::iterator it = m_event_table.begin(); it != m_event_table.end(); it++)
			delete (it->second);
		m_event_table.clear();
	}
};

#endif
