#ifndef _FCSTATEMACHINE_H
#define _FCSTATEMACHINE_H

#include <map>
#include <atomic>
#include <mutex>

#include "FCAsyncLauncher.h"
#include "FCState.h"

class FCStateMachine
{
public:
	FCStateMachine() : 
	  m_current_state(0),
	  m_previous_state(0) { m_quit.store(false); }
	~FCStateMachine() { }

	bool init_state(const int state_id, FuncType function_loop, FuncType function_init = nullptr, FuncType function_exit = nullptr)
	{
		//Loop function must be always set
		if (function_loop == nullptr)
			return false;
		FCState* pState_to_set = new FCState;
		pState_to_set->set_loop_function(function_loop);
		//Other setter functions check for nullptr
		pState_to_set->set_init_function(function_init);
		pState_to_set->set_exit_function(function_exit);
		auto result = m_states.emplace(state_id, pState_to_set);
		return result.second;
	}

	//Start execute loop
	void start()
	{
		std::function<void()> function = std::bind(&FCStateMachine::loop, this);
		m_launcher.init(function);
		m_launcher.launch();
	}

	//Stop execute loop
	void stop()
	{
		m_quit.store(true);
	}

	//Trigger transition to other state
	void set_state(const int state_to_set)
	{
		//Critical section starts here
		std::unique_lock<std::mutex> lock(m_mutex);
		if (state_to_set == m_current_state)
			return;
		if (m_states.find(state_to_set) == m_states.end())
			return;
		m_previous_state = m_current_state;
		m_current_state = state_to_set;
	}

	//No copy constructor, no move assignment
	FCStateMachine(FCStateMachine&&) = delete;
	FCStateMachine(const FCStateMachine&) = delete;
	FCStateMachine& operator=(FCStateMachine&&) = delete;
	FCStateMachine& operator=(const FCStateMachine&) = delete;

private:
	std::map<const int, FCState*> m_states;
	FCAsyncLauncher<void> m_launcher;
	unsigned int m_current_state, m_previous_state;
	std::atomic<bool> m_quit;
	std::mutex m_mutex;

	//Start executing states - asynchronously executed by start method
	void loop()
	{
		FCState* pStateCurrent = nullptr;
		FCState* pStatePrevious = nullptr;
		while (m_quit.load() == false)
		{
			//Critical section starts here
			unsigned int current_state = 0, previous_state = 0;
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				current_state = m_current_state;
				previous_state = m_previous_state;
			} //Mutex is automatically released here

			//Get current state instance from state map
			pStateCurrent = m_states.find(current_state)->second;
			//Check if state has been changed
			if (current_state != previous_state)
			{
				//Get previous state from map, set exit and execute
				//This causes state to reset to init state
				pStatePrevious = m_states.find(previous_state)->second;
				pStatePrevious->set_exit();
				pStatePrevious->execute();
				//Update previous state
				//Critical section starts here
				{
					std::unique_lock<std::mutex> lock(m_mutex);
					m_previous_state = current_state;
				} //Mutex is automatically released here
			}
			//Execute current state
			pStateCurrent->execute();
		}
		//Check if pStateCurrent has been assigned to
		if (pStateCurrent == nullptr)
			return;
		//Ensure exit state is called after stop command
		if (pStateCurrent->get_state() != FCState_StateExit)
		{
			pStateCurrent->set_exit();
			pStateCurrent->execute();
		}
	}
};

#endif
