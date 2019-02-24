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
	bool fullscreen;
	unsigned int res_x, res_y;
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

	//Size information
	int size;
};

struct FCWindowSpectrumSize_t
{
	int x_border = 50;
	int y_border_bottom = 50;
	int y_border_top = 100;
	int x_margin = 5;

	double max_value = 100.0;
	double min_value = 0.0;

	int n_ticks = 11;
	int tick_width = 5;
};

//Base window class
class FCWindow
{
public:
	explicit FCWindow(int, int, std::string, bool);
	virtual ~FCWindow();

	//Public update method
	virtual void update(FCWindowData_t&) = 0;

	//Static methods for starting and stopping GLUT
	//Init GLUT
	static void init(int, char**);
	//Start event handler - parameter fullscreen
	static void start();
	//Stop event handler
	static void stop();

protected:
	//Member callbacks - must be overridden by derived class
	virtual void reshape_(int, int) = 0;
	virtual void display_(void) = 0;
	virtual void Keyboard_(unsigned char, int, int) = 0;

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
	static void Keyboard(unsigned char, int, int);
};

//Derived window classes
//Simple text display
class FCWindowLabel : public FCWindow
{
public:
	explicit FCWindowLabel(int, int, std::string, bool);
	~FCWindowLabel();

	void update(FCWindowData_t&);
	bool get_quit();

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
	void Keyboard_(unsigned char, int, int);

	//Quit signal
	bool quit;
};

//Frequency spectrum display
class FCWindowSpectrum : public FCWindow
{
public:
	explicit FCWindowSpectrum(int, int, std::string, int, bool);
	~FCWindowSpectrum();

	void update(FCWindowData_t&);
	void set_param(FCWindowSpectrumSize_t&);
	bool get_quit();

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
	int n_ticks;
	int tick_width;

	//Auxiliary parameters
	int x_size;
	int x_begin;
	int x_diff;
	int y_bar;
	int y_diff;
	double m;
	double n;

	//Quit signal
	bool quit;

	//Helper function for displaying text
	void output(int, int, std::string);
	//Helper function to initialize non spectrum content
	void init();
	
	//Member callbacks - must be overridden by derived class
	void reshape_(int, int);
	void display_(void);
	void Keyboard_(unsigned char, int, int);
};

#endif