#ifndef _FCTIMER_H
#define _FCTIMER_H

#include "bpm_globals.hpp"
#include <iostream>

//Class for timer
//Inherited from event - we can consider a timer a similar struct to an event
//with some additional timing functionality
class FCTimer : public FCEvent
{
public:
	//Constructors - inline
	FCTimer()
	{
		//We set the ID to -1
		this->ID = -1;
	}

	FCTimer(int id, unsigned long int tv)
	{
		//We just set the ID at this point
		//Init to be executed later
		this->ID = id;

		//Set the timervalue
		this->timer_value = tv;
	}

	//Destructor - inline
	~FCTimer()
	{
		//Destructor not used
	}

	//Additional timing information
	//True if timer has been started
	bool started = false;

	//Starting timepoint saved
	long long start_value = 0;

	//Timervalue
	long long timer_value = 0;
};

#endif