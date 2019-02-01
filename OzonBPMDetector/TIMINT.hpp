#ifndef _TIMER_INTERRUPT_H
#define _TIMER_INTERRUPT_H

#include <iostream>
#include <cstring>
#include <sys/time.h>
#include <signal.h>

namespace TIMINT
{
	//Start timer
	//Uses sigevent == NULL, therefore some standard settings are used
	//Set callback with function set_callback(void(*)(int, siginfo_t*, void*))
	void start_timer(timer_t* timer_ID, int sec, int usec)
	{
		struct itimerspec timer;
		
		timer.it_value.tv_sec = sec;
		timer.it_value.tv_nsec = usec * 1000;
		timer.it_interval.tv_sec = sec;
		timer.it_interval.tv_nsec = usec * 1000;

		timer_create(CLOCK_REALTIME, NULL, timer_ID);
		timer_settime(*timer_ID, 0, &timer, NULL);
	}

	//Start timer
	//Uses sigevent, settings provided by user
	//Callback provided with set_callback(sigevent&, void(*)(sigval_t), T*)
	void start_timer(timer_t* timer_ID, sigevent& se, int sec, int usec)
	{
		struct itimerspec timer;
		
		timer.it_value.tv_sec = sec;
		timer.it_value.tv_nsec = usec * 1000;
		timer.it_interval.tv_sec = sec;
		timer.it_interval.tv_nsec = usec * 1000;

		timer_create(CLOCK_REALTIME, &se, timer_ID);
		timer_settime(*timer_ID, 0, &timer, NULL);
	}		

	void stop_timer(timer_t* timer_ID)
	{
		struct itimerspec timer;
		
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_nsec = 0;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_nsec = 0;

		timer_settime(*timer_ID, 0, &timer, NULL);
	}

	//Set event struct
	//If timer elapses, function defined by notify_function is called
	//as if it was a start function of a new thread. Instance of context
	//is passed with third argument.
	template <typename T>
	void set_callback(sigevent& se, void(*handler)(sigval_t), T* instance)
	{
		memset(&se, 0, sizeof(se));
		se.sigev_notify = SIGEV_THREAD;
		se.sigev_notify_function = handler;
		se.sigev_value.sival_ptr = instance;
	}

	//Register callback for default notification settings
	void set_callback(void(*callback)(int, siginfo_t*, void*))
	{
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = callback;
		sigemptyset(&sa.sa_mask);
		if (sigaction(SIGALRM, &sa, NULL) == -1)
			std::cout << "Error sigaction." << std::endl;
	}
};

#endif