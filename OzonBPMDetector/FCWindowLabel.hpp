#ifndef _FC_WINDOWLABEL
#define _FC_WINDOWLABEL

#include "FCWindow.hpp"

#include <string>
#include <vector>
#include <mutex>

//Define data element for a label window
struct FCWindowLabelDataItem_t
{
	std::string text = "text";
	int x = 100;
	int y = 100;
	float color_R = 1.0;
	float color_G = 1.0;
	float color_B = 1.0;
	bool use_stroke = false;
};

//Inherit from base data struct
struct FCWindowLabelData_t : public FCWindowData_t
{
	std::vector<FCWindowLabelDataItem_t> items;
};

//Derived window classes
//Simple text display
class FCWindowLabel : public FCWindow
{
public:
	~FCWindowLabel();

	void update(FCWindowData_t*);
	
	//FCWindowManager must be able to access FCWindowSpectrum
	friend class FCWindowManager;
	
protected:
	//Protected constructor (factory method)
	explicit FCWindowLabel(FCWindowParam_t&);

private:
	//Font used on this window
	void* font;
	//Mutex for data protection
	std::mutex label_mutex;

	//Data to display on the window
	FCWindowLabelData_t text_items;

	//Helper functions for displaying text
	void output(int, int, std::string);
	void output(float, float, std::string);

	//Member callbacks - must be overridden by derived class
	void display_(void);
};

#endif
