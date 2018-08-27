#include <iostream>
#include <fstream>
#include <string>
#include <vector>

//The following headers aren't available on WIN32
#ifndef _WIN32
	#include <wiringPi.h>
	#include "GPIOPin.hpp"
	#include "GPIOWriter.hpp"
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

eError init_appl()
{
	//Declare return value
	eError retval = eSuccess;

	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("Application init function.", SPLIT_MAIN);
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	#endif

	//Create queue
	appl_info.os_queue = new(std::nothrow) FCQueue;

	//Create new state machine
	appl_info.os_machine = new(std::nothrow) FCMachine;

	//Get start time point of application
	appl_info.start = std::chrono::high_resolution_clock::now();

	//Check if objects have been created successfully
	if ((appl_info.os_queue == nullptr) || (appl_info.os_machine == nullptr))
	{
		retval = eMemoryAllocationFailed;
	}

	return retval;
}

eError init_gpio()
{

	//Declare return value
	eError retval = eSuccess;

	#ifndef _WIN32

	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("GPIO init function.", SPLIT_MAIN);
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	#endif

	//Initialize GPIO
	int result = GPIOPin::init_gpio();

	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("Initialize wiringPi... return = " + std::to_string(result), SPLIT_MAIN);
	#endif

	if (result != 0)
	{
		retval = eWiringPiNotInitialized;
	}

	//Create GPIO writer
	gpio_info.gpio_writer = new(std::nothrow) GPIOWriter();

	//Create GPIO listener
	gpio_info.gpio_listener = new(std::nothrow) GPIOPin(RESET_PIN_1, eIN, false);

	//Check if objects have been created successfully
	if ((gpio_info.gpio_writer == nullptr) || (gpio_info.gpio_listener == nullptr))
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
	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("BPM init function.", SPLIT_MAIN);
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	#endif

	//Create BPM Audio class instance
	bpm_info.bpm_audio = new(std::nothrow) BPMAudio();

	//Create BPM Analyzer class instance
	bpm_info.bpm_analyze = new(std::nothrow) BPMAnalyze();

	//Create buffers
	bpm_info.bpm_buffer = new(std::nothrow) BPMBuffer<short>(PCM_BUF_SIZE);

	//Check if objects have been created successfully
	if ((bpm_info.bpm_audio == nullptr) || (bpm_info.bpm_analyze == nullptr)
	    || (bpm_info.bpm_buffer->check_init() != eSuccess))
	{
		retval = eMemoryAllocationFailed;
	}

	return retval;
}

eError init_events()
{
	//We create and initialize all the events and timers used
	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("Event/timer init function.", SPLIT_MAIN);
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	#endif

	//Declare return value
	std::vector<eError> retval;
	eError success = eSuccess;

	//Create the events
	event_info.push_back(new(std::nothrow) FCEvent(eGoToState1));
	event_info.push_back(new(std::nothrow) FCEvent(eGoToState2));
	event_info.push_back(new(std::nothrow) FCEvent(eGoToState3));
	event_info.push_back(new(std::nothrow) FCEvent(eGoToState4));
	event_info.push_back(new(std::nothrow) FCEvent(eQuit));
	event_info.push_back(new(std::nothrow) FCEvent(eChange));
	event_info.push_back(new(std::nothrow) FCEvent(eManMode));
	event_info.push_back(new(std::nothrow) FCEvent(eRecordSamples));
	event_info.push_back(new(std::nothrow) FCEvent(eCopyAudioToBuffer));
	event_info.push_back(new(std::nothrow) FCEvent(eReadBufferReady));
	event_info.push_back(new(std::nothrow) FCEvent(eCopyBufferToAnalyzer));
	event_info.push_back(new(std::nothrow) FCEvent(eApplyFilter));
	event_info.push_back(new(std::nothrow) FCEvent(eMaximizeVolume));
	event_info.push_back(new(std::nothrow) FCEvent(eGetBPMValue));

	//Create the timers
	event_info.push_back(new(std::nothrow) FCTimer(eTestTimer1, 5000000));
	event_info.push_back(new(std::nothrow) FCTimer(eInitDone, 5000000));
	event_info.push_back(new(std::nothrow) FCTimer(eSupervisionTimer, 2000000));

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
	retval.push_back(event_info[eGoToState1]->init_event(goto_state_1));
	retval.push_back(event_info[eGoToState2]->init_event(goto_state_2));
	retval.push_back(event_info[eGoToState3]->init_event(goto_state_3));
	retval.push_back(event_info[eGoToState4]->init_event(goto_state_4));
	retval.push_back(event_info[eQuit]->init_event(goto_state_5));
	retval.push_back(event_info[eChange]->init_event(goto_state));
	retval.push_back(event_info[eManMode]->init_event(goto_state));	//Dummy event
	retval.push_back(event_info[eRecordSamples]->init_event(&BPMAudio::capture_samples, bpm_info.bpm_audio));
	retval.push_back(event_info[eCopyAudioToBuffer]->init_event(&BPMBuffer<short>::put_data, bpm_info.bpm_buffer, false));
	retval.push_back(event_info[eReadBufferReady]->init_event(&BPMAudio::is_ready, bpm_info.bpm_audio));
	retval.push_back(event_info[eCopyBufferToAnalyzer]->init_event(&BPMAnalyze::prepare_audio_data, bpm_info.bpm_analyze));
	retval.push_back(event_info[eApplyFilter]->init_event(&BPMAnalyze::lowpass_filter, bpm_info.bpm_analyze));
	retval.push_back(event_info[eMaximizeVolume]->init_event(&BPMAnalyze::maximize_volume, bpm_info.bpm_analyze));
	retval.push_back(event_info[eGetBPMValue]->init_event(&BPMAnalyze::extract_bpm_value, bpm_info.bpm_analyze, false));

	//Initialize the timers
	retval.push_back(event_info[eTestTimer1]->init_event(timer1_elapsed));
	retval.push_back(event_info[eInitDone]->init_event(init_done));
	retval.push_back(event_info[eSupervisionTimer]->init_event(supervision_queue));

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

	return success;
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
			case 0:
			{
				//The user entered exit code
				//Stop all the async threads and exit application
				cont = false;
				eError exit_retval;
				my_console.WriteToSplitConsole("Exiting!", SPLIT_MAIN);
				SEND_EVENT(eQuit);

				//Wait for a short amount of time to display the quit string
				std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_WAIT));

				//Stop the operating system and the state machine
				appl_info.os_queue->queue_stop();

				//Retrieve exit return value
				exit_retval = appl_info.os_thread.get();
				my_console.WriteToSplitConsole("Return value of queue = " + std::to_string((int)exit_retval) + ".", SPLIT_MAIN); 

				//We stop the machine
				appl_info.os_machine->machine_stop();

				//Retrieve the exit return value
				exit_retval = appl_info.machine_thread.get();
				my_console.WriteToSplitConsole("Return value of state machine = " + std::to_string((int)exit_retval) + ".", SPLIT_MAIN);
				break;
			}
			case 1:
				//We go to state 1
				my_console.WriteToSplitConsole("Going to state 1...", SPLIT_MAIN);
				SEND_INT_EVENT(eChange, 1);
				break;
			case 2:
				//We go to state 2
				my_console.WriteToSplitConsole("Going to state 2...", SPLIT_MAIN);
				SEND_EVENT(eGoToState2);
				break;
			case 3:
				//We go to state 3
				my_console.WriteToSplitConsole("Going to state 3...", SPLIT_MAIN);
				SEND_EVENT(eGoToState3);
				break;
			case 4:
				//We go to state 4
				my_console.WriteToSplitConsole("Going to state 4...", SPLIT_MAIN);
				SEND_EVENT(eGoToState4);
				manual_mode();
				break;
			default:
				//Unknown command - we just ignore it
				my_console.WriteToSplitConsole("Unknown command.", SPLIT_MAIN);
				break;
		}
	}
	
	//Wait for a short time
	std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_WAIT));
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
		my_console.WriteToSplitConsole("Error in state machine - detected in SIR...", SPLIT_ERRORS);
		my_console.WriteToSplitConsole("The error code was " + std::to_string((int)error_code) + ".", SPLIT_ERRORS);
		exit(error_code);
	}

	//Check if queue is still running - if this function returns true,
	//it means that std::async "thread" has finished unexpectedly
	if (is_ready(appl_info.os_thread) == true)
	{
		//The queue thread has finished with an error - get error code
		eError error_code = appl_info.os_thread.get();
		my_console.WriteToSplitConsole("Error in queue - detected in SIR...", SPLIT_ERRORS);
		my_console.WriteToSplitConsole("The error code was " + std::to_string((int)error_code) + ".", SPLIT_ERRORS);
		exit(error_code);
	}
}
	
int main()
{
	//Testcode for sound recording
	//init_appl();
	//init_bpm();

	///TESTCODE FOR AUDIO SAMPLE CAPTURE AND WAV-FILE CREATION
	/*
	eError retval = bpm_info.bpm_audio->capture_samples();
	if (retval == eSuccess)
		my_console.WriteToSplitConsole("Successfully captured audio data!", SPLIT_MAIN);
	else
	{
		my_console.WriteToSplitConsole("Error capturing audio data!", SPLIT_MAIN);
		exit(retval);
	}
	short* testbuffer = bpm_info.bpm_audio->flush_buffer();
	std::ofstream outfile("record.wav", std::ios_base::out | std::ios_base::binary);
	bpm_info.bpm_audio->write_wav_file(outfile, testbuffer);
	outfile.close();
	*/

	///TESTCODE FOR AUDIO FILE READ AND ANALYSIS
	/*
	//Read wav file
	std::ifstream readfile("fast160.wav", std::ios_base::in | std::ios_base::binary);
	WAVFile wavfile;
	bpm_info.bpm_audio->read_wav_file(readfile, wavfile);
	readfile.close();
	my_console.WriteToSplitConsole("the last byte is ..." + std::to_string(wavfile.buffer[PCM_BUF_SIZE - 1]), SPLIT_MAIN);	
	
	//Instance for analyzer
	BPMAnalyze analyze_info;
	analyze_info.prepare_audio_data(wavfile.buffer);
	analyze_info.lowpass_filter(60, 44100);
	analyze_info.maximize_volume();
	double a = analyze_info.extract_bpm_value();
	my_console.WriteToSplitConsole("The BPM value is : " + std::to_string(a), SPLIT_MAIN);	
	exit(0);
	*/


	//Begin of main routine
	#ifdef DEBUG_MAIN
    	my_console.WriteToSplitConsole("Starting main...", SPLIT_MAIN);
		std::string s = "Path: ";
		s.append(FILE_PATH);
		my_console.WriteToSplitConsole(s, SPLIT_MAIN);
	#endif

	//Declare return value
	eError retval = eSuccess;

	//Create application information
	retval = init_appl();
	if (retval != eSuccess)
	{
		my_console.WriteToSplitConsole("Error during application init.", SPLIT_ERRORS);
		my_console.WriteToSplitConsole("Errorcode: " + std::to_string(retval), SPLIT_ERRORS);
		exit(retval);
	}

	//Create GPIO information
	retval = init_gpio();
	if (retval != eSuccess)
	{
		my_console.WriteToSplitConsole("Error during GPIO init.", SPLIT_ERRORS);
		my_console.WriteToSplitConsole("Errorcode: " + std::to_string(retval), SPLIT_ERRORS);
		exit(retval);
	}

	//Create BPM information
	retval = init_bpm();
	if (retval != eSuccess)
	{
		my_console.WriteToSplitConsole("Error during BPM init.", SPLIT_ERRORS);
		my_console.WriteToSplitConsole("Errorcode: " + std::to_string(retval), SPLIT_ERRORS);
		exit(retval);
	}

	//Create event information
	retval = init_events();
	if (retval != eSuccess)
	{
		my_console.WriteToSplitConsole("Error during event/timer init.", SPLIT_ERRORS);
		my_console.WriteToSplitConsole("Errorcode: " + std::to_string(retval), SPLIT_ERRORS);
		exit(retval);
	}
	
	//Bind functions for state machine
	//Every state consists of one entry function (executed once upon entry), one loop
	//function which is executed periodically by the state machine's main-loop and
	//one exit function (executed once upon exit and before entering next state)
	//Binding must be done for all states and all functions
	appl_info.os_machine->bind_function(State0_entry, eStateInit);
	appl_info.os_machine->bind_function(State0_loop,  eStateInit);
	appl_info.os_machine->bind_function(StateX_exit,  eStateInit);
	appl_info.os_machine->bind_function(State1_entry, eStateBPMCounter);
	appl_info.os_machine->bind_function(State1_loop,  eStateBPMCounter);
	appl_info.os_machine->bind_function(State1_exit,  eStateBPMCounter);
	appl_info.os_machine->bind_function(State2_entry, eStateTimer);
	appl_info.os_machine->bind_function(State2_loop,  eStateTimer);
	appl_info.os_machine->bind_function(StateX_exit,  eStateTimer);
	appl_info.os_machine->bind_function(State3_entry, eStateClock);
	appl_info.os_machine->bind_function(State3_loop,  eStateClock);
	appl_info.os_machine->bind_function(StateX_exit,  eStateClock);
	appl_info.os_machine->bind_function(State4_entry, eStateManual);
	appl_info.os_machine->bind_function(State4_loop,  eStateManual);
	appl_info.os_machine->bind_function(StateX_exit,  eStateManual);
	appl_info.os_machine->bind_function(State5_entry, eStateQuit);
	appl_info.os_machine->bind_function(State5_loop,  eStateQuit);
	appl_info.os_machine->bind_function(StateX_exit,  eStateQuit);

	//Write debug line
	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("Binding done.", SPLIT_MAIN);
		my_console.WriteToSplitConsole("Starting state machine...", SPLIT_MAIN);
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	#endif
		
	//Start the state machine
	appl_info.machine_thread = appl_info.os_machine->start_machine();

	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("State machine running.", SPLIT_MAIN);
		my_console.WriteToSplitConsole("Binding software interrupt routine...", SPLIT_MAIN);
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	#endif
		
	//Bind software interrupt routine to queue
	appl_info.os_queue->set_SIR(software_interrupt_routine);	
	
	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("SIR bound to FC queue.", SPLIT_MAIN);
		my_console.WriteToSplitConsole("Starting queue...", SPLIT_MAIN);
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
	#endif

	//Create thread for operating system queue
	appl_info.os_thread = appl_info.os_queue->start_queue();

	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("Operating system queue running.", SPLIT_MAIN);
		my_console.WriteToSplitConsole("Entering prompt mode...", SPLIT_MAIN);
		std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
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

	//GPIO not used on WIN32
	#ifndef _WIN32
		delete gpio_info.gpio_writer;
		delete gpio_info.gpio_listener;
	#endif	

	delete bpm_info.bpm_audio;
	delete bpm_info.bpm_analyze;

	event_info.clear();
		
	#ifdef DEBUG_MAIN
		my_console.WriteToSplitConsole("Main end.", SPLIT_MAIN);
	#endif

    return 0;
}
