//The GPIO implementation is not used on WIN32
#ifndef _WIN32

#include "GPIOListener.hpp"
#include <wiringPi.h>
#include "bpm_globals.hpp"
#include "bpm_param.hpp"
#include "SplitConsole.hpp"

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

GPIOListener::GPIOListener()
{
	//Execute init function - ignore return value
	//The result is stored in member variable is_initialized
	init_listener();
}

GPIOListener::~GPIOListener()
{
	//We delete all the instances
	delete(pin);
}

eError GPIOListener::init_listener()
{
	//Write debug line
	if (param_list.get<bool>("debug listener") == true)
		my_console.WriteToSplitConsole("GPIO Listener Class: Created listener.", param_list.get<int>("split main"));

	//Declare return value
	eError retval = eSuccess;

	//Set initial state
	this->state_button = ButtonReleased;
	this->state_listener = StateReleased;
	this->pin_state = false;
	this->pin_state_old = false;
	this->scans = 0;
	this->scans_ok = 0;

	//Address pins
	this->pin = new(std::nothrow) GPIOPin(RESET_PIN_1, eIN, false);

	//Check for proper initialization
	if (this->pin == nullptr || this->pin->check_init() != eSuccess)
	{
		this->is_initialized = false;
		retval = eGPIOListenerNotInitialized;
	}
	else
		this->is_initialized = true;

	return retval;
}

eButtonState GPIOListener::get_state()
{
	//Return actual state
	return this->state_button;
}

eError GPIOListener::check_init()
{
	//Read init flag and set return value
	eError retval = eGPIOListenerNotInitialized;

	if (this->is_initialized == true)
		retval = eSuccess;

	return retval;
}

eError GPIOListener::update()
{
	eError retval = eSuccess;

	//Update process of the GPIO Listener
	if (this->is_initialized == false)
		return eGPIOListenerNotInitialized;

	//Get actual raw pin state
	this->pin_state = this->pin->getval_gpio();

	//Debug variables
	static bool pressed = false;
	static bool released = false;

	switch (this->state_listener)
	{
		case StateReleasedEntered:
			//Set back scan counts
			this->scans = 0;
			this->scans_ok = 0;
			//Set next state
			this->state_listener = StateReleased;
			break;

		case StateReleased:
			//Set button state
			this->state_button = ButtonReleased;
			//Increase counts (deadtime)
			this->scans++;
			//Start checking after a certain amount of scans
			if (this->scans >= NUMBER_OF_SCANS_OFF)
			{
				//We check if there was a change in the raw state
				if (this->pin_state != this->pin_state_old && this->pin_state == true)
				{
					//Yes, there was a change
					this->pin_state_old = this->pin_state;
					//We set next state
					this->state_listener = StatePosEdge;
					//Reset scans
					this->scans = 0;
				}
				//Write debug line
				if (param_list.get<bool>("debug listener") == true)
				{
					if (released == false)
						my_console.WriteToSplitConsole("GPIO Listener Class: Button RELEASED state.", param_list.get<int>("split main"));
					released = true;
					pressed = false;
				}
			}
			break;

		case StatePosEdge:
			//Positive edge false->true detected
			//Now we count how many times in a row a 'true' is detected
			//If there's only one or two, we assume the button is still released
			this->scans++;
			if (this->pin_state == true)
				this->scans_ok++;

			//Update old state since it is not used in this state
			this->pin_state_old = this->pin_state;

			//Check number of scans
			if (this->scans >= NUMBER_OF_SCANS)
			{
				//Number of scans reached. Here we check how many were ok (=true)
				if (this->scans_ok >= NUMBER_OF_SCANS_OK)
				{
					//Most of the scans were ok. We can proceed to the next state
					this->state_listener = StatePressedEntered;
				}
				else
				{
					//Too many scans were not ok (=false). We stay in the released state
					this->state_listener = StateReleasedEntered;
				}
			}
			break;

		case StatePressedEntered:
			//Set back scan counts
			this->scans = 0;
			this->scans_ok = 0;
			//Set next state
			this->state_listener = StatePressed;
			break;

		case StatePressed:
			//Set button state
			this->state_button = ButtonPressed;
			//Increase counts (deadtime)
			this->scans++;
			//Start checking after a certain amount of scans
			if (this->scans >= NUMBER_OF_SCANS_OFF)
			{
				//We check if there was a change in the raw state
				if (this->pin_state != this->pin_state_old && this->pin_state == false)
				{
					//Yes, there was a change
					this->pin_state_old = this->pin_state;
					//We set next state
					this->state_listener = StateNegEdge;
					//Reset scans
					this->scans = 0;
				}
				//Write debug line
				if (param_list.get<bool>("debug listener") == true)
				{
					if (pressed == false)
						my_console.WriteToSplitConsole("GPIO Listener Class: Button PRESSED state.", param_list.get<int>("split main"));
					pressed = true;
					released = false;
				}
			}
			break;

		case StateNegEdge:
			//Negative edge true->false detected
			//Now we count how many times in a row a 'false' is detected
			//If there's only one or two, we assume the button is still pressed
			this->scans++;
			if (this->pin_state == false)
				this->scans_ok++;

			//Update old state since it is not used in this state
			this->pin_state_old = this->pin_state;

			//Check number of scans
			if (this->scans >= NUMBER_OF_SCANS)
			{
				//Number of scans reached. Here we check how many were ok (=false)
				if (this->scans_ok >= NUMBER_OF_SCANS_OK)
				{
					//Most of the scans were ok. We can proceed to the next state
					this->state_listener = StateReleasedEntered;
				}
				else
				{
					//Too many scans were not ok (=true). We stay in the pressed state
					this->state_listener = StatePressedEntered;
				}
			}
			break;
	}

	return retval;
}

#endif