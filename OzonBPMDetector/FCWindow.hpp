#ifndef _FC_WINDOW
#define _FC_WINDOW

#include <string>
#include <future>
#include <map>
#include <mutex>

//Forward declaration
class FCWindow;

//Base struct for data exchange
struct FCWindowData_t
{
	virtual ~FCWindowData_t() { }
};

//Static GLUT information
struct FCWindowCommon_t
{
	int argc;
	char** argv;
	std::future<void> ftr;
	bool running;
	std::map<int, FCWindow*> windows;
	std::mutex mtx;
	unsigned int sleep;
	int res_x, res_y;
	bool fullscreen;
};

//Base window class
class FCWindow
{
public:
	explicit FCWindow(int, int, std::string, bool);
	virtual ~FCWindow();

	//Public update method
	//Uses pointer to 
	virtual void update(FCWindowData_t*) = 0;
	
	//Getter function for window parameters
	int get_width();
	int get_height();
	
	//Public quit method
	bool get_quit();

	//Static methods for starting and stopping GLUT
	//Init GLUT
	static void init(int, char**);
	//Start event handler - parameter fullscreen
	static void start();
	//Stop event handler
	static void stop();
	
	//Static getter functions
	//Get screen resolution and fullscreen option
	static int get_res_x();
	static int get_res_y();
	static bool get_fullscreen();

protected:
	//Member callbacks - overridden by derived class
	//Only display method is pure virtual, since in most cases
	//the default implementation for reshape and Keyboard are appropriate
	virtual void reshape_(int, int);
	virtual void display_(void) = 0;
	virtual void Keyboard_(unsigned char, int, int);

	//Static members used for GLUT initialisation
	static FCWindowCommon_t static_data;

	//Handle for accessing window by ID
	int handle;

	//Size of window
	int x, y;
	//Quit signal
	bool quit;

private:
	//Static callbacks
	static void reshape(int, int);
	static void display(void);
	static void Keyboard(unsigned char, int, int);
};

#endif
