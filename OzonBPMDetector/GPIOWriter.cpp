//The GPIO implementation is not used on WIN32
#ifndef _WIN32

#include <iostream>
#include <chrono>
#include <math.h>
#include <wiringPi.h>
#include <unistd.h>
#include <signal.h>
#include "GPIOWriter.hpp"
#include "bpm_globals.hpp"
#include "bpm_param.hpp"
#include "SplitConsole.hpp"
#include "TIMINT.hpp"

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

GPIOWriter::GPIOWriter()
{
	//Execute init function - ignore return value
	//The result is stored in member variable is_initialized
	init_writer();
}

GPIOWriter::~GPIOWriter()
{
	//We set all pins to zero
	reset_display(0);
	
	//We delete all the instances
	delete(adr_1);
	delete(adr_2);
	delete(adr_3);
	delete(adr_4);
	delete(seg_1);
	delete(seg_2);
	delete(seg_3);
	delete(seg_4);
	delete(seg_5);
	delete(seg_6);
	delete(seg_7);
	delete(seg_8);
}

eError GPIOWriter::init_writer()
{
	//Write debug line
	if (param_list.get<bool>("debug writer") == true)
		std::cout << "GPIO Writer Class: Created writer " << std::endl;

	//Declare return value
	eError retval = eSuccess;

	//Address pins
	this->adr_1 = new(std::nothrow) GPIOPin(ADDRESS_PIN_1, eOUT, false);
	this->adr_2 = new(std::nothrow) GPIOPin(ADDRESS_PIN_2, eOUT, false);
	this->adr_3 = new(std::nothrow) GPIOPin(ADDRESS_PIN_3, eOUT, false);
	this->adr_4 = new(std::nothrow) GPIOPin(ADDRESS_PIN_4, eOUT, false);
	//Segment pins
	this->seg_1 = new(std::nothrow) GPIOPin(SEGMENT_PIN_1, eOUT, false);
	this->seg_2 = new(std::nothrow) GPIOPin(SEGMENT_PIN_2, eOUT, false);
	this->seg_3 = new(std::nothrow) GPIOPin(SEGMENT_PIN_3, eOUT, false);
	this->seg_4 = new(std::nothrow) GPIOPin(SEGMENT_PIN_4, eOUT, false);
	this->seg_5 = new(std::nothrow) GPIOPin(SEGMENT_PIN_5, eOUT, false);
	this->seg_6 = new(std::nothrow) GPIOPin(SEGMENT_PIN_6, eOUT, false);
	this->seg_7 = new(std::nothrow) GPIOPin(SEGMENT_PIN_7, eOUT, false);
	this->seg_8 = new(std::nothrow) GPIOPin(SEGMENT_PIN_8, eOUT, false);

	//Set configuration
	this->control_mode = 1;

	//Timing configuration
	SevenSegTiming config;
	config.hold_addr_seg = HOLD_ADDR_SEG;
	config.pause_addr_seg = PAUSE_ADDR_SEG;
	config.hold_seg_addr = HOLD_SEG_ADDR;
	config.pause_seg_addr = PAUSE_SEG_ADDR;
	this->timing_config = config;

	//Check for proper initialization
	if (   this->adr_1 == nullptr || this->adr_2 == nullptr || this->adr_3 == nullptr 
	    || this->adr_4 == nullptr || this->seg_1 == nullptr || this->seg_2 == nullptr
   	    || this->seg_3 == nullptr || this->seg_4 == nullptr || this->seg_5 == nullptr
	    || this->seg_6 == nullptr || this->seg_7 == nullptr || this->seg_8 == nullptr)
	{
		this->is_initialized = false;
		retval = eGPIOWriterNotInitialized;
	}
	else
		this->is_initialized = true;

	//Timer interrupt
	this->interrupt_activated = false;
	this->value = 0.0;
	this->sequence = 0;
	//Configure and start timer
	TIMINT::set_callback(this->se, &GPIOWriter::interrupt_callback_wrapper, this);
	//TIMINT::start_timer(&this->display_timer, this->se, 0, DISPLAY_CYCLE);

	//Save starting time point for time measurement
	this->start = std::chrono::high_resolution_clock::now();

	return retval;		
}

SevenSegNum GPIOWriter::num_to_seg(float number)
{
	SevenSegNum result;
	
	//We only do positive numbers
	number = fabs(number);

	//Truncate number (max. is 999.9)
	//Other digits (higher than hundreds and lower than tenths) are ignored
	number = number - ((int)(number / 1000.0)) * 1000;

	result.hundreds  = (int)( number                           / 100.0);
	result.tens      = (int)((number - result.hundreds*100)    /  10.0);
	result.ones	 = (int)((number - result.hundreds*100 
					 - result.tens*10)         /   1.0);
	result.tenths	 = (int)((number - result.hundreds*100 
					 - result.tens*10 
					 - result.ones*1)          /   0.1);
	
	//Store resulting number as a float
	result.number    =   result.hundreds * 100.0 
			   + result.tens     *  10.0 
			   + result.ones     *   1.0
			   + result.tenths   *   0.1;
 
	return result;
}

eError GPIOWriter::print_number(float number, unsigned int sequence)
{
	//Function shall be executed periodically by timer interrupt
	//One cycle prints out one 4 digit number with one decimal dot

	//Create number	
	SevenSegNum num = num_to_seg(number);

	unsigned int num_seq = 16;
	unsigned int seq = sequence % num_seq;
	
	//Depending on the sequence number, we set or reset the pins
	switch (seq)
	{
		//Hundreds
		case 0:
			this->adr_1->setval_gpio(true);
			set_segments(num.hundreds, 16, false);
			set_dot(num.number, 1);
			break;
		case 1:
			//Wait
			break;
		case 2:
			reset_display(2);
			reset_display(1);
			break;
		case 3: 
			//Wait
			break;

		//Tens
		case 4:
			this->adr_2->setval_gpio(true);
			set_segments(num.tens, 16, false);
			set_dot(num.number, 2);
			break;
		case 5:
			//Wait
			break;
		case 6:
			reset_display(2);
			reset_display(1);
			break;
		case 7: 
			//Wait
			break;

		//Ones
		case 8:
			this->adr_3->setval_gpio(true);
			set_segments(num.ones, 16, false);
			set_dot(num.number, 3);
			break;
		case 9:
			//Wait
			break;
		case 10:
			reset_display(2);
			reset_display(1);
			break;
		case 11: 
			//Wait
			break;

		//Tenths
		case 12:
			this->adr_4->setval_gpio(true);
			set_segments(num.tenths, 16, false);
			break;
		case 13:
			//Wait
			break;
		case 14:
			reset_display(2);
			reset_display(1);
			break;
		case 15: 
			//Wait
			break;
	}

	return eSuccess;
}

eError GPIOWriter::print_number(float number)
{
	//Function shall be executed periodically
	//One cycle prints out one 4 digit number with one decimal dot

	//Check if writer is initialized
	eError retval = check_init();
	if (retval != eSuccess)
		return retval;

	//Create number	
	SevenSegNum num = num_to_seg(number);

	if (param_list.get<bool>("debug writer") == true)
	{
		my_console.WriteToSplitConsole("Number: " + std::to_string(number), param_list.get<int>("split main"));
		//std::cout << "GPIO Writer Class: Found digits ";
		//std::cout << num.hundreds << "_" << num.tens << "_";
		//std::cout << num.ones << "_" << num.tenths << std::endl;
	}
	
	//Depending on the control mode, the timing is different
	if (this->control_mode == 0)
	{
		//Control mode 0
		//First we set the address, then we set the segments

		//Set hundreds
		this->adr_1->setval_gpio(true);
		//We set the pins of the segments according to the received digits
		set_segments(num.hundreds, 16, false);
		set_dot(num.number, 1);
		//We keep this for some time
		usleep(HOLD_ADDR_SEG);

		//We reset the segments
		reset_display(2);
		//We reset the addresses
		reset_display(1);

		//We pause for a short time
		usleep(PAUSE_ADDR_SEG);

		//Set tens
		this->adr_2->setval_gpio(true);
		set_segments(num.tens, 16, false);
		set_dot(num.number, 2);
		usleep(HOLD_ADDR_SEG);
		reset_display(2);
		reset_display(1);
		usleep(PAUSE_ADDR_SEG);

		//Set ones
		this->adr_3->setval_gpio(true);
		set_segments(num.ones, 16, false);
		set_dot(num.number, 3);
		usleep(HOLD_ADDR_SEG);
		reset_display(2);
		reset_display(1);
		usleep(PAUSE_ADDR_SEG);

		//Set tenths
		this->adr_4->setval_gpio(true);
		set_segments(num.tenths, 16, false);
		//There's no decimal dot needed at the end for now
		usleep(HOLD_ADDR_SEG);
		reset_display(2);
		reset_display(1);
		usleep(PAUSE_ADDR_SEG);
	}
	
	if (this->control_mode == 1)
	{
		//Control mode 1
		//First we set the segments, then we set the address

		//Set hundreds
		//We set the pins of the segments according to the received digits
		set_segments(num.hundreds, 16, false);
		set_dot(num.number, 1);
		this->adr_1->setval_gpio(true);
		//We keep this for some time
		usleep(HOLD_SEG_ADDR);

		//We reset the addresses
		reset_display(1);
		//We reset the segments
		reset_display(2);

		//We pause for a short time
		usleep(PAUSE_SEG_ADDR);

		//Set tens
		set_segments(num.tens, 16, false);
		set_dot(num.number, 2);
		this->adr_2->setval_gpio(true);
		usleep(HOLD_SEG_ADDR);
		reset_display(1);
		reset_display(2);
		usleep(PAUSE_SEG_ADDR);

		//Set ones
		set_segments(num.ones, 16, false);
		set_dot(num.number, 3);
		this->adr_3->setval_gpio(true);
		usleep(HOLD_SEG_ADDR);
		reset_display(1);
		reset_display(2);
		usleep(PAUSE_SEG_ADDR);

		//Set tenths
		set_segments(num.tenths, 16, false);
		//There's no decimal dot needed at the end for now
		this->adr_4->setval_gpio(true);
		usleep(HOLD_SEG_ADDR);
		reset_display(1);
		reset_display(2);
		usleep(PAUSE_SEG_ADDR);
	}

	return retval;
}

void GPIOWriter::set_segments(int number, int offset, bool text)
{
	//Write debug lines
	if (param_list.get<bool>("debug writer") == true)
	{
		//std::cout << "GPIO Writer Class: Segments of " << number+offset << "  ";
		//std::cout << (aChars[number+offset] & 0x01) << "_";
		//std::cout << (aChars[number+offset] & 0x02) << "_";
		//std::cout << (aChars[number+offset] & 0x04) << "_";
		//std::cout << (aChars[number+offset] & 0x08) << "_";
		//std::cout << (aChars[number+offset] & 0x10) << "_";
		//std::cout << (aChars[number+offset] & 0x20) << "_";
		//std::cout << (aChars[number+offset] & 0x40) << std::endl;
	}

	//We check the digit and select the pins accordingly
	this->seg_1->setval_gpio((aChars[number+offset] & 0x01) == 0x01);
	this->seg_2->setval_gpio((aChars[number+offset] & 0x02) == 0x02);
	this->seg_3->setval_gpio((aChars[number+offset] & 0x04) == 0x04);
	this->seg_4->setval_gpio((aChars[number+offset] & 0x08) == 0x08);
	this->seg_5->setval_gpio((aChars[number+offset] & 0x10) == 0x10);
	this->seg_6->setval_gpio((aChars[number+offset] & 0x20) == 0x20);
	this->seg_7->setval_gpio((aChars[number+offset] & 0x40) == 0x40);

	//The dot is only displayed regularly, if we are in text mode. If a number is being
	//displayed, the dot is handled differently with function set_dot()
	if (text == true)
		this->seg_8->setval_gpio((aChars[number+offset] & 0x80) == 0x80);
}

void GPIOWriter::set_dot(float number, int select)
{
	//Decimal dot is implemented only at one position, code in /* */ below is used
	//for advanced numerical display (varying decimal dot location)
	/*
	//We have to check where the decimal dot has to appear
	//Number has already been forced to be positive, so we can get the log10
	//But we still have to check if the number is 0
	double exp_dot;
	if (number > 0)
		exp_dot = floor(log10((double)number));
	else
		exp_dot = 0;
	*/

	//We have to distinguish between 5 cases (int select)
	// 1  2  3  5  4
	//8. 8. 8. 8.  none
	//Since we truncate the number from hundreds to tenths, the decimal dot can
	//only appear on (3)
	
	//Dot appears on (1) - not used
	//Dot appears on (2) - not used
	//Dot appears on (3) - value is <  1000,0
	//Dot appears on (4) - value is >= 1000,0
	//Dot appears on (5) - not used so far, could be used to indicate some calculation state...

	//We just have to check if the decimal dot has to be set to true for this digit (1...4)
	/*
	if (  (select == 1 && exp_dot <= 0) 
	    ||(select == 2 && exp_dot >= 1 && exp_dot < 2)
	    ||(select == 3 && exp_dot >= 2 && exp_dot < 3)
            ||(select == 5) )
		this->seg_8->setval_gpio(true);
	else
		this->seg_8->setval_gpio(false);
	*/

	//Set the decimal dot
	if (select == 3 && number < 1000)
		this->seg_8->setval_gpio(true);
	else
		this->seg_8->setval_gpio(false);
}

eError GPIOWriter::print_string(const char* text, int size, int pos)
{
	//Function shall be executed periodically
	//One cycle prints out a string
	//Note: we do not check if the size is actually correct and matches the
	//actual size of the const char*! 
	//			  D I S P L A Y
	//		       Dig1 Dig2 Dig3 Dig4
	//		         X    X    X    X
	// Example: size = 7
	// Array: x x x x  |  x    x    x    x    x    x    x  |  x x x x
	// Index: 0 1 2 3     4    5    6    7    8    9   10    11....14
	// If position (parameter) is 0, Dig1 maps to index 0, Dig2 to index 1 etc.
	// If position (parameter) is 4, Dig1 maps to index 4, Dig2 ti index 5 etc.

	//Check if writer is initialized
	eError retval = check_init();
	if (retval != eSuccess)
		return retval;

	//Check if there's only valid characters
	retval = check_chars(text, size);
	if (retval != eSuccess)
		return retval;

	//Position max. is size + 4
	if (pos > (size + 4))
		pos = size + 4;

	//Create temporary string to display - 4 chars on left and right side added
	char disp[size + 8];
	
	//Fill in string - append 4 spaces left and right
	for (int i = 0; i < size + 8; i++)
		if (i < 4)
			disp[i] = ' ';
		else if (i >= 4 && i < (size + 4))
			disp[i] = text[i - 4];
		else
			disp[i] = ' ';

	if (param_list.get<bool>("debug writer") == true)
		my_console.WriteToSplitConsole("GPIO Writer Class: Printing " + std::string(text), param_list.get<int>("split main"));

	//Select chars to display
	char first = disp[pos+0];
	char secnd = disp[pos+1];
	char third = disp[pos+2];
	char forth = disp[pos+3];
	
	//Depending on the control mode, the timing is different
	if (this->control_mode == 0)
	{
		//Control mode 0
		//First we set the address, then we set the segments

		//Set first 
		this->adr_1->setval_gpio(true);
		//We set the pins of the segments according to the received digits
		set_segments(first, -32, true);
		//We keep this for some time
		usleep(HOLD_ADDR_SEG);

		//We reset the segments
		reset_display(2);
		//We reset the addresses
		reset_display(1);

		//We pause for a short time
		usleep(PAUSE_ADDR_SEG);

		//Set second
		this->adr_2->setval_gpio(true);
		set_segments(secnd, -32, true);
		usleep(HOLD_ADDR_SEG);
		reset_display(2);
		reset_display(1);
		usleep(PAUSE_ADDR_SEG);

		//Set third
		this->adr_3->setval_gpio(true);
		set_segments(third, -32, true);
		usleep(HOLD_ADDR_SEG);
		reset_display(2);
		reset_display(1);
		usleep(PAUSE_ADDR_SEG);

		//Set fourth
		this->adr_4->setval_gpio(true);
		set_segments(forth, -32, true);
		usleep(HOLD_ADDR_SEG);
		reset_display(2);
		reset_display(1);
		usleep(PAUSE_ADDR_SEG);
	}
	
	if (this->control_mode == 1)
	{
		//Control mode 1
		//First we set the segments, then we set the address

		//Set first
		//We set the pins of the segments according to the received digits
		set_segments(first, -32, true);
		this->adr_1->setval_gpio(true);
		//We keep this for some time
		usleep(HOLD_ADDR_SEG);

		//We reset the addresses
		reset_display(1);
		//We reset the segments
		reset_display(2);

		//We pause for a short time
		usleep(PAUSE_ADDR_SEG);

		//Set second
		set_segments(secnd, -32, true);
		this->adr_2->setval_gpio(true);
		usleep(HOLD_ADDR_SEG);
		reset_display(1);
		reset_display(2);
		usleep(PAUSE_ADDR_SEG);

		//Set third
		set_segments(third, -32, true);
		this->adr_3->setval_gpio(true);
		usleep(HOLD_ADDR_SEG);
		reset_display(1);
		reset_display(2);
		usleep(PAUSE_ADDR_SEG);

		//Set fourth
		set_segments(forth, -32, true);
		this->adr_4->setval_gpio(true);
		usleep(HOLD_ADDR_SEG);
		reset_display(1);
		reset_display(2);
		usleep(PAUSE_ADDR_SEG);
	}

	return retval;
}

eError GPIOWriter::print_string(std::string text, unsigned int cycle)
{
	//Declare return value
	eError retval = eSuccess;

	//We measure the time since application start
	auto elapsed = std::chrono::high_resolution_clock::now() - this->start;
	long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	static long long ms_old = ms;

	//Retrieve lenght of string to display
	int len = text.length();

	//Position
	static int pos = 0;
	//Cycle finished flag
	bool cycle_finished = false;

	if (ms - ms_old > cycle)
	{
		pos++;
		if (pos > (len + 4))
		{
			//Cycle has finished, reset pos to zero
			cycle_finished = true;
			pos = 0;
		}
		ms_old = ms;
	}
	
	retval = this->print_string(text.c_str(), len, pos);

	//Check return value
	if ((cycle_finished == false) && (retval == eSuccess))	
		retval = eGPIOWriterCycleNotFinished;

	return retval;
}

eError GPIOWriter::reset_display(int option)
{
	//Check init
	eError retval = check_init();
	if (retval != eSuccess)
		return retval;

	//We have the following reset options
	//Option 0: reset all of the pins
	//Option 1: reset all of the address pins
	//Option 2: reset all of the segment pins
	switch (option)
	{
		case 0:
			this->adr_1->setval_gpio(false);
			this->adr_2->setval_gpio(false);
			this->adr_3->setval_gpio(false);
			this->adr_4->setval_gpio(false);
			this->seg_1->setval_gpio(false);
			this->seg_2->setval_gpio(false);
			this->seg_3->setval_gpio(false);
			this->seg_4->setval_gpio(false);
			this->seg_5->setval_gpio(false);
			this->seg_6->setval_gpio(false);
			this->seg_7->setval_gpio(false);
			this->seg_8->setval_gpio(false);
			break;
		case 1: 
			this->adr_1->setval_gpio(false);
			this->adr_2->setval_gpio(false);
			this->adr_3->setval_gpio(false);
			this->adr_4->setval_gpio(false);
			break;
		case 2:
			this->seg_1->setval_gpio(false);
			this->seg_2->setval_gpio(false);
			this->seg_3->setval_gpio(false);
			this->seg_4->setval_gpio(false);
			this->seg_5->setval_gpio(false);
			this->seg_6->setval_gpio(false);
			this->seg_7->setval_gpio(false);
			this->seg_8->setval_gpio(false);
			break;
		default:
			retval = eGPIOWriterResetInvalidOption;
			break;
	}

	return retval;
}

eError GPIOWriter::check_init()
{
	//Declare return value
	eError retval = eSuccess;

	//Check if Writer is initialized
	if (this->is_initialized == false)
		retval = eGPIOWriterNotInitialized;
	
	return retval;
}

eError GPIOWriter::check_chars(const char* text, int size)
{
	//Declare return value
	eError retval = eSuccess;

	for (int i = 0; i < size; i++)
	{
		if ((text[i] < 32) || (text[i] > 127))
			retval = eGPIOWriterInvalidCharacter;
	}

	return retval;
}

eError GPIOWriter::set_mode(int mode)
{
	//Declare return value
	eError retval = eSuccess;

	//Set the control mode of the writer class
	//The control mode defines the timing behaviour of the 
	//address and the segment lines
	//0 means, set address first, then segments
	//1 means, set segments first, then address
	//Re-set is done in opposite order

	//Write debug line
	if (param_list.get<bool>("debug writer") == true)
		my_console.WriteToSplitConsole("GPIO Writer Class: Set control mode = " + std::to_string(mode), param_list.get<int>("split main"));

	switch (mode)
	{
		case 0:
			this->control_mode = 0;
			break;
		case 1:
			this->control_mode = 1;
			break;
		default:
			retval = eGPIOWriterInvalidControlMode;
	}

	return retval;
}

eError GPIOWriter::set_timing(SevenSegTiming timing)
{
	//Declare return value
	eError retval = eSuccess;

	//Set the timing of the writer class

	//Write debug line
	if (param_list.get<bool>("debug writer") == true)
	{
		my_console.WriteToSplitConsole("GPIO Writer Class: Set timing... ", param_list.get<int>("split main"));
		//std::cout << "GPIO Writer Class: HOLD_ADDR_SEG " << timing.hold_addr_seg << std::endl;
		//std::cout << "GPIO Writer Class: PAUSE_ADDR_SEG " << timing.pause_addr_seg << std::endl;
		//std::cout << "GPIO Writer Class: HOLD_SEG_ADDR " << timing.hold_seg_addr << std::endl;
		//std::cout << "GPIO Writer Class: PAUSE_SEG_ADDR " << timing.pause_seg_addr << std::endl;
	}

	if ((timing.hold_addr_seg < 0) 
         || (timing.pause_addr_seg < 0) 
	 || (timing.hold_seg_addr < 0) 
	 || (timing.pause_seg_addr < 0))
	{
		retval = eGPIOWriterInvalidTiming;
	}
	else
	{
		this->timing_config = timing;
	}

	return retval;
}

void GPIOWriter::interrupt_callback(sigval_t val)
{
	GPIOWriter* instance = (GPIOWriter*)val.sival_ptr;

	if (instance->interrupt_activated == false)
		return;
	else
	{
		instance->print_number(instance->value, this->sequence);
		this->sequence++;
		if (this->sequence >= 16)
			this->sequence = 0;
	}
}

void GPIOWriter::interrupt_callback_wrapper(sigval_t val)
{
	GPIOWriter* obj = (GPIOWriter*)val.sival_ptr;
	obj->interrupt_callback(val);
}

eError GPIOWriter::activate_timer_interrupt()
{
	this->interrupt_activated = true;
	return eSuccess;
}

eError GPIOWriter::deactivate_timer_interrupt()
{
	this->interrupt_activated = false;
	return eSuccess;
}

eError GPIOWriter::set_value(double value)
{
	this->value = value;
	return eSuccess;
}
	
#endif