#ifndef _FC_WINDOW
#define _FC_WINDOW

#include <string>
#include <future>
#include <map>
#include <mutex>

//Forward declaration
class FCWindow;

//Static GLUT information
struct FCWindowCommon_t
{
	int argc;
	char** argv;
	std::future<void> ftr;
	bool running;
	std::map<int, FCWindow*> windows;
	std::mutex mtx;
	unsigned int index;
	unsigned int sleep;
};

struct FCWindowData_t
{
	union
	{
		const char* string_data;
		int int_data;
		double double_data;
		double* double_array_data;
	};
};

//Base window class
class FCWindow
{
public:
	explicit FCWindow(int, int, std::string);
	virtual ~FCWindow();

	//Public update method
	virtual void update(FCWindowData_t&) = 0;

	//Static methods for starting and stopping GLUT
	//Init GLUT
	static void start(int, char**);
	//Start event handler - parameter fullscreen
	static void start(bool);
	//Stop event handler
	static void stop();

protected:
	//Member callbacks - must be overridden by derived class
	virtual void reshape_(int, int) = 0;
	virtual void display_(void) = 0;

	//Static members used for GLUT initialisation
	static FCWindowCommon_t static_data;

	//Handle for accessing window by ID
	int handle;

	//Size of window
	int x, y;

private:
	//Static callbacks
	static void reshape(int, int);
	static void display(void);
};

//Derived window classes
//Simple text display
class FCWindowLabel : public FCWindow
{
public:
	explicit FCWindowLabel(int, int, std::string);
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
	void reshape_(int, int);
	void display_(void);
};

//Frequency spectrum display
class FCWindowSpectrum : public FCWindow
{
public:
	explicit FCWindowSpectrum(int, int, std::string, int);
	~FCWindowSpectrum();

	void update(FCWindowData_t&);

private:
	//Font used on this window
	void* font;

	//Data for frequency array
	int size;
	double* data;
	
	//Graphic parameters
	int x_border;
	int y_border_bottom;
	int y_border_top;
	int x_margin;
	double max_value;
	double min_value;

	//Helper function for displaying text
	void output(int, int, std::string);
	
	//Member callbacks - must be overridden by derived class
	void reshape_(int, int);
	void display_(void);
};

#endif
