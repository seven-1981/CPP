#include "FCState.hpp"
#include "SplitConsole.hpp"
#include "bpm_param.hpp"
#include <iostream>
#include <thread>

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

//Static instance ID counter
int FCState::count;

//Class FCState
//Constructor
FCState::FCState()
{
	//Initialize instance state
	this->state = StateEntr;
	//Get new ID and increase counter
	this->my_id = count;
	count++;
	//Initialize next ID to -1
	this->next_id = -1;
	//Init info is false until function pointers set
	this->is_initialized = false;

	//Write debug line
	if (param_list.get<bool>("debug state") == true)
		my_console.WriteToSplitConsole("Initialized FCState " + std::to_string(my_id), param_list.get<int>("split fc"));

}

//Destructor
FCState::~FCState()
{
	if (param_list.get<bool>("debug state") == true)
		my_console.WriteToSplitConsole("Removed FCState " + std::to_string(my_id), param_list.get<int>("split fc"));
}

//Bind methods for function pointers
eError FCState::bind_entr(void(*function)())
{
	//Check for null pointer
	if (function == nullptr)
	{
		if (param_list.get<bool>("debug state") == true)
			my_console.WriteToSplitConsole("FCStateError_BindFunctionNullPtr " + std::to_string(my_id), param_list.get<int>("split errors"));
		return eFCStateError_BindFunctionNullPtr;
	}

	this->entry_function = function;
	//Update init info
	set_init();

	//Write debug line
	if (param_list.get<bool>("debug state") == true)
		my_console.WriteToSplitConsole("Binding entry function done for " + std::to_string(my_id), param_list.get<int>("split fc"));

	return eSuccess;
}

eError FCState::bind_loop(FCStates(*function)())
{
	//Check for null pointer
	if (function == nullptr)
	{
		if (param_list.get<bool>("debug state") == true)
			my_console.WriteToSplitConsole("FCStateError_BindFunctionNullPtr " + std::to_string(my_id), param_list.get<int>("split errors"));

		return eFCStateError_BindFunctionNullPtr;
	}

	this->loop_function = function;
	//Update init info
	set_init();

	//Write debug line
	if (param_list.get<bool>("debug state") == true)
		my_console.WriteToSplitConsole("Binding loop function done for " + std::to_string(my_id), param_list.get<int>("split fc"));

	return eSuccess;
}

eError FCState::bind_exit(int(*function)())
{
	//Check for null pointer
	if (function == nullptr)
	{
		if (param_list.get<bool>("debug state") == true)
			my_console.WriteToSplitConsole("FCStateError_BindFunctionNullPtr " + std::to_string(my_id), param_list.get<int>("split errors"));

		return eFCStateError_BindFunctionNullPtr;
	}

	this->exit_function = function;
	//Update init info
	set_init();

	//Write debug line
	if (param_list.get<bool>("debug state") == true)
		my_console.WriteToSplitConsole("Binding exit function done for " + std::to_string(my_id), param_list.get<int>("split fc"));

	return eSuccess;
}

//Execute method
eError FCState::execute()
{
	//First we have to check, if functions are initialized
	//It is enough, if the loop function is specified
	//The others aren't mandatory
	if (this->is_initialized == false)
	{
		if (param_list.get<bool>("debug state") == true)
			my_console.WriteToSplitConsole("FCStateError_FCStateNotInitialized " + std::to_string(my_id), param_list.get<int>("split errors"));

		return eFCStateError_FCStateNotInitialized;
	}

	//Depending on the state, we execute the bound function
	switch (this->state)
	{
		case StateEntr:
			if (param_list.get<bool>("debug state") == true)
				my_console.WriteToSplitConsole("Executing entry function Id = " + std::to_string(my_id), param_list.get<int>("split fc"));
			entry_function();
			this->state = StateLoop;
			break;
		case StateLoop:
			if (param_list.get<bool>("debug state") == true)
				my_console.WriteToSplitConsole("Executing loop function Id = " + std::to_string(my_id), param_list.get<int>("split fc"));
			this->state = loop_function();
			break;
		case StateExit:
			if (param_list.get<bool>("debug state") == true)
				my_console.WriteToSplitConsole("Executing exit function Id = " + std::to_string(my_id), param_list.get<int>("split fc"));
			this->next_id = exit_function();
			//Here, we have to check if the next state is
			//defined (not -1)
			if (this->next_id == -1)
			{
				if (param_list.get<bool>("debug state") == true)
					my_console.WriteToSplitConsole("FCStateError_FCStateNextIdNotDefined " + std::to_string(my_id), param_list.get<int>("split errors"));
				return eFCStateError_FCStateNextIdNotDefined;
			}
			this->state = StateEntr;
			break;
	}

	return eSuccess;
}

//Public member functions
//Getter, setter
int FCState::get_next_id()
{
	return this->next_id;
}

eError FCState::set_next_id(int id)
{
	//We check the ID
	if (id < -1)
	{
		if (param_list.get<bool>("debug state") == true)
			my_console.WriteToSplitConsole("FCStateError_InvalidIDNumber " + std::to_string(my_id), param_list.get<int>("split errors"));
		return eFCStateError_InvalidIDNumber;
	}

	//We set the ID
	this->next_id = id;

	return eSuccess;
}

void FCState::set_init() 
{
	if (this->entry_function == nullptr || this->loop_function == nullptr || this->exit_function == nullptr)
		is_initialized = false;
	else
		is_initialized = true;
}

bool FCState::check_init()
{
	return is_initialized;
}
