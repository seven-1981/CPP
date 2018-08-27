#ifndef _FCMACHINE_H
#define _FCMACHINE_H

#include "bpm_globals.hpp"
#include "FCState.hpp"
#include <thread>
#include <future>

//Class for the state machine
class FCMachine
{
private:
	//Array of state instances (pointers)
	FCState* states[eNumberOfStates];
	//Current state being executed
	FCState* active_state;
	//Init info
	bool is_initialized;
	//Size info
	int size_info;
	//Transitions
	int transition;
	//Stop flag
	bool stop;

	//Loop function - auxiliary method for start_machine
	eError machine_loop();

public:
	//Constructor
	FCMachine();

	//Destructor
	~FCMachine();

	//Bind methods for function pointers
	//Entry function - signature f(void*(), int)
	eError bind_function(void(*function)(), int id);

	//Loop function - signature f(FCStates*(), int)
	eError bind_function(FCStates(*function)(), int id);

	//Exit function - signature f(unsigned int*(), int)
	eError bind_function(int(*function)(), int id);

	//Execute method
	//This method must be executed periodically
	eError execute();

	//Stop method
	void machine_stop();

	//Create thread with executing loop for state machine
	std::future<eError> start_machine();

	//Set transition
	eError set_trans(int id);

	//Get transition info
	int get_trans();
};

#endif

