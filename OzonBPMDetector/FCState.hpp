#ifndef _FCSTATE_H
#define _FCSTATE_H

#include "bpm_globals.hpp"

//Enum for possible states
enum FCStates
{
	StateEntr = 1,
	StateLoop = 2,
	StateExit = 3
};

//State class
class FCState
{
private:
	//ID counter of the state
	//Is increased for every instance creation
	static int count;
	//This instance's ID
	int my_id;
	//State of the state instance
	FCStates state;
	//Transition, next state's ID
	int next_id;
	//Initialized flag
	bool is_initialized;
	
	//Function pointers
	void(*entry_function)();
	FCStates(*loop_function)();
	int(*exit_function)();

public:
	//Constructor
	FCState();
	//Destructor
	~FCState();

	//Bind methods used to pass function pointers to execute
	//to the state instance
	eError bind_entr(void(*function)());
	eError bind_loop(FCStates(*function)());
	eError bind_exit(int(*function)());

	//Execute method
	//This method must be executed periodically
	eError execute();

	//Public member functions
	//Get the next state's ID
	int get_next_id();
	//Set the next state's ID
	eError set_next_id(int id);
	//Update init info
	void set_init();
	//Check init info
	bool check_init();
};

#endif
