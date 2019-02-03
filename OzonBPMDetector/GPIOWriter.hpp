#ifndef _GPIO_WRITER_H
#define _GPIO_WRITER_H

#include <chrono>
#include <string>
#include <signal.h>
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
	//Print out a string - simplified handling
	eError print_string(std::string text, unsigned int cycle);
	//Set all pins to zero
	eError reset_display(int option);

	#ifndef _WIN32
		//Activation method for timer interrupt
		//After this function has been called, the print method is called by
		//timer interrupt internally. User must call set_value to control displayed value.
		eError activate_timer_interrupt();
		//Deactivate interrupt timer
		//After this function has been called, user must periodically call 
		//print method to display value.
		eError deactivate_timer_interrupt();
		//Setter method for internal value
		eError set_value(double value);
		//Timer interrupt callback wrapper - callbacks are static, so we can't 
		//pass a member function as callback, therefore a wrapper is needed.
		static void interrupt_callback_wrapper(sigval_t val);
	#endif

private:
	//Initialized state
	bool is_initialized;
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

	#ifndef _WIN32
		//Timer interrupt
		timer_t display_timer;
		//Event struct for timer interrupt event configuration
		sigevent se;
		//Timer interrupt callback 
		void interrupt_callback(sigval_t val);
		//Activation flag
		bool interrupt_activated;
		//Internal value
		double value;
		//Sequence value for interrupt
		unsigned int sequence;
		//Print out a number - internal callback
		eError print_number(float number, unsigned int sequence);
		//Save starting point for time measurement
		std::chrono::high_resolution_clock::time_point start;
	#endif

};

#endif