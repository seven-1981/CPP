#ifndef _FCFUNCTIONS_H
#define _FCFUNCTIONS_H

#include "bpm_globals.hpp"
#include "bpm.hpp"
#include "bpm_value.hpp"
#include "FCState.hpp"
#include "FCMachine.hpp"
#include "FCQueue.hpp"
#include "zeit.hpp"
#include "SplitConsole.hpp"
#include <iostream>
#include <ctime>
#include <string>
#include <chrono>
#include <fstream>

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

//Definition of state machine functions
//Note: since we are using a split console that uses mutex for
//handling critical sections, do not use the console in the loop functions
//The mutex may block and interrupt the loop - use only in entry or exit functions
//Split console may only be used in loop functions, if theres some mechanism to prevent flooding/blocking
//Also use the queued events for triggering functions in here, no function calls that
//block or slow down code execution!

//********************
//TRANSITION FUNCTIONS
//********************
void goto_state(int state)
{
	//Set transition
	appl_info.os_machine->set_trans(state);
}

void goto_state_BPM()
{
	//Set transition
	appl_info.os_machine->set_trans(eStateBPMCounter);
}

void goto_state_Timer()
{
	//Set transition
	appl_info.os_machine->set_trans(eStateTimer);
}

void goto_state_Clock()
{
	//Set transition
	appl_info.os_machine->set_trans(eStateClock);
}

void goto_state_Manual()
{
	//Set transition
	appl_info.os_machine->set_trans(eStateManual);
}

void goto_state_Quit()
{
	//Set transition
	appl_info.os_machine->set_trans(eStateQuit);
}

//*********************************
//STATE INIT - INIT STATE - STARTUP
//*********************************
void init_done()
{
	//The initial waiting time has elapsed - continue to BPM counter
	appl_info.os_machine->set_trans(eStateBPMCounter);
}

void StateInit_entry()
{
	if (param_list.get<bool>("debug functions") == true)
		my_console.WriteToSplitConsole("Init state.", param_list.get<int>("split fc"));

	//Start timer - if it elapses, we continue to next state (BPM)
	START_TIMER(eInitDone);

	CURRENT_STATE;
}

FCStates StateInit_loop()
{
	//Init state - we display some info
	std::string s = "init";
	const char* c = s.c_str();

	//GPIO not used on WIN32
	#ifndef _WIN32
		gpio_info.gpio_writer->print_string(c, 4, 4);
	#endif

	LOOP_OR_EXIT;
}

//***********************************************
//STATE BPM - DEFAULT STARTUP STATE - BPM COUNTER
//***********************************************

bool toggle_old = false;
eBPMCaptureState eCaptureState = eBPM_CaptureStartAudio;
eBPMAnalyzeState eAnalyzeState = eBPM_AnalyzeReady;
eBPMCaptureState eCaptureStateOld = eBPM_CaptureStartAudio;
eBPMAnalyzeState eAnalyzeStateOld = eBPM_AnalyzeReady;
BPMValue value_handler;
bool capture_timeout = false;
bool rms_value_ok = false;
long long ms_old1 = 0;
int pos1 = 0;
unsigned int number = 0;
bool rms_hyst_ok = false;
bool rms_timer_started = false;

void supervision_queue()
{
	//Display message
	my_console.WriteToSplitConsole("Timeout supervision of OS queue!", param_list.get<int>("split errors"));
}

void supervision_capture()
{
	capture_timeout = true;
	my_console.WriteToSplitConsole("Timeout during audio capture!", param_list.get<int>("split errors"));
}

void rms_hyst_callback()
{
	rms_hyst_ok = true;
	my_console.WriteToSplitConsole("RMS hysteresis elapsed!", param_list.get<int>("split errors"));
}

void StateBPM_entry()
{
	if (param_list.get<bool>("debug functions") == true)
		my_console.WriteToSplitConsole("State_entry. BPM counter.", param_list.get<int>("split fc"));

	//Activate timer interrupt
	//Not possible yet. Interrupt is blocking rest of process
	//because it is executed far too frequently
	//gpio_info.gpio_writer->activate_timer_interrupt();
	
	//Reset state machine
	eCaptureState = eBPM_CaptureStartAudio;
	eAnalyzeState = eBPM_AnalyzeReady;
	eCaptureStateOld = eCaptureState;
	eAnalyzeStateOld = eAnalyzeState;
	//Reset analyzer
	SEND_EVENT(eResetAnalyzer);

	START_TIMER(eSupervisionTimer);

	CURRENT_STATE;
}

FCStates StateBPM_loop()
{
	//Check queue toggle bit
	if (appl_info.os_queue->toggle != toggle_old)
	{
		//Toggle bit has changed, reset timer
		RESET_TIMER(eSupervisionTimer);
		START_TIMER(eSupervisionTimer);
		toggle_old = appl_info.os_queue->toggle;
	}
	
	//Audio capture state machine
	short* temp;
	int record_status;
	int stop_status;
	switch(eCaptureState)
	{
		case eBPM_CaptureStartAudio:
			//Start capturing
			#ifndef _WIN32
				RESET_TIMER(eCaptureTimer);
				START_TIMER(eCaptureTimer);
				SEND_EVENT(eRecordSamples);
				//Set next state
				eCaptureState = eBPM_CaptureAudio;
			#else
				SEND_EVENT(eReadWavFile);
				temp = GET_AUDIO_BUFFER;
				//Set next state
				eCaptureState = eBPM_CaptureReadWav;
			#endif
			break;
		
		case eBPM_CaptureAudio:
			//Check if samples have been captured
			record_status = GET_INT_DATA(eRecordSamples);
			if (record_status == eSuccess)
			{
				eCaptureState = eBPM_CaptureStartHandover;
				STOP_TIMER(eCaptureTimer);
			}
			else if (record_status == eAudio_ErrorCapturingAudio)
			{
				eCaptureState = eBPM_CaptureError;
			}
			else if (capture_timeout == true)
			{
				eCaptureState = eBPM_CaptureError;
				capture_timeout = false;
			}
			break;

		case eBPM_CaptureReadWav:
			//Check if wavfile has been read
			if (GET_INT_DATA(eReadWavFile) == eSuccess)
				eCaptureState = eBPM_CaptureStartHandover;
			break;

		case eBPM_CaptureStartHandover:
			//Handover data to analyzer
			if (bpm_info.bpm_analyze->get_state() == eReadyForData)
			{
				SEND_PSHORT_EVENT(eCopyBufferToAnalyzer, GET_AUDIO_BUFFER);
				eCaptureState =eBPM_CaptureHandover;
			}
			break;

		case eBPM_CaptureHandover:
			//Check if handover has been finished. If yes, we can capture again.
			if (bpm_info.bpm_analyze->get_state() == eDataCopyFinished
	    		 && GET_INT_DATA(eCopyBufferToAnalyzer) == eSuccess)
				eCaptureState = eBPM_CaptureStartAnalysis;
			break;

		case eBPM_CaptureStartAnalysis:
			//This state is used to trigger the ANALYSIS state machine
			//Just start from the beginning
			eCaptureState = eBPM_CaptureStartAudio;
			break;

		case eBPM_CaptureError:
			//This is not good, a problem during sound recording occured
			//We detect this by timeout of recording timer
			SEND_EVENT(eStopRecording);
			eCaptureState = eBPM_CaptureErrorWait;
			break;

		case eBPM_CaptureErrorWait:
			//Check if stopping was successful
			stop_status = GET_INT_DATA(eStopRecording);
			if (stop_status == eSuccess)
				eCaptureState = eBPM_CaptureStartAudio;
			break;

		default:
			//Here, we don't do anything
			break;
	}

	//Analysis state machine
	eError result_rms;
	switch(eAnalyzeState)
	{
		case eBPM_AnalyzeReady:
			//Check if CAPTURE state machine is in correct state. 
			//If yes, we can start the analysis procedure.
			if (eCaptureState == eBPM_CaptureStartAnalysis)
			{
				SEND_EVENT(eCheckRMSValue);
				eAnalyzeState = eBPM_AnalyzeRMS;
			}
			break;

		case eBPM_AnalyzeRMS:
			//Check if rms value is above threshold
			result_rms = (eError)GET_INT_DATA(eCheckRMSValue);
			if (result_rms == eSuccess)
			{
				//RMS value is ok, continue to next state
				eAnalyzeState = eBPM_AnalyzeStartBPM;
				rms_value_ok = true;
			}
			else if (result_rms == eAnalyzer_RMSBelowThreshold || 
			         result_rms == eAnalyzer_NotReadyYet)
			{
				//RMS value is too low or analyzer not ready, reset state machine
				eAnalyzeState = eBPM_AnalyzeReady;
				rms_value_ok = false;
				//Reset analyzer
				SEND_EVENT(eResetAnalyzer);
			}
			break;
		
		case eBPM_AnalyzeStartBPM:
			//Start BPM analysis
			SEND_EVENT(eGetBPMValue);
			eAnalyzeState = eBPM_AnalyzeBPM;
			break;

		case eBPM_AnalyzeBPM:
			//Get returned bpm value after calculation has finished
			if (bpm_info.bpm_analyze->get_state() == eReadyForData)
				eAnalyzeState = eBPM_AnalyzeReady;
			break;
		
		default:
			//Here, we don't do anything
			break;
	}

	//Log the capture and analyze state if one of it has changed
	if (param_list.get<bool>("debug bpm state") == true)
	{
		if ((eCaptureStateOld != eCaptureState) || (eAnalyzeStateOld != eAnalyzeState))
		{
			std::string s = "Capture State = " + std::to_string((int)eCaptureState);
			s += "; Analyze State = " + std::to_string((int)eAnalyzeState);
			my_console.WriteToSplitConsole(s, param_list.get<int>("split fc"));
		}
	}

	//Update old states
	eCaptureStateOld = eCaptureState;
	eAnalyzeStateOld = eAnalyzeState;

	//Read the bpm value from the register continuously
	double bpm = GET_DBL_DATA(eGetBPMValue);

	//Process value with bpm value handler
	static double bpm_old = 0.0;
	static double bpm_value = 0.0;
	if (bpm != bpm_old)
	{
		bpm_value = value_handler.process_value(bpm);
		bpm_old = bpm;
	}

	//Display the value using gpio writer
	#ifndef _WIN32
		if ((rms_value_ok == true) && (rms_timer_started == false))
		{
			START_TIMER(eRMSHystTimer);
			rms_timer_started = true;
		}
		if (rms_value_ok == false)
		{
			rms_timer_started = false;
			rms_hyst_ok = false;
			RESET_TIMER(eRMSHystTimer);
		}
		if (rms_hyst_ok == true)
			gpio_info.gpio_writer->print_number((float)bpm_value);
		else
		{		
			//Display according sentence
			int cycle = param_list.get<int>("man cycle");
			eError finished = gpio_info.gpio_writer->print_string(sentences[number], cycle);
			//Check if we have to generate a new random number
			if (finished == eSuccess)
				number = rand() % NUM_SENTENCES;
		}

		//gpio_info.gpio_writer->print_number((float)bpm_value);
		//Timer interrupt function, not used yet
		//gpio_info.gpio_writer->set_value(bpm_value);

		//Display value using window
		if (appl_info.hdmi_attached == true)
		{
			FCWindowData_t windata;
			windata.string_data = std::to_string(bpm_value).c_str();
			appl_info.window->update(windata);
		}
	#endif
	
	LOOP_OR_EXIT;
}

int StateBPM_exit()
{
	//Clear all used registers - clear flag set to true
	SEND_INT_DATA(eRecordSamples, 0, true);
	SEND_INT_DATA(eReadWavFile, 0, true);
	SEND_INT_DATA(eCopyBufferToAnalyzer, 0, true);
	SEND_INT_DATA(eCheckRMSValue, 0, true);

	//Stop the recording
	SEND_EVENT(eStopRecording);

	//Clear rms flag
	rms_value_ok = false;

	//Deactivate timer interrupt
	//Not used yet
	//gpio_info.gpio_writer->deactivate_timer_interrupt();

	//We shut down all pins
	//GPIO not used on WIN32
	#ifndef _WIN32
		gpio_info.gpio_writer->reset_display(0);
	#endif

	RESET_TIMER(eSupervisionTimer);
	RESET_TIMER(eCaptureTimer);
	RESET_TIMER(eRMSHystTimer);
	capture_timeout = false;
	rms_timer_started = false;
	rms_hyst_ok = false;

	if (param_list.get<bool>("debug functions") == true)
		my_console.WriteToSplitConsole("State BPM counter exit.", param_list.get<int>("split fc"));

	NEXT_STATE;
}

//***************************
//STATE TIMER - CLICK COUNTER
//***************************
long long ms_old2 = 0;

void StateTimer_entry()
{
	if (param_list.get<bool>("debug functions") == true)
		my_console.WriteToSplitConsole("State_entry. Click counter.", param_list.get<int>("split fc"));

	ms_old2 = 0;

	CURRENT_STATE;
}

FCStates StateTimer_loop()
{
	//Click counter
	//We measure the time since application start
	auto elapsed = std::chrono::high_resolution_clock::now() - appl_info.start;
	long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	
	//Display the count using GPIO writer
	//GPIO not used on WIN32
	#ifndef _WIN32
		gpio_info.gpio_writer->print_number((float)ms / 1000);
	#endif

	//Write debug line
	if (param_list.get<bool>("debug functions") == true)
	{
		if (ms - ms_old2 > 1000)
		{
			std::string s = "Timervalue : " + std::to_string((float)ms / 1000);
			my_console.WriteToSplitConsole(s, param_list.get<int>("split fc"));
			ms_old2 = ms;
		}
	}
	
	LOOP_OR_EXIT;
}

void timer1_elapsed()
{
	//Write some message
	my_console.WriteToSplitConsole("The timer has elapsed!", param_list.get<int>("split fc"));
}

//***************************
//STATE CLOCK - CLOCK DISPLAY
//***************************
int year=0;
int month=0;
int day=0;
int hour=0;
int minute=0;
int second=0;

int *a = &year;
int *b = &month;
int *c = &day;
int *d = &hour;
int *e = &minute;
int *f = &second;

void StateClock_entry()
{
	if (param_list.get<bool>("debug functions") == true)
		my_console.WriteToSplitConsole("State_entry. Clock.", param_list.get<int>("split fc"));

	//Start timer
	START_TIMER(eTestTimer1);

	CURRENT_STATE;
}

FCStates StateClock_loop()
{	
	//Get the actual time
	time_t now = time(NULL);

	//Convert the unix time format to time values
	UnixTimeToDateTime(now, a, b, c, d, e, f);

	//Convert the time values to string
	const char* t = std::to_string(hour*100 + minute).c_str();
	
	//Write time to the display
	//GPIO not used on WIN32
	#ifndef _WIN32
		gpio_info.gpio_writer->print_string(t, 4, 4);
	#endif
	
	LOOP_OR_EXIT;
}

//**************************
//STATE MANUAL - MANUAL MODE
//**************************
std::string my_string = "";
std::string str_old = "";

void StateManual_entry()
{
	if (param_list.get<bool>("debug functions") == true)
		my_console.WriteToSplitConsole("State_entry. Manual mode.", param_list.get<int>("split fc"));

	my_string = "";
	str_old = my_string;

	CURRENT_STATE;
}

FCStates StateManual_loop()
{
	//Read string to display from queue
	my_string = GET_STR_DATA(eManMode);

	//Retrieve lenght of string to display
	int len = my_string.length();

	//Check if it has changed
	if (str_old != my_string)
	{
		my_console.WriteToSplitConsole("Displaying: " + my_string, param_list.get<int>("split fc"));
		my_console.WriteToSplitConsole("Length: " + std::to_string(len), param_list.get<int>("split fc"));
		str_old = my_string;
	}
	
	//Display the string using GPIO writer
	//GPIO not used on WIN32
	#ifndef _WIN32
		int cycle = param_list.get<int>("man cycle");
		gpio_info.gpio_writer->print_string(my_string.c_str(), cycle);
	#endif

	LOOP_OR_EXIT;
}

//******************************************
//STATE QUIT - QUIT STATE - EXIT APPLICATION
//******************************************
void StateQuit_entry()
{
	if (param_list.get<bool>("debug functions") == true)
		my_console.WriteToSplitConsole("Quit state.", param_list.get<int>("split fc"));

	CURRENT_STATE;
}

FCStates StateQuit_loop()
{
	//Quit state - we display some info
	std::string s = "quit";
	const char* c = s.c_str();

	//GPIO not used on WIN32
	#ifndef _WIN32
		gpio_info.gpio_writer->print_string(c, 4, 4);
	#endif

	LOOP_OR_EXIT;
}

int StateX_exit()
{
	//We shut down all pins
	//GPIO not used on WIN32
	#ifndef _WIN32
		gpio_info.gpio_writer->reset_display(0);
	#endif

	if (param_list.get<bool>("debug functions") == true)
		my_console.WriteToSplitConsole("State exit.", param_list.get<int>("split fc"));

	NEXT_STATE;
}

//*************************
//STATE TCP - MANUAL TCP MODE
//*************************
std::string my_string_tcp = "";
std::string str_tcp_old = "";
bool quit_TCP = false;

void StateTCP_entry()
{
	if (param_list.get<bool>("debug functions") == true)
		my_console.WriteToSplitConsole("State_entry. TCP mode.", param_list.get<int>("split fc"));

	my_string_tcp = "";
	str_tcp_old = my_string_tcp;
	quit_TCP = false;

	CURRENT_STATE;
}

FCStates StateTCP_loop()
{
	//Read string to display from queue
	#ifndef _WIN32
		appl_info.os_socket->get_buffer(my_string_tcp);
	#else
		my_string_tcp = "No Socket on Win!";
	#endif

	//Check if '0' has been entered - quit command
	if ((my_string_tcp.compare(std::to_string(0)) == 0) && (quit_TCP == false))
	{
		//By default, we enter BPM state
		SEND_EVENT(eGoToStateBPM);
		quit_TCP = true;
	}

	if (quit_TCP == false)
	{
		//Check if it has changed
		if ((str_tcp_old != my_string_tcp) && (my_string_tcp != ""))
		{
			//Retrieve lenght of string to display
			int len = my_string_tcp.length();
			my_console.WriteToSplitConsole("Displaying: " + my_string_tcp, param_list.get<int>("split fc"));
			my_console.WriteToSplitConsole("Length: " + std::to_string(len), param_list.get<int>("split fc"));
			str_tcp_old = my_string_tcp;
		}
	
		//Display the string using GPIO writer
		//GPIO not used on WIN32
		#ifndef _WIN32
			int cycle = param_list.get<int>("man cycle");
			gpio_info.gpio_writer->print_string(str_tcp_old.c_str(), cycle);
		#endif
	}

	LOOP_OR_EXIT;
}

#endif
