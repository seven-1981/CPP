#ifndef _GPIO_LISTENER_H
#define _GPIO_LISTENER_H

#include "bpm_globals.hpp"
#include "GPIOPin.hpp"

#define NUMBER_OF_SCANS 5
#define NUMBER_OF_SCANS_OK 3
#define NUMBER_OF_SCANS_OFF 5
#define SLEEP_LISTENER_MS 10

enum eButtonState
{
	ButtonPressed,
	ButtonReleased
};

enum eListenerState
{
	StatePosEdge,
	StatePressedEntered,
	StatePressed,
	StateNegEdge,
	StateReleasedEntered,
	StateReleased
};

class GPIOListener
{
public:
	GPIOListener();
	~GPIOListener();

	//Called to get actual state
	eButtonState get_state();
	//Periodically update state
	eError update();
	//Check init flag
	eError check_init();

private:
	//Initialized state
	bool is_initialized;
	//Pin to monitor
	GPIOPin* pin;
	//Init method
	eError init_listener();
	//State of the button and the listener
	eButtonState state_button;
	eListenerState state_listener;
	//Raw states of pin
	bool pin_state;
	bool pin_state_old;
	//Count scans
	long scans;
	long scans_ok;
};

#endif