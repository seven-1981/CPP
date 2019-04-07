#ifndef _FC_WINDOW_H
#define _FC_WINDOW_H

#include "FCWindowManager.hpp"

#include <string>

//Forward declaration
class FCWindow;

//Base struct for data exchange
struct FCWindowData_t
{
	virtual ~FCWindowData_t() { }
};

//Base window class
class FCWindow
{
public:
	virtual ~FCWindow();

	//Public update method
	//Uses pointer to 
	virtual void update(FCWindowData_t*) = 0;
	
	//Getter function for window parameters
	int get_width();
	int get_height();
	
	//Public quit method
	bool get_quit();
	//Get handle method
	int get_handle();
	//Get fullscreen method
	bool get_fullscreen();
	
	//Setter method for keyboard function pointer
	void set_keyboard_callback(void (*)(unsigned char, int, int));
	
	//FCWindow must be able to access FCWindowManager
	friend class FCWindowManager;

protected:
	//Protected constructor (factory method)
	explicit FCWindow(FCWindowParam_t&);
	
	//Member callbacks - overridden by derived class
	//Only display method is pure virtual, since in most cases
	//the default implementation for reshape and Keyboard are appropriate
	virtual void reshape_(int, int);
	virtual void display_(void) = 0;
	virtual void Keyboard_(unsigned char, int, int);
	
	//Function pointer for enhancing Keyboard method
	void (*pKeyboard)(unsigned char, int, int);

private:
	//Handle for accessing window by ID
	int handle;
	//Fullscreen flag
	bool fullscreen;

	//Size of window
	int x, y;
	//Quit signal
	bool quit;
};

#endif
