#ifndef _FCSTATE_H
#define _FCSTATE_H

#include <functional>

typedef enum FCState_State
{
	FCState_StateInit,
	FCState_StateLoop,
	FCState_StateExit
} FCState_States;

//Function pointer type used for FCState
using FuncType = void(*)();
using FunctionType = std::function<void()>;

//Class that wraps a state containing a loop function and optional init and exit functions
//To be used by FCStateMachine
class FCState
{
public:
	FCState() :
	  m_initialized(false), 
	  m_state(FCState_StateInit) { }
	~FCState() { }

	FCState_State get_state() { return m_state; }

	void set_init_function(FuncType function)
	{
		if (function == nullptr)
			return;
		m_states.emplace(FCState_StateInit, FunctionType(function));
	}

	void set_loop_function(FuncType function)
	{
		m_states.emplace(FCState_StateLoop, FunctionType(function));
		m_initialized = true; //Only loop function is required for execution
	}

	void set_exit_function(FuncType function)
	{
		if (function == nullptr)
			return;
		m_states.emplace(FCState_StateExit, FunctionType(function));
	}

	//Execute method to be called continuously by state machine
	void execute()
	{
		if (m_initialized == false)
			return;
		//Check if function is existing
		if (m_states.find(m_state) != m_states.end())
		{
			//Get actual active function from map and execute it
			FunctionType function_to_execute = m_states.find(m_state)->second;
			function_to_execute();
		}
		//Update state
		if (m_state == FCState_StateInit)
			m_state = FCState_StateLoop;
		else if (m_state == FCState_StateExit)
			m_state = FCState_StateInit;
	}

	void set_exit()
	{
		m_state = FCState_StateExit;
	}

private:
	std::map<FCState_State, std::function<void()>> m_states;
	FCState_State m_state;
	bool m_initialized;
};

#endif
