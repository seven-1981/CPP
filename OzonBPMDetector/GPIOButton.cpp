//The GPIO implementation is not used on WIN32
#ifndef _WIN32

#include "GPIOButton.hpp"
#include "SplitConsole.hpp"
#include "bpm_globals.hpp"
#include "bpm_param.hpp"
#include <chrono>

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

GPIOButton::GPIOButton()
{
	//Execute init function - ignore return value
	//The result is stored in member variable is_initialized
	init_button();
}

GPIOButton::~GPIOButton()
{
	//We delete all the instances
	delete(listener);
}

eError GPIOButton::init_button()
{
	//Write debug line
	if (param_list.get<bool>("debug button") == true)
		my_console.WriteToSplitConsole("GPIO Button Class: Created button.", param_list.get<int>("split main"));

	//Declare return value
	eError retval = eSuccess;

	//Set initial state
	this->state_handler = Handler_ButtonReleased;
	this->state_button = ButtonReleased;
	this->state_button_old = ButtonReleased;
	this->pressed = std::chrono::high_resolution_clock::now();
	this->released = std::chrono::high_resolution_clock::now();

	//Create listener
	this->listener = new(std::nothrow) GPIOListener();

	//Check for proper initialization
	if (this->listener == nullptr || this->listener->check_init() != eSuccess)
	{
		this->is_initialized = false;
		retval = eGPIOButtonNotInitialized;
	}
	else
		this->is_initialized = true;

	return retval;
}

eButtonHandlerState GPIOButton::get_state()
{
	//Method determines listener state and performs some timing
	//analysis to set one of the states in enum eButtonHandlerState

	//First, update listener
	this->listener->update();

	//Flag to determine if we can change handler state
	bool update_state = false;

	//If button has been pressed, save timestamp
	this->state_button = this->listener->get_state();

	if (this->state_button != this->state_button_old 
	 && this->state_button == ButtonPressed)
	{
		this->pressed = std::chrono::high_resolution_clock::now();
		this->state_button_old = this->state_button;
	}

	//If button has been released, save timestamp
	if (this->state_button != this->state_button_old 
	 && this->state_button == ButtonReleased)
	{
		this->released = std::chrono::high_resolution_clock::now();
		this->state_button_old = this->state_button;	
		//We update state on release of button
		update_state = true;
	}	
		
	//Determine the timestamp difference
	long ms = std::chrono::duration_cast<std::chrono::milliseconds>(released - pressed).count();

	//Evaluate difference
	if (update_state == true)
	{
		//How long is the difference?
		if (ms >= TIME_LONG_MS)
		{
			this->state_handler = Handler_ButtonHeldLong;
			if (param_list.get<bool>("debug button") == true)
				my_console.WriteToSplitConsole("GPIO Button Class: HELD LONG.", param_list.get<int>("split main"));
		}
		else if (ms >= TIME_SHORT_MS)
		{
			this->state_handler = Handler_ButtonHeldShort;
			if (param_list.get<bool>("debug button") == true)
				my_console.WriteToSplitConsole("GPIO Button Class: HELD SHORT.", param_list.get<int>("split main"));
		}
		else 
		{
			this->state_handler = Handler_ButtonPressed;
			if (param_list.get<bool>("debug button") == true)
				my_console.WriteToSplitConsole("GPIO Button Class: PRESSED.", param_list.get<int>("split main"));
		}

		//Store timestamp
		this->updated = std::chrono::high_resolution_clock::now();
	}

	//After a short amount of time, set the handler state back
	auto time_now = std::chrono::high_resolution_clock::now();
	long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_now - this->updated).count();
	if (elapsed >= TIME_RESET)
	{
		this->state_handler = Handler_ButtonReleased;
	}
			
	//Return actual state
	return this->state_handler;
}

eError GPIOButton::check_init()
{
	//Check and return init status
	eError retval = eGPIOButtonNotInitialized;

	if (this->is_initialized == true)
		retval = eSuccess;

	return retval;
}

#endif