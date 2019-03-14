#ifndef _BPM_H
#define _BPM_H

#include "GPIOPin.hpp"
#include "GPIOWriter.hpp"
#include "GPIOButton.hpp"
#include "FCState.hpp"
#include "FCMachine.hpp"
#include "FCQueue.hpp"
#include "FCSocket.hpp"
#include "FCWindowLabel.hpp"
#include "SplitConsole.hpp"
#include "bpm_audio.hpp"
#include "bpm_analyze.hpp"
#include "bpm_param.hpp"
#include <sstream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <future>
#include <vector>

//We use a special splitted console
//This one has to be defined 'external' in other files
SplitConsole my_console(NUMBER_OF_SPLITS);
//Generate parameter list for application
//Has to be defined 'external' in files that use parameter list
ParamList param_list;

//Struct for application information
struct FCApplInfo
{
	//Pointer for operating system queue access
	FCQueue* os_queue;
	//Threads can't return values, we use std::async and future instead
	std::future<eError> os_thread;
	std::future<eError> os_SIR;

	//Pointer for state machine access
	FCMachine* os_machine;
	//Future for state machine thread
	//Threads can't return values, we use std::async and future instead
	std::future<eError> machine_thread;

	//Timestamp of starting time
	std::chrono::high_resolution_clock::time_point start;

	//Socket for TCP connection
	//FCWindow handle
	#ifndef _WIN32
		FCSocket* os_socket;
		//Future for socket 
		std::future<eError> socket_thread;

		//HDMI status
		bool hdmi_attached;
		//Window handle
		FCWindowLabel* window;
	#endif
} appl_info;

//Struct for GPIO information
struct GPIOInfo
{
	//Pointer for gpio access
	GPIOWriter* gpio_writer;
	GPIOButton* gpio_button;
} gpio_info;

//Struct for BPM counter related information
struct BPMInfo
{
	BPMAudio* bpm_audio;
	BPMAnalyze* bpm_analyze;
} bpm_info;

//Vector for events and timers. Note: the vector is of pointer of FCEvent type, the timers
//are of pointer of FCTimer type, which is derived from FCEvent -> therefore the vector can
//contain both object types (a pointer to an object of a base class can point to any object from
//classes that derive from the base class - polymorphism).
std::vector<FCEvent*> event_info;

//Auxiliary functions for main and bpm.cpp

//Wrapper for determining status of std::future
//It will return true, if the underlying std::async has been finished
template<typename R>
bool is_ready(std::future<R> const& f)
{ 
	return (f.wait_for(std::chrono::seconds(0)) == std::future_status::ready); 
}

//Converting string with double number precision (which is 6 per default) to another value
template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::setw(4) << std::fixed << a_value;
    return out.str();
}

//Retrieving user input
std::string get_user_input()
{
	//Declare prompt response
	std::string user_value = "";

	//Get split param
	int split = param_list.get<int>("split input");
	
	//Display command prompt
	user_value = my_console.ReadFromSplitConsole("OZON BPM prompt -> ", split);

	return user_value;
}

//Checking input from prompt for invalid characters
//Only numbers are accepted
int handle_user_input(std::string user_value)
{
	//Declare return value
	unsigned int retval = 0;
	
	//Declare working string
	std::string s;
	
	//Loop through string chars and remove invalid
	for (char& c : user_value)
	{
		if (c >= 48 && c <= 57)
			s = s + c;
	}
	
	//Verify if string is empty
	if (s.empty() == true)
	{
		retval = -1;
	}
	else
	{	
		//Here, it is still possible that the number is greater than uint max (65535)
		//so we just truncate the string to 2 chars - 100 different commands are enough
		s = s.substr(0, 2);

		//Convert the string to a number
		//std::stoi could throw invalid_argument or out_of_range exceptions
		try
		{
			retval = std::stoi(s);
		}
		catch (...)
		{
			//No matter what exception was thrown, we return -1
			retval = -1;
		}
	}
		
	//Return value
	return retval;
}

//Manual mode
void manual_mode()
{
	//Manual mode
	//Get user input from console and display it using GPIO writer
	
	//Continue or not
	bool cont = true;

	//Declare prompt response
	std::string user_value = "";

	//Get split param
	int split = param_list.get<int>("split fc");

	//Check user input
	while (cont == true)
	{
		//Get user input
		user_value = get_user_input();
		if (user_value.compare(std::to_string(0)) == 0)
		{
			cont = false;
			my_console.WriteToSplitConsole("Leaving manual mode.", split);
			continue;
		}

		SEND_STR_DATA(eManMode, user_value, false);
	}
}

#endif