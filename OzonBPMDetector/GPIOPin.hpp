#ifndef _GPIO_PIN_H
#define _GPIO_PIN_H
//Purpose: Each object instantiated from this class will control a GPIO pin
//The GPIO pin number must be passed to the construtor

#include "bpm_globals.hpp"

class GPIOPin
{
public:
	//Static members
	//This initializer flag is not referring to the pin - it's referring to the wiringPi setup
	//It will be automatically executed if the first pin is created
	static bool is_initialized;

	//Constructors
	GPIOPin();
	GPIOPin(int pin_number, eDirection direction, bool value);

	//Static member functions
	static int init_gpio();
	static int gpio_to_wiringPi(int gpio_number);

	//Getter functions
	int get_gpio_number();
	eDirection get_gpio_direction();
	bool get_gpio_value();

	//Public member functions
	eError setdir_gpio(eDirection direction);
	eError setval_gpio(bool value);
	bool getval_gpio();

private:
	//Private members
	//GPIO number, refers to GPIO pin number on Raspberry Pi2
	int gpio_number;
	//Direction enum, input or output
	eDirection direction;
	//Value of the pin, either high or low
	bool pin_value;

	//Private member functions
	//used by GPIOPin class internally
	eError check_init();
	eError check_pin(int number);
	eError check_dir();
};

#endif