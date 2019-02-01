#ifndef _FCEVENT_H
#define _FCEVENT_H

#include "bpm_globals.hpp"
#include "bpm_audio.hpp"
#include "bpm_analyze.hpp"
#include <string>
#include <iostream>
#include <type_traits>

//Class for event - also acts as base class for FCTimer class
//Events are used by FCQueue. One event is something like a functionid, meaning that
//it can hold one of a bunch of function pointers. By pushing one event into the OS queue,
//the appropriate function is executed. Function pointers can either be:
//Simple - no class member - only function pointer is stored
//Member function - the function is member of class - we have to store function pointer and class instance
//Argument - events can handle arguments
//ID is used for register access (return value of underlying function)
//Select is used to determine the function to be executed by the OS queue loop
class FCEvent
{
public:
	//ID is used for register designation
	int ID = -1;

	//Select value is used to choose the correct function pointer by the execute method
	int select = -1;

	//True, if the event is configured
	bool success = false;

	//True, if event register has to be cleared after read
	bool clear = true;

	//Used for parameter handover
	union arguments
	{
		int int_argument;
		short* pshort_argument;
		double double_argument;
	} arguments;

	//We only use one function pointer at a time
	union functions
	{
		//Functions with no arguments
		void(*void_function)();
		int(*int_function)();
		std::string(*string_function)();
		eError(BPMAudio::*audio_function)();
		eError(BPMAnalyze::*analyze_function)();
		double(BPMAnalyze::*double_analyze_function)();
		//Functions with arguments
		void(*void_int_function)(int);
		eError(BPMAnalyze::*analyze_pshort_function)(short*);
		eError(BPMAnalyze::*analyze_double_function)(double);
	} functions;

	//Same for instances
	union instances
	{
		BPMAudio* audio_instance;
		BPMAnalyze* analyze_instance;
	} instances;

	//Constructors - inline
	FCEvent()
	{
		//We set the ID to -1
		this->ID = -1;
	}

	FCEvent(int id)
	{
		//We just set the ID at this point
		//Init to be executed later
		this->ID = id;
	}

	//Destructor - inline
	~FCEvent()
	{
		//Destructor not used
	}

	//Init function - class member function
	template <typename R, class T>
	eError init_event(R(T::*function)(), T* instance, bool clear = true);

	//Init function - class member function - with argument
	template <typename R, class T, typename A>
	eError init_event(R(T::*function)(A), T* instance, bool clear = true);

	//Init function - global function pointer
	template <typename T>
	eError init_event(T(*function)(), bool clear = true);

	//Init function - global function pointer - with argument
	template <typename T, typename A>
	eError init_event(T(*function)(A), bool clear = true);
};

template <typename T>
eError FCEvent::init_event(T(*function)(), bool clear)
{
	//Declare return value	
	eError retval = eFCQueueError_FCEventNotInitialized;

	//Set clear flag
	this->clear = clear;

	//Check the template class type
	if (std::is_same<T, void>::value)
	{
		this->functions.void_function = (void(*)())function;
		this->select = eVoidFunction;
		this->success = true;
		retval = eSuccess;
	}

	if (std::is_same<T, int>::value)
	{
		this->functions.int_function = (int(*)())function;
		this->select = eIntFunction;
		this->success = true;
		retval = eSuccess;
	}

	if (std::is_same<T, std::string>::value)
	{
		this->functions.string_function = (std::string(*)())function;
		this->select = eStringFunction;
		this->success = true;
		retval = eSuccess;
	}

	//Check if ID is in valid range
	if (this->ID < 0 || this->ID >= eLastTimer)
		retval = eFCQueueError_FCEventNotInitialized;

	return retval;	
}

template <typename T, typename A>
eError FCEvent::init_event(T(*function)(A), bool clear)
{
	//Declare return value	
	eError retval = eFCQueueError_FCEventNotInitialized;

	//Set clear flag
	this->clear = clear;

	//Check the template class type
	if ((std::is_same<T, void>::value) && (std::is_same<A, int>::value))
	{
		this->functions.void_int_function = (void(*)(int))function;
		this->select = eVoidIntFunction;
		this->success = true;
		retval = eSuccess;
	}

	//Check if ID is in valid range
	if (this->ID < 0 || this->ID >= eLastTimer)
		retval = eFCQueueError_FCEventNotInitialized;

	return retval;	
}

template <typename R, class T>
eError FCEvent::init_event(R(T::*function)(), T* instance, bool clear)
{
	//Declare return value	
	eError retval = eFCQueueError_FCEventNotInitialized;

	//Set clear flag
	this->clear = clear;
	
	//Check the template class type
	if ((std::is_same<R, eError>::value) && (std::is_same<T, BPMAudio>::value))
	{
		//Function from BPMAudio class - return type eError
		this->functions.audio_function = (eError(BPMAudio::*)())function;
		this->instances.audio_instance = (BPMAudio*)instance;
		this->select = eBPMAudioFunction;
		this->success = true;
		retval = eSuccess;
	}

	if ((std::is_same<R, eError>::value) && (std::is_same<T, BPMAnalyze>::value))
	{
		//Function from BPMAnalyze class - return type eError
		this->functions.analyze_function = (eError(BPMAnalyze::*)())function;
		this->instances.analyze_instance = (BPMAnalyze*)instance;
		this->select = eBPMAnalyzeFunction;
		this->success = true;
		retval = eSuccess;
	}

	if ((std::is_same<R, double>::value) && (std::is_same<T, BPMAnalyze>::value))
	{
		//Function from BPMAnalyze class - return type double
		this->functions.double_analyze_function = (double(BPMAnalyze::*)())function;
		this->instances.analyze_instance = (BPMAnalyze*)instance;
		this->select = eBPMAnalyzeFunctionDouble;
		this->success = true;
		retval = eSuccess;
	}

	//Check if ID is in valid range
	if (this->ID < 0 || this->ID >= eLastTimer)
		retval = eFCQueueError_FCEventNotInitialized;

	return retval;
}

template <typename R, class T, typename A>
eError FCEvent::init_event(R(T::*function)(A), T* instance, bool clear)
{
	//Declare return value	
	eError retval = eFCQueueError_FCEventNotInitialized;
	
	//Set clear flag
	this->clear = clear;
	
	//Check the template class type
	if (   (std::is_same<R, eError>::value) 
            && (std::is_same<T, BPMAnalyze>::value) 
	    && (std::is_same<A, short*>::value))
	{
		//Function from BPMAnalyze class - return type eError, argument short*
		this->functions.analyze_pshort_function = (eError(BPMAnalyze::*)(short*))function;
		this->instances.analyze_instance = (BPMAnalyze*)instance;
		this->select = eBPMAnalyzepShortFunction;
		this->success = true;
		retval = eSuccess;
	}

	//Check the template class type
	if (   (std::is_same<R, eError>::value) 
            && (std::is_same<T, BPMAnalyze>::value) 
	    && (std::is_same<A, double>::value))
	{
		//Function from BPMAnalyze class - return type eError, argument double
		this->functions.analyze_double_function = (eError(BPMAnalyze::*)(double))function;
		this->instances.analyze_instance = (BPMAnalyze*)instance;
		this->select = eBPMAnalyzerdoubleFunction;
		this->success = true;
		retval = eSuccess;
	}

	//Check if ID is in valid range
	if (this->ID < 0 || this->ID >= eLastTimer)
		retval = eFCQueueError_FCEventNotInitialized;

	return retval;
}

#endif
