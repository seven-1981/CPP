#ifndef _FC_WINDOWLABEL
#define _FC_WINDOWLABEL

#include "FCWindow.hpp"

#include <string>

//Derived window classes
//Simple text display
class FCWindowLabel : public FCWindow
{
public:
	explicit FCWindowLabel(int, int, std::string, bool);
	~FCWindowLabel();

	void update(FCWindowData_t&);

private:
	//Font used on this window
	void* font;

	//Data to display on the window
	std::string data;

	//Helper functions for displaying text
	void output(int, int, std::string);
	void output(float, float, std::string);

	//Member callbacks - must be overridden by derived class
	void display_(void);
};

#endif