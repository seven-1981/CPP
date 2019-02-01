#ifndef _GPIO_BUTTON_H
#define _GPIO_BUTTON_H

//GPIO Button
//Handles Reset button for OZON bpm counter
//Runs a thread internally which updates button state periodically
//Button instance is meant to check periodically in SIR routine

#include "GPIOListener.hpp"
#include "bpm_globals.hpp"
#include <future>
#include <chrono>

#define TIME_SHORT_MS  2000
#define TIME_LONG_MS   5000
#define TIME_RESET     1000

enum eButtonHandlerState
{
	Handler_ButtonReleased,
	Handler_ButtonPressed,
	Handler_ButtonHeldShort,
	Handler_ButtonHeldLong
};

class GPIOButton
{
public:
	GPIOButton();
	~GPIOButton();

	//Getter method for button state
	//Must be called periodically
	eButtonHandlerState get_state();
	//Check init flag
	eError check_init();

private:
	//Initialized state
	bool is_initialized;
	//GPIO Listener pointer
	GPIOListener* listener;
	//Init method
	eError init_button();
	//State of the button handler
	eButtonHandlerState state_handler;
	//Raw state of button
	eButtonState state_button;
	eButtonState state_button_old;
	//Timestamps of button press
	std::chrono::time_point<std::chrono::high_resolution_clock> pressed;
	std::chrono::time_point<std::chrono::high_resolution_clock> released;
	std::chrono::time_point<std::chrono::high_resolution_clock> updated;
};

#endif