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

//Base window class
class FCWindow
{
public:
	explicit FCWindow(int, int, std::string, bool);
	virtual ~FCWindow();

	//Public update method
	virtual void update(FCWindowData_t&) = 0;
	//Public quit method
	bool get_quit();

	//Static methods for starting and stopping GLUT
	//Init GLUT
	static void init(int, char**);
	//Start event handler - parameter fullscreen
	static void start();
	//Stop event handler
	static void stop();

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