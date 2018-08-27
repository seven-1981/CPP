//The GPIO implementation is not used on WIN32
#ifndef _WIN32

#include "GPIOPin.hpp"
#include "bpm_globals.hpp"
#include "SplitConsole.hpp"
#include <wiringPi.h>
#include <iostream>

//Extern split console instance
extern SplitConsole my_console;

bool GPIOPin::is_initialized = false;

GPIOPin::GPIOPin()
{
	//Default constructor
	this->gpio_number = 2;
	//Write debug line
	#ifdef DEBUG_GPIO
		std::cout << "GPIO Pin Class: Creating pin " << gpio_number << std::endl;
	#endif
	this->direction = eOUT;
	setdir_gpio(eOUT);

	this->pin_value = false;
	setval_gpio(false);

	//Write debug line
	#ifdef DEBUG_GPIO
		std::cout << "GPIO Pin Class: Direction = " << direction << std::endl;
		std::cout << "GPIO Pin Class: Pin value = " << pin_value << std::endl;
	#endif
}

GPIOPin::GPIOPin(int pin_number, eDirection direction, bool value)
{
	//Constructor
	this->gpio_number = pin_number;
	//Write debug line
	#ifdef DEBUG_GPIO
		std::cout << "GPIO Pin Class: Creating pin " << pin_number << std::endl;
	#endif
	this->direction = direction;
	setdir_gpio(direction);

	//Check if pin is input or output
	if (this->direction == eOUT)
	{
		this->pin_value = value;
		setval_gpio(value);
	}

	if (this->direction == eIN)
	{
		this->pin_value = getval_gpio();
	}

	//Write debug line
	#ifdef DEBUG_GPIO
		std::cout << "GPIO Pin Class: Direction = " << direction << std::endl;
		std::cout << "GPIO Pin Class: Pin value = " << pin_value << std::endl;
	#endif
}

int GPIOPin::init_gpio()
{	
	//Initialize wiringPi
	int result = wiringPiSetup();
	if (result == 0)
		GPIOPin::is_initialized = true;
	else
		GPIOPin::is_initialized = false;

	//Write debug line
	#ifdef DEBUG_GPIO
		std::cout << "GPIO Pin Class: wiringPiSetup = " << result << std::endl;
	#endif
	
	return result;
}

int GPIOPin::gpio_to_wiringPi(int gpio_number)
{
	//Get GPIO number and translate to WiringPi pin number
	switch (gpio_number)
	{
		case  2:	return 8;
		case  3:	return 9;
		case  4:	return 7;
		case  5:	return 21;
		case  6:	return 22;
		case  7:	return 11;
		case  8:	return 10;
		case  9:	return 13;
		case 10:	return 12;
		case 11:	return 14;
		case 12:	return 26;
		case 13:	return 23;
		case 14:	return 15;
		case 15:	return 16;
		case 16:	return 27;
		case 17:	return 0;
		case 18:	return 1;
		case 19:	return 24;
		case 20:	return 28;
		case 21:	return 29;
		case 22:	return 3;
		case 23:	return 4;
		case 24:	return 5;
		case 25:	return 6;
		case 26:	return 25;
		case 27:	return 2;
		default:	return -1;
	}
}

int GPIOPin::get_gpio_number()
{
	return this->gpio_number;
}

eDirection GPIOPin::get_gpio_direction()
{
	return this->direction;
}

bool GPIOPin::get_gpio_value()
{
	return this->pin_value;
}

eError GPIOPin::setdir_gpio(eDirection direction)
{
	//Declare return value
	eError retval = eSuccess;

	//Check if WiringPi is initialized
	retval = check_init();
	//If something's not ok -> return and send error code
	if (retval != eSuccess)
		return retval;

	//Check if pin is initalized properly
	//Note: WiringPi numbering differs from GPIO numbering
	int wiringPi_number = gpio_to_wiringPi(this->gpio_number);
	retval = check_pin(wiringPi_number);
	//If something's not ok -> return and send error code
	if (retval != eSuccess)
		return retval;

	//Set direction of the pin
	if (direction == eOUT)
		pinMode(wiringPi_number, OUTPUT);
	else
		pinMode(wiringPi_number, INPUT);

	#ifdef DEBUG_GPIO
		std::cout << "GPIO Pin Class: Set direction on pin " << wiringPi_number;
		std::cout << " equals gpio " << this->gpio_number << std::endl;
		std::cout << "GPIO Pin Class: Direction = " << direction << std::endl;
	#endif

	return retval;
}

eError GPIOPin::setval_gpio(bool value)
{
	//Declare return value
	eError retval = eSuccess;

	//Check if WiringPi is initialized
	retval = check_init();
	//If something's not ok -> return and send error code
	if (retval != eSuccess)
		return retval;

	//Check if pin is initalized properly
	//Note: WiringPi numbering differs from GPIO numbering
	int wiringPi_number = gpio_to_wiringPi(this->gpio_number);
	retval = check_pin(wiringPi_number);
	//If something's not ok -> return and send error code
	if (retval != eSuccess)
		return retval;

	//Check if pin is set up as output
	retval = check_dir();
	//If something's not ok -> return and send error code
	if (retval != eSuccess)
		return retval;

	//Set pin to desired value
	if (value == true)
		digitalWrite(wiringPi_number, HIGH);
	else
		digitalWrite(wiringPi_number, LOW);

	#ifdef DEBUG_GPIO
		std::cout << "GPIO Pin Class: Set value on pin " << wiringPi_number;
		std::cout << " equals gpio " << this->gpio_number << std::endl;
		std::cout << "GPIO Pin Class: Value = " << value << std::endl;
	#endif	

	return retval;
}

bool GPIOPin::getval_gpio()
{
	//Declare return value
	eError retval = eSuccess;

	//Check if WiringPi is initialized
	retval = check_init();
	//If something's not ok -> return false
	if (retval != eSuccess)
		return false;

	//Check if pin is initialized properly
	//Note: WiringPi numbering differs from GPIO numbering
	int wiringPi_number = gpio_to_wiringPi(this->gpio_number);
	retval = check_pin(wiringPi_number);
	//If something's not ok -> return false
	if (retval != eSuccess)
		return false;

	//Return the value at the pin, HIGH or LOW
	return (digitalRead(wiringPi_number == HIGH));
}

eError GPIOPin::check_init()
{
	//Declare return value
	eError retval = eWiringPiNotInitialized;

	//Check if WiringPi is initialized
	if (GPIOPin::is_initialized == true)
	{
		retval = eSuccess;
	}

	return retval;
}

eError GPIOPin::check_pin(int number)
{
	//Declare return value
	eError retval = eGPIONumberNotValid;

	//Check if pin number is valid
	if (number != -1)
	{
		retval = eSuccess;
	}

	return retval;
}

eError GPIOPin::check_dir()
{
	//Declare return value
	eError retval = eGPIOConfiguredAsInput;

	//Check if pin is set up as output
	if (this->direction == eOUT)
	{
		retval = eSuccess;
	}

	return retval;
}

#endif