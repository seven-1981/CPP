#include "bpm_globals.hpp"
#include "FCMachine.hpp"
#include "SplitConsole.hpp"
#include <iostream>
#include <thread>
#include <future>

//Extern split console instance
extern SplitConsole my_console;

FCMachine::FCMachine()
{
	//Create new member states
	for (int i = 0; i < eNumberOfStates; i++)
	{
		states[i] = new FCState;
	}

	//We set the first state, always 0 per default
	this->active_state = states[0];

	//Set size info
	this->size_info = eNumberOfStates;

	//Set transitions
	this->transition = -1;

	//Set init info - only initialized if all function
	//pointers are initialized properly (!= null pointer)
	this->is_initialized = false;

	//Stop flag set to false
	this->stop = false;

	//Write debug line
	#ifdef DEBUG_FC_MACHINE
		my_console.WriteToSplitConsole("Created FC Machine.", SPLIT_FC);
	#endif
}

FCMachine::~FCMachine()
{
	//Write debug line
	#ifdef DEBUG_FC_MACHINE
		my_console.WriteToSplitConsole("Removed FC Machine.", SPLIT_FC);
	#endif

	//Remove member states
	for (int i = 0; i < eNumberOfStates; i++)
		delete states[i];
}

//Bind methods for function pointers
//Entry function - signature f(void*(), int)
eError FCMachine::bind_function(void(*function)(), int id)
{
	//Check if ID is valid
	if (id >= eNumberOfStates)
	{
		#ifdef DEBUG_FC_MACHINE
			my_console.WriteToSplitConsole("FCStateError_InvalidIDNumber " + std::to_string(id), SPLIT_FC);
		#endif
		return eFCStateError_InvalidIDNumber;
	}

	//Bind the function pointer to the state
	return (this->states[id]->bind_entr(function));
}

//Loop function - signature f(FCStates*(), int)
eError FCMachine::bind_function(FCStates(*function)(), int id)
{
	//Check if ID is valid
	if (id >= eNumberOfStates)
	{
		#ifdef DEBUG_FC_MACHINE
			my_console.WriteToSplitConsole("FCStateError_InvalidIDNumber " + std::to_string(id), SPLIT_FC);
		#endif
		return eFCStateError_InvalidIDNumber;
	}

	//Bind the function pointer to the state
	return (this->states[id]->bind_loop(function));
}

//Exit function - signature f(unsigned int*(), int)
eError FCMachine::bind_function(int(*function)(), int id)
{
	//Check if ID is valid
	if (id >= eNumberOfStates)
	{
		#ifdef DEBUG_FC_MACHINE
			my_console.WriteToSplitConsole("FCStateError_InvalidIDNumber " + std::to_string(id), SPLIT_FC);
		#endif
		return eFCStateError_InvalidIDNumber;
	}

	//Bind the function pointer to the state
	return (this->states[id]->bind_exit(function));
}

//Loop function - auxiliary method for start_machine
eError FCMachine::machine_loop()
{
	eError retval = eSuccess;

	//State machine loop
	while (this->stop == false && retval == eSuccess)
	{
		//Fire execute method of state machine
		retval = this->execute();
	}

	//If this loop ends, it means that there went something wrong inside or a stop command was issued
	//This way, the executing function can verify, if it was an exceptional termination or a normal shutdown
	return retval;
}

//Stop method
void FCMachine::machine_stop()
{
	#ifdef DEBUG_FC_MACHINE
		my_console.WriteToSplitConsole("Stopping machine!", SPLIT_FC);
	#endif

	//Set the stop flag to true
	this->stop = true;
}

//Execute method
//This method must be executed periodically
eError FCMachine::execute()
{
	//Check if everything's initialized
	if (is_initialized == false)
	{
		bool initialized = true;
		for (int i = 0; i < eNumberOfStates; i++)
		{
			if (this->states[i]->check_init() == false)
				initialized = false;
		}

		is_initialized = initialized;
	}

	if (is_initialized == false)
	{
		#ifdef DEBUG_FC_MACHINE
			my_console.WriteToSplitConsole("FCStateError_FCMachineNotInitialized", SPLIT_FC);
		#endif
		return eFCStateError_FCMachineNotInitialized;
	}

	//Check which state is active and execute its functions
	eError retval = this->active_state->execute();
	if (retval != eSuccess)
		return retval;

	int next_id;
	//Check next ID, if not -1, we have to change state
	if (this->active_state->get_next_id() != -1)
	{
		next_id = this->active_state->get_next_id();
		if (next_id >= this->size_info)
		{
			#ifdef DEBUG_FC_MACHINE
				my_console.WriteToSplitConsole("FCStateError_InvalidIDNumber " + std::to_string(next_id), SPLIT_FC);
			#endif
			return eFCStateError_InvalidIDNumber;
		}

		//Reset next id to -1 for continuation
		eError retval = this->active_state->set_next_id(-1);
		if (retval != eSuccess)
			return retval;

		//Change active state according to next ID
		this->active_state = states[next_id];
	}

	return eSuccess;
}

//Create thread with executing loop for state machine
std::future<eError> FCMachine::start_machine()
{
	//It is not possible to return values from thread functions
	//Therefore, we use a std::async. This basically launches a thread for us and provides
	//a std::future object. After termination of std::async, the future object holds the return value
	//The return value is of type 'eError'
	//We can read the return value in main using the member function get() (of the future object)
	std::future<eError> result = std::async(std::launch::async, &FCMachine::machine_loop, this);
	return result;
}

eError FCMachine::set_trans(int id)
{
	//Check if ID is valid
	if (id >= eNumberOfStates)
	{
		#ifdef DEBUG_FC_MACHINE
			my_console.WriteToSplitConsole("FCStateError_InvalidIDNumber " + std::to_string(id), SPLIT_FC);
		#endif
		return eFCStateError_InvalidIDNumber;
	}

	//Set one bit according to the ID
	this->transition = id;

	return eSuccess;
}

int FCMachine::get_trans()
{
	return this->transition;
}
