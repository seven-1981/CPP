#ifndef _FCFUNCTIONS_H
#define _FCFUNCTIONS_H

#include "bpm_globals.hpp"
#include "bpm.hpp"
#include "FCState.hpp"
#include "FCMachine.hpp"
#include "FCQueue.hpp"
#include "zeit.hpp"
#include "SplitConsole.hpp"
#include <iostream>
#include <ctime>
#include <string>

//Extern split console instance
extern SplitConsole my_console;

//Definition of state machine functions
//Note: since we are using a split console that uses mutex for
//handling critical sections, do not use the console in the loop functions
//The mutex may block and interrupt the loop - use only in entry or exit functions
//Split console may only be used in loop functions, if theres some mechanism to prevent flooding/blocking

//********************
//TRANSITION FUNCTIONS
//********************
void goto_state(int state)
{
	//Set transition
	appl_info.os_machine->set_trans(state);
}

void goto_state_1()
{
	//Set transition
	appl_info.os_machine->set_trans(1);
}

void goto_state_2()
{
	//Set transition
	appl_info.os_machine->set_trans(2);
}

void goto_state_3()
{
	//Set transition
	appl_info.os_machine->set_trans(3);
}

void goto_state_4()
{
	//Set transition
	appl_info.os_machine->set_trans(4);
}

void goto_state_5()
{
	//Set transition
	appl_info.os_machine->set_trans(5);
}

//******************************
//STATE 0 - INIT STATE - STARTUP
//******************************
void init_done()
{
	//The initial waiting time has elapsed - continue to BPM counter
	appl_info.os_machine->set_trans(eStateBPMCounter);
}

void State0_entry()
{
	#ifdef DEBUG_FUNCTIONS
		my_console.WriteToSplitConsole("Init state.", SPLIT_FC);
	#endif

	//Start timer - if it elapses, we continue to next state (BPM)
	START_TIMER(eInitDone);

	CURRENT_STATE;
}

FCStates State0_loop()
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

//*********************************************
//STATE 1 - DEFAULT STARTUP STATE - BPM COUNTER
//*********************************************
bool samples_captured = false, copied_audio2buffer = false, copied_to_analyzer = false;
bool wav_file_read = false;
bool toggle_old = false;
int counter1 = 0, counter2 = 0, counter3 = 0;
double bpm_old = 0.0;

void supervision_queue()
{
	//Display message
	my_console.WriteToSplitConsole("Timeout supervision of OS queue!", SPLIT_ERRORS);
}

void State1_entry()
{
	#ifdef DEBUG_FUNCTIONS
		my_console.WriteToSplitConsole("State1_entry. BPM counter.", SPLIT_FC);
	#endif

	samples_captured = false;
	copied_audio2buffer = false;

	START_TIMER(eSupervisionTimer);

	CURRENT_STATE;
}

FCStates State1_loop()
{
	//BPM Counter state

	//Check queue toggle bit
	if (appl_info.os_queue->toggle != toggle_old)
	{
		//Toggle bit has changed, reset timer
		RESET_TIMER(eSupervisionTimer);
		START_TIMER(eSupervisionTimer);
		toggle_old = appl_info.os_queue->toggle;
	}	

	//1. Capture samples
	#ifndef _WIN32
		//1.a On Linux OS, we capture samples.
		if (samples_captured == false)
		{
			SEND_EVENT(eRecordSamples);
			my_console.WriteToSplitConsole("Capture audio data. Counter1 = " + std::to_string(counter1), SPLIT_FC);
			counter1++;
			samples_captured = true;
		}
	#else
		//1.b On WIN32, we can't capture samples, we just read the same wav file for analysis.
		if (wav_file_read == false)
		{
			my_console.WriteToSplitConsole("Reading wav file.", SPLIT_FC);
			std::ifstream readfile("fast160.wav", std::ios_base::in | std::ios_base::binary);
			bpm_info.bpm_audio->read_wav_file(readfile);
			readfile.close();
			wav_file_read = true;
		}
	#endif

	//2. As soon as samples have been captured, handover to buffer
	if ((GET_INT_DATA(eRecordSamples) == eSuccess || wav_file_read == true) && copied_audio2buffer == false)
	{
		SEND_PSHORT_EVENT(eCopyAudioToBuffer, GET_AUDIO_BUFFER);
		my_console.WriteToSplitConsole("Samples captured. Copy to buffer. Counter2 = " + std::to_string(counter2), SPLIT_FC);
		counter2++;
		copied_audio2buffer = true;
	}

	//3. As soon as flushing has been finished, check if buffer is full. If not, we can capture again.
	if (GET_INT_DATA(eCopyAudioToBuffer) == eSuccess && bpm_info.bpm_buffer->check_full() == eSuccess)
	{
		samples_captured = false;
		copied_audio2buffer = false;
		wav_file_read = false;
		SEND_INT_DATA(eCopyAudioToBuffer, 0, false);
		my_console.WriteToSplitConsole("Buffer not yet full. Capture again.", SPLIT_FC);
	}
	
	//4. If there is something in the buffer, copy it to analyzer, but only if analyzer is ready
	if (bpm_info.bpm_buffer->check_ready() == eSuccess && bpm_info.bpm_analyze->get_state() == eReadyForData && copied_to_analyzer == false)
	{
		//Note: GET_BUFFER flushes it -> this increases the read pointer
		short* temp_buf = GET_BUFFER;
		if (temp_buf != nullptr)
		{
			my_console.WriteToSplitConsole("Buffer not empty. Copying to analyzer. Counter3 = " + std::to_string(counter3), SPLIT_FC);
			SEND_PSHORT_EVENT(eCopyBufferToAnalyzer, temp_buf);
			counter3++;
			copied_to_analyzer = true;
		}
	}

	//5. After analyzer has received data, perform analysis
	if (GET_INT_DATA(eCopyBufferToAnalyzer) == eSuccess)
	{
		my_console.WriteToSplitConsole("Performing analysis.", SPLIT_FC);
		SEND_DOUBLE_EVENT(eApplyFilter, 60.0);
		SEND_EVENT(eMaximizeVolume);
		SEND_EVENT(eGetBPMValue);
		copied_to_analyzer = false;
	}

	//6. Get the bpm value from the register
	double bpm = GET_DBL_DATA(eGetBPMValue);
	if (bpm != bpm_old)
	{
		my_console.WriteToSplitConsole("The BPM value was " + std::to_string(bpm), SPLIT_ERRORS);
		bpm_old = bpm;
	}

	//7. Display the value using gpio writer
	#ifndef _WIN32
		gpio_info.gpio_writer->print_number((float)bpm);
	#endif

	LOOP_OR_EXIT;
}

int State1_exit()
{
	//We shut down all pins
	//GPIO not used on WIN32
	#ifndef _WIN32
		gpio_info.gpio_writer->reset_display(0);
	#endif

	RESET_TIMER(eSupervisionTimer);

	#ifdef DEBUG_FUNCTIONS
		my_console.WriteToSplitConsole("State BPM counter exit.", SPLIT_FC);
	#endif

	NEXT_STATE;
}

//***********************
//STATE 2 - CLICK COUNTER
//***********************
long long ms_old2 = 0;

void State2_entry()
{
	#ifdef DEBUG_FUNCTIONS
		my_console.WriteToSplitConsole("State2_entry. Click counter.", SPLIT_FC);
	#endif

	ms_old2 = 0;

	CURRENT_STATE;
}

FCStates State2_loop()
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
	#ifdef DEBUG_FUNCTIONS
		if (ms - ms_old2 > 1000)
		{
			std::string s = "Timervalue : " + std::to_string((float)ms / 1000);
			my_console.WriteToSplitConsole(s, SPLIT_FC);
			ms_old2 = ms;
		}
	#endif
	
	LOOP_OR_EXIT;
}

void timer1_elapsed()
{
	//Write some message
	my_console.WriteToSplitConsole("The timer has elapsed!", SPLIT_FC);
}

//***********************
//STATE 3 - CLOCK DISPLAY
//***********************
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

void State3_entry()
{
	#ifdef DEBUG_FUNCTIONS
		my_console.WriteToSplitConsole("State3_entry. Clock.", SPLIT_FC);
	#endif

	//Start timer
	START_TIMER(eTestTimer1);

	CURRENT_STATE;
}

FCStates State3_loop()
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

//*********************
//STATE 4 - MANUAL MODE
//*********************
std::string my_string = "";
std::string str_old = "";
long long ms_old4 = 0;
int pos4 = 0;

void State4_entry()
{
	#ifdef DEBUG_FUNCTIONS
		my_console.WriteToSplitConsole("State4_entry. Manual mode.", SPLIT_FC);
	#endif

	my_string = "";
	str_old = my_string;
	ms_old4 = 0;
	pos4 = 0;

	CURRENT_STATE;
}

FCStates State4_loop()
{
	//Read string to display from queue
	my_string = GET_STR_DATA(eManMode);

	//Retrieve lenght of string to display
	int len = my_string.length();

	//Check if it has changed
	if (str_old != my_string)
	{
		my_console.WriteToSplitConsole("Displaying: " + my_string, SPLIT_FC);
		my_console.WriteToSplitConsole("Length: " + std::to_string(len), SPLIT_FC);
		str_old = my_string;
		pos4 = 0;
	}

	//We measure the time since application start
	auto elapsed = std::chrono::high_resolution_clock::now() - appl_info.start;
	long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

		if (ms - ms_old4 > 500)
		{
			pos4++;
			if (pos4 > (len+4))
				pos4 = 0;
			ms_old4 = ms;
		}
	
	//Display the string using GPIO writer
	//GPIO not used on WIN32
	#ifndef _WIN32
		gpio_info.gpio_writer->print_string(my_string.c_str(), len, pos4);
	#endif

	LOOP_OR_EXIT;
}

//***************************************
//STATE 5 - QUIT STATE - EXIT APPLICATION
//***************************************
void State5_entry()
{
	#ifdef DEBUG_FUNCTIONS
		my_console.WriteToSplitConsole("Quit state.", SPLIT_FC);
	#endif

	CURRENT_STATE;
}

FCStates State5_loop()
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

	#ifdef DEBUG_FUNCTIONS
		my_console.WriteToSplitConsole("State exit.", SPLIT_FC);
	#endif

	NEXT_STATE;
}

#endif
