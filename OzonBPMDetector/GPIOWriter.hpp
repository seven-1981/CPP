#ifndef _GPIO_WRITER_H
#define _GPIO_WRITER_H

#include "bpm_globals.hpp"
#include "GPIOPin.hpp"

class GPIOWriter
{
public:
	//Constructor
	//Only use default constructor
	//Create 4 address pins and 8 segment pins
	//The info comes from the bpm_globals.hpp file
	GPIOWriter();

	//Destructor
	~GPIOWriter();

	//Public member functions
	//Convert a number to 7Seg Struct
	SevenSegNum num_to_seg(float number);
	//Print out a number
	eError print_number(float number);
	//Print out a string
	eError print_string(const char* text, int size, int pos);
	//Set all pins to zero
	eError reset_display(int option);
	//Configuration setter
	eError set_mode(int mode);
	//Timing setter
	eError set_timing(SevenSegTiming timing);	

private:
	//Initialized state
	bool is_initialized;
	//Control mode
	//Pin control mode: 0=address first, 1=segments first
	int control_mode;
	//Timing configuration, times in microseconds
	SevenSegTiming timing_config;
	//Address pins
	GPIOPin* adr_1;
	GPIOPin* adr_2;
	GPIOPin* adr_3;
	GPIOPin* adr_4;
	//Segment pins
	GPIOPin* seg_1;
	GPIOPin* seg_2;
	GPIOPin* seg_3;
	GPIOPin* seg_4;
	GPIOPin* seg_5;
	GPIOPin* seg_6;
	GPIOPin* seg_7;
	GPIOPin* seg_8;

	//Init function
	//Called upon instance creation
	eError init_writer();
	//Check initialization of writer
	eError check_init();
	//Check if characters are valid
	eError check_chars(const char* text, int size);
	//Helper function for print method
	//No return value because initialization is already checked before
	void set_segments(int number, int offset, bool text);
	//Display decimal dot
	void set_dot(float number, int select);
};

#endif