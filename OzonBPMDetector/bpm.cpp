#include <iostream>
#include <fstream>
#include <string>
#include <vector>

//The following headers aren't available on WIN32
#ifndef _WIN32
	#include <wiringPi.h>
	#include "GPIOPin.hpp"
	#include "GPIOWriter.hpp"
	#include "GPIOListener.hpp"
	#include "GPIOButton.hpp"

//Allowing C-header file to be compiled with C++
extern "C" {
#include <xdo.h>
}
#endif

#include "bpm_globals.hpp"
#include "bpm.hpp"
#include "bpm_audio.hpp"
#include "bpm_analyze.hpp"
#include "bpm_functions.hpp"
#include "FCState.hpp"
#include "FCMachine.hpp"
#include "FCQueue.hpp"
#include "FCEvent.hpp"
#include "SplitConsole.hpp"

//Timer values used
//After this time, the statemachine automatically proceeds from init to bpm counter state
#define INIT_TIME 5000000
//Supervision timer used for queue
#define QUEUE_TIMEOUT 6000000
//Audio capture timeout 
#define AUDIO_TIMEOUT 3000000
//RMS hysteresis timer
#define RMS_HYSTERESIS 2000000

eError init_appl()
{
	//Declare return value
	eError retval = eSuccess;

	int split = param_list.get<int>("split main");
	if (param_list.get<bool>("debug main") == true)
	{
		my_console.WriteToSplitConsole("Application init function.", param_list.get<int>("split main"));
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	}

	//Create queue
	appl_info.os_queue = new(std::nothrow) FCQueue;

	//Create new state machine
	appl_info.os_machine = new(std::nothrow) FCMachine;

	//Get start time point of application
	appl_info.start = std::chrono::high_resolution_clock::now();

	//Create socket
	#ifndef _WIN32
		appl_info.os_socket = new(std::nothrow) FCSocket;
	#endif

	//Check if objects have been created successfully
	if ((appl_info.os_queue == nullptr) || (appl_info.os_machine == nullptr))
	{
		retval = eMemoryAllocationFailed;
	}

	#ifndef _WIN32
		if ((appl_info.os_socket == nullptr) || (appl_info.os_socket->check_init() != eSuccess))
		{
			retval = eMemoryAllocationFailed;
		}
	#endif

	return retval;
}

eError init_gpio()
{

	//Declare return value
	eError retval = eSuccess;

	#ifndef _WIN32

	if (param_list.get<bool>("debug main") == true)
	{
		my_console.WriteToSplitConsole("GPIO init function.", param_list.get<int>("split main"));
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	}

	//Initialize GPIO
	int result = GPIOPin::init_gpio();

	if (param_list.get<bool>("debug main") == true)
		my_console.WriteToSplitConsole("Initialize wiringPi... return = " + std::to_string(result), param_list.get<int>("split main"));

	if (result != 0)
	{
		retval = eWiringPiNotInitialized;
	}

	//Create GPIO writer
	gpio_info.gpio_writer = new(std::nothrow) GPIOWriter();

	//Create GPIO button
	gpio_info.gpio_button = new(std::nothrow) GPIOButton();

	//Check if objects have been created successfully
	if ((gpio_info.gpio_writer == nullptr) || (gpio_info.gpio_button == nullptr)
	    || gpio_info.gpio_button->check_init() != eSuccess)
	{
		retval = eMemoryAllocationFailed;
	}

	#endif

	return retval;
}

eError init_bpm()
{
	//Declare return value
	eError retval = eSuccess;

	//Initialize BPM - audio and analysis
	if (param_list.get<bool>("debug main") == true)
	{
		my_console.WriteToSplitConsole("BPM init function.", param_list.get<int>("split main"));
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	}

	//Create BPM Audio class instance
	bpm_info.bpm_audio = new(std::nothrow) BPMAudio();

	//Create BPM Analyzer class instance
	bpm_info.bpm_analyze = new(std::nothrow) BPMAnalyze();

	//Check if objects have been created successfully and if audio is set up ok
	if ((bpm_info.bpm_audio == nullptr) || (bpm_info.bpm_analyze == nullptr) ||
	    (bpm_info.bpm_audio->get_state() != eSuccess))
	{
		retval = eMemoryAllocationFailed;
	}

	return retval;
}

eError init_events()
{
	//We create and initialize all the events and timers used
	if (param_list.get<bool>("debug main") == true)
	{
		my_console.WriteToSplitConsole("Event/timer init function.", param_list.get<int>("split main"));
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	}

	//Declare return value
	std::vector<eError> retval;
	eError success = eSuccess;

	//Create the events
	event_info.push_back(new(std::nothrow) FCEvent(eGoToStateBPM));
	event_info.push_back(new(std::nothrow) FCEvent(eGoToStateTimer));
	event_info.push_back(new(std::nothrow) FCEvent(eGoToStateClock));
	event_info.push_back(new(std::nothrow) FCEvent(eGoToStateManual));
	event_info.push_back(new(std::nothrow) FCEvent(eQuit));
	event_info.push_back(new(std::nothrow) FCEvent(eChangeState));
	event_info.push_back(new(std::nothrow) FCEvent(eManMode));
	event_info.push_back(new(std::nothrow) FCEvent(eRecordSamples));
	event_info.push_back(new(std::nothrow) FCEvent(eCopyBufferToAnalyzer));
	event_info.push_back(new(std::nothrow) FCEvent(eGetBPMValue));
	event_info.push_back(new(std::nothrow) FCEvent(eResetAnalyzer));
	event_info.push_back(new(std::nothrow) FCEvent(eCheckRMSValue));
	event_info.push_back(new(std::nothrow) FCEvent(eReadWavFile));
	event_info.push_back(new(std::nothrow) FCEvent(eStopRecording));
	
	//Create the timers
	event_info.push_back(new(std::nothrow) FCTimer(eTestTimer1, 5000000));
	event_info.push_back(new(std::nothrow) FCTimer(eInitDone, INIT_TIME));
	event_info.push_back(new(std::nothrow) FCTimer(eSupervisionTimer, QUEUE_TIMEOUT));
	event_info.push_back(new(std::nothrow) FCTimer(eCaptureTimer, AUDIO_TIMEOUT));
	event_info.push_back(new(std::nothrow) FCTimer(eRMSHystTimer, RMS_HYSTERESIS));

	//Check if events have been created successfully
	for (int i = 0; i < event_info.size(); i++)
	{
		if (event_info[i] == nullptr)
		{
			success = eMemoryAllocationFailed;
			return success;
		}
	}

	//Initialize the events
	retval.push_back(event_info[eGoToStateBPM]->init_event(goto_state_BPM));
	retval.push_back(event_info[eGoToStateTimer]->init_event(goto_state_Timer));
	retval.push_back(event_info[eGoToStateClock]->init_event(goto_state_Clock));
	retval.push_back(event_info[eGoToStateManual]->init_event(goto_state_Manual));
	retval.push_back(event_info[eQuit]->init_event(goto_state_Quit));
	retval.push_back(event_info[eChangeState]->init_event(goto_state));
	retval.push_back(event_info[eManMode]->init_event(goto_state));	//Dummy event
	retval.push_back(event_info[eRecordSamples]->init_event(&BPMAudio::capture_samples, bpm_info.bpm_audio));
	retval.push_back(event_info[eCopyBufferToAnalyzer]->init_event(&BPMAnalyze::prepare_audio_data, bpm_info.bpm_analyze));
	retval.push_back(event_info[eGetBPMValue]->init_event(&BPMAnalyze::get_bpm_value, bpm_info.bpm_analyze, false));
	retval.push_back(event_info[eResetAnalyzer]->init_event(&BPMAnalyze::reset_state, bpm_info.bpm_analyze, false));
	retval.push_back(event_info[eCheckRMSValue]->init_event(&BPMAnalyze::check_rms_value, bpm_info.bpm_analyze));
	retval.push_back(event_info[eReadWavFile]->init_event(&BPMAudio::read_std_wav_file, bpm_info.bpm_audio));
	retval.push_back(event_info[eStopRecording]->init_event(&BPMAudio::stop_recording, bpm_info.bpm_audio));

	//Initialize the timers
	retval.push_back(event_info[eTestTimer1]->init_event(timer1_elapsed));
	retval.push_back(event_info[eInitDone]->init_event(init_done));
	retval.push_back(event_info[eSupervisionTimer]->init_event(supervision_queue));
	retval.push_back(event_info[eCaptureTimer]->init_event(supervision_capture));
	retval.push_back(event_info[eRMSHystTimer]->init_event(rms_hyst_callback));

	//Check if events have been successfully initialized
	for (int i = 0; i < retval.size(); i++)
	{
		if (retval[i] != eSuccess)
		{
			success = retval[i];
			break;
		}
	}

	//Timers must be periodically checked by the OS queue
	//therefore we must send them to the FCQueue. Since the vector is of FCEvent type
	//we must cast to FCTimer*
	appl_info.os_queue->config_timer(eTestTimer1, *(FCTimer*)(event_info[eTestTimer1]));
	appl_info.os_queue->config_timer(eInitDone,   *(FCTimer*)(event_info[eInitDone]));
	appl_info.os_queue->config_timer(eSupervisionTimer, *(FCTimer*)(event_info[eSupervisionTimer]));
	appl_info.os_queue->config_timer(eCaptureTimer, *(FCTimer*)(event_info[eCaptureTimer]));
	appl_info.os_queue->config_timer(eRMSHystTimer, *(FCTimer*)(event_info[eRMSHystTimer]));
	
	return success;
}

void exit_application()
{
	//The user entered exit code
	//Stop all the async threads and exit application
	eError exit_retval;
	my_console.WriteToSplitConsole("Exiting!", param_list.get<int>("split main"));
	SEND_EVENT(eQuit);
	
	//Wait for a short amount of time to display the quit string
	std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_WAIT));

	//Stop the operating system and the state machine
	appl_info.os_queue->queue_stop();

	//Retrieve exit return value
	exit_retval = appl_info.os_thread.get();
	my_console.WriteToSplitConsole("Return value of queue = " + std::to_string((int)exit_retval) + ".", param_list.get<int>("split main")); 
	exit_retval = appl_info.os_SIR.get();
	my_console.WriteToSplitConsole("Return value of SIR = " + std::to_string((int)exit_retval) + ".", param_list.get<int>("split main"));
	
	//Stop socket
	#ifndef _WIN32
		appl_info.os_socket->stop_listening();
		exit_retval = appl_info.socket_thread.get();
		my_console.WriteToSplitConsole("Return value of socket = " + std::to_string((int)exit_retval) + ".", param_list.get<int>("split main"));
	#endif

	//We stop the machine
	appl_info.os_machine->machine_stop();

	//Retrieve the exit return value
	exit_retval = appl_info.machine_thread.get();
	my_console.WriteToSplitConsole("Return value of state machine = " + std::to_string((int)exit_retval) + ".", param_list.get<int>("split main"));							
}

void control_statemachine()
{
	//Bool variable for while loop
	bool cont = true;
	
	//Declare prompt response
	std::string user_value = "";
	int prompt_value;
			
	//Check prompt response
	while(cont == true)
	{
		//Display command prompt
		user_value = get_user_input();
	
		//We take care of the user input
		prompt_value = handle_user_input(user_value);
			
		//Check return value
		switch (prompt_value)
		{
			case eKeyQuit:
				//The user entered exit code
				//Stop all the async threads and exit application
				cont = false;
				exit_application();
				break;
			case eKeyBPMCounter:
				//We go to state BPM
				my_console.WriteToSplitConsole("Going to state BPM...", param_list.get<int>("split main"));
				SEND_EVENT(eGoToStateBPM);
				break;
			case eKeyTimer:
				//We go to state Timer
				my_console.WriteToSplitConsole("Going to state Timer...", param_list.get<int>("split main"));
				SEND_EVENT(eGoToStateTimer);
				break;
			case eKeyClock:
				//We go to state Clock
				my_console.WriteToSplitConsole("Going to state Clock...", param_list.get<int>("split main"));
				SEND_EVENT(eGoToStateClock);
				break;
			case eKeyManual:
				//We go to state Manual Console
				my_console.WriteToSplitConsole("Going to state Manual Console...", param_list.get<int>("split main"));
				SEND_EVENT(eGoToStateManual);
				manual_mode();
				break;
			case eKeyTCP:
				//We go to state Manual TCP
				my_console.WriteToSplitConsole("Going to state Manual TCP...", param_list.get<int>("split main"));
				SEND_INT_EVENT(eChangeState, eStateTCP);
				break;
			default:
				//Unknown command - we just ignore it
				my_console.WriteToSplitConsole("Unknown command.", param_list.get<int>("split main"));
				break;
		}
	}
	
	//Wait for a short time
	std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_WAIT));
}

void simulate_keyb_quit()
{
	#ifndef _WIN32
		//Simulate keyboard input '0' and enter
		//No other possibility to cancel std::cin from remote  (in 'ReadFromSplitConsole')
		//Since std::cin is blocking until data from keyboard is available
		//So we do it by simulating a keyboard keypress
		if (param_list.get<bool>("debug main") == true)
			my_console.WriteToSplitConsole("Simulated quit command.", param_list.get<int>("split main"));

		xdo_t* x = xdo_new(NULL);
		xdo_enter_text_window(x, CURRENTWINDOW, "0", 100000);
		xdo_send_keysequence_window(x, CURRENTWINDOW, "Return", 100000);
		xdo_free(x);
	#endif
}

void software_interrupt_routine()
{
	//Important: code in here is not allowed to block the application
	//The get() method of the std::future class would block, therefore we
	//check the std::future status using the wrapper method 'is_ready'
	//Interrupt routine for periodically checking GPIO input pin,
	//and verifying that the "operating system" as well as the state machine
	//are still being executed - if not, something went wrong and we should stop the application
	
	//Check if state machine is still running - if this function returns true,
	//it means that std::async "thread" has finished unexpectedly
	if (is_ready(appl_info.machine_thread) == true)
	{
		//The state machine thread has finished with an error - get error code
		eError error_code = appl_info.machine_thread.get();
		my_console.WriteToSplitConsole("Error in state machine - detected in SIR...", param_list.get<int>("split errors"));
		my_console.WriteToSplitConsole("The error code was " + std::to_string((int)error_code) + ".", param_list.get<int>("split errors"));
		exit(error_code);
	}

	//Check if queue is still running - if this function returns true,
	//it means that std::async "thread" has finished unexpectedly
	if (is_ready(appl_info.os_thread) == true)
	{
		//The queue thread has finished with an error - get error code
		eError error_code = appl_info.os_thread.get();
		my_console.WriteToSplitConsole("Error in queue - detected in SIR...", param_list.get<int>("split errors"));
		my_console.WriteToSplitConsole("The error code was " + std::to_string((int)error_code) + ".", param_list.get<int>("split errors"));
		exit(error_code);
	}

	//Socket can't be checked as it conatins blocking functions
	//This means, that calling get() on its future object will also cause block here and
	//therefore make our main routine synchronous.

	#ifndef _WIN32
		//Check reset pin status - periodically call get_state
		eButtonHandlerState state = gpio_info.gpio_button->get_state();

		//Execute commands only once
		static bool restart_issued = false;
		static bool reboot_issued = false;
		static bool shutdown_issued = false;
		switch (state)
		{
			case Handler_ButtonReleased:
				//Do nothing
				break;

			case Handler_ButtonPressed:
				//Restart the program
				if (restart_issued == false)
				{
					simulate_keyb_quit();
					system("./bpm_restart.sh &"); //The ampersand is important here, making the execution async!
					restart_issued = true;
				}	
				break;

			case Handler_ButtonHeldShort:
				//Reboot the PI
				if (reboot_issued == false)
				{
					simulate_keyb_quit();
					system("./bpm_reboot.sh &"); //The ampersand is important here, making the execution async!
					reboot_issued = true;
				}
				break;

			case Handler_ButtonHeldLong:
				//Shutdown the PI
				if (shutdown_issued == false)
				{
					simulate_keyb_quit();
					system("./bpm_shutdown.sh &"); //The ampersand is important here, making the execution async!
					shutdown_issued = true;
				}
				break;

			default:
				//Here, we don't do anything
				break;
		}

		//Check if there is some command pending from the socket
		std::string command;
		appl_info.os_socket->get_buffer(command); // No second argument, clear data
		if (command == "quit")
		{
			//Stop the threads
			simulate_keyb_quit();
		}
		if (command.substr(0, 6) == "state ")
		{
			if (command.substr(7, 3) == "BPM")
				SEND_EVENT(eGoToStateBPM);
		}
	#endif
}
	
int main()
{
	//Begin of main routine
	if (param_list.get<bool>("debug main") == true)
	{
    		my_console.WriteToSplitConsole("Starting main...", param_list.get<int>("split main"));
		std::string s = "Path: ";
		s.append(FILE_PATH);
		my_console.WriteToSplitConsole(s, param_list.get<int>("split main"));
	}

	//Declare return value
	eError retval = eSuccess;

	//Create application information
	retval = init_appl();
	if (retval != eSuccess)
	{
		my_console.WriteToSplitConsole("Error during application init.", param_list.get<int>("split errors"));
		my_console.WriteToSplitConsole("Errorcode: " + std::to_string(retval), param_list.get<int>("split errors"));
		exit(retval);
	}

	//Create GPIO information
	retval = init_gpio();
	if (retval != eSuccess)
	{
		my_console.WriteToSplitConsole("Error during GPIO init.", param_list.get<int>("split errors"));
		my_console.WriteToSplitConsole("Errorcode: " + std::to_string(retval), param_list.get<int>("split errors"));
		exit(retval);
	}

	//Create BPM information
	retval = init_bpm();
	if (retval != eSuccess)
	{
		my_console.WriteToSplitConsole("Error during BPM init.", param_list.get<int>("split errors"));
		my_console.WriteToSplitConsole("Errorcode: " + std::to_string(retval), param_list.get<int>("split errors"));
		exit(retval);
	}

	//Create event information
	retval = init_events();
	if (retval != eSuccess)
	{
		my_console.WriteToSplitConsole("Error during event/timer init.", param_list.get<int>("split errors"));
		my_console.WriteToSplitConsole("Errorcode: " + std::to_string(retval), param_list.get<int>("split errors"));
		exit(retval);
	}
	
	//Bind functions for state machine
	//Every state consists of one entry function (executed once upon entry), one loop
	//function which is executed periodically by the state machine's main-loop and
	//one exit function (executed once upon exit and before entering next state)
	//Binding must be done for all states and all functions
	appl_info.os_machine->bind_function(StateInit_entry, 	eStateInit);
	appl_info.os_machine->bind_function(StateInit_loop,  	eStateInit);
	appl_info.os_machine->bind_function(StateX_exit,  	eStateInit);
	appl_info.os_machine->bind_function(StateBPM_entry, 	eStateBPMCounter);
	appl_info.os_machine->bind_function(StateBPM_loop,  	eStateBPMCounter);
	appl_info.os_machine->bind_function(StateBPM_exit,  	eStateBPMCounter);
	appl_info.os_machine->bind_function(StateTimer_entry, 	eStateTimer);
	appl_info.os_machine->bind_function(StateTimer_loop,  	eStateTimer);
	appl_info.os_machine->bind_function(StateX_exit,  	eStateTimer);
	appl_info.os_machine->bind_function(StateClock_entry, 	eStateClock);
	appl_info.os_machine->bind_function(StateClock_loop,  	eStateClock);
	appl_info.os_machine->bind_function(StateX_exit,  	eStateClock);
	appl_info.os_machine->bind_function(StateManual_entry, 	eStateManual);
	appl_info.os_machine->bind_function(StateManual_loop,  	eStateManual);
	appl_info.os_machine->bind_function(StateX_exit,  	eStateManual);
	appl_info.os_machine->bind_function(StateQuit_entry, 	eStateQuit);
	appl_info.os_machine->bind_function(StateQuit_loop,  	eStateQuit);
	appl_info.os_machine->bind_function(StateX_exit,  	eStateQuit);
	appl_info.os_machine->bind_function(StateTCP_entry, 	eStateTCP);
	appl_info.os_machine->bind_function(StateTCP_loop, 	eStateTCP);
	appl_info.os_machine->bind_function(StateX_exit,  	eStateTCP);

	//Write debug line
	if (param_list.get<bool>("debug main") == true)
	{
		my_console.WriteToSplitConsole("Binding done.", param_list.get<int>("split main"));
		my_console.WriteToSplitConsole("Starting state machine...", param_list.get<int>("split main"));
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	}
		
	//Start the state machine
	appl_info.machine_thread = appl_info.os_machine->start_machine();

	if (param_list.get<bool>("debug main") == true)
	{
		my_console.WriteToSplitConsole("State machine running.", param_list.get<int>("split main"));
		my_console.WriteToSplitConsole("Binding software interrupt routine...", param_list.get<int>("split main"));
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	}
		
	//Bind software interrupt routine to queue
	appl_info.os_queue->set_SIR(software_interrupt_routine);	
	
	if (param_list.get<bool>("debug main") == true)
	{
		my_console.WriteToSplitConsole("SIR bound to FC queue.", param_list.get<int>("split main"));
		my_console.WriteToSplitConsole("Starting queue...", param_list.get<int>("split main"));
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	}

	//Create threads for operating system queue and SIR
	appl_info.os_thread = appl_info.os_queue->start_queue();
	appl_info.os_SIR = appl_info.os_queue->start_SIR();

	if (param_list.get<bool>("debug main") == true)
	{
		my_console.WriteToSplitConsole("Operating system queue running.", param_list.get<int>("split main"));
		#ifndef _WIN32
			my_console.WriteToSplitConsole("Starting TCP listener...", param_list.get<int>("split main"));
		#endif	
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	}

	#ifndef _WIN32
		//Create thread for TCP socket
		appl_info.socket_thread = appl_info.os_socket->start_listening();

		if (param_list.get<bool>("debug main") == true)
		{
			my_console.WriteToSplitConsole("Socket listener running.", param_list.get<int>("split main"));
			my_console.WriteToSplitConsole("Entering prompt mode...", param_list.get<int>("split main"));
			std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
		}
	#else
		if (param_list.get<bool>("debug main") == true)
		{
			my_console.WriteToSplitConsole("Entering prompt mode...", param_list.get<int>("split main"));
			std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
		}
	#endif

	//Start prompt thread
	std::thread prompt_thread(control_statemachine);
	prompt_thread.join();	
	
	//We shut down the display
	//GPIO not used on WIN32
	#ifndef _WIN32
		gpio_info.gpio_writer->reset_display(0);
	#endif
	
	//We have to release resources - malloc -> free, new -> delete
	delete appl_info.os_queue;
	delete appl_info.os_machine;

	//GPIO and socket not used on WIN32
	#ifndef _WIN32
		delete gpio_info.gpio_writer;
		delete gpio_info.gpio_button;
		delete appl_info.os_socket;
	#endif	

	delete bpm_info.bpm_audio;
	delete bpm_info.bpm_analyze;

	//Clear vector contents
	std::vector<FCEvent*>::iterator iter;
	for (iter = event_info.begin(); iter < event_info.end(); ++iter)
		delete (*iter);
	event_info.clear();
		
	if (param_list.get<bool>("debug main") == true)
		my_console.WriteToSplitConsole("Main end.", param_list.get<int>("split main"));

    return 0;
}
