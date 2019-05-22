#ifndef _FC_WINDOWMANAGER_H
#define _FC_WINDOWMANAGER_H

#include <string>
#include <future>
#include <map>
#include <mutex>

//Forward declaration
class FCWindow;

//Enumeration for window types
enum class FCWindowType_e
{
	TypeWindowLabel,
	TypeWindowSpectrum,
	TypeWindowGraph
};

//Struct for window parameters forward declaration
struct FCWindowParam_t;

//Static GLUT information
struct FCWindowCommon_t
{
	int argc;							//Console parameter, number of arguments
	char** argv;						//Passed arguments
	std::future<void> ftr;				//Future for event loop handler
	bool running;						//Flag indicates event loop running
	std::map<int, FCWindow*> windows;	//Table containing created windows and IDs
	std::mutex mtx;						//Mutex for static data access
	unsigned int sleep;					//Sleep value for decreasing processor usage
	int res_x, res_y;					//Screen resolution
	int last_handle;					//Last used window handle
	FCWindow* last_instance;			//Last used window pointer
};

//Window manager class, handling static data
class FCWindowManager
{
public:
	//Static methods for starting and stopping GLUT
	//Init GLUT
	static void init(int, char**);
	//Start event handler - parameter fullscreen
	static void start();
	//Stop event handler
	static void stop();
	
	//Method for creating FCWindow (factory)
	static FCWindow* create(FCWindowType_e, FCWindowParam_t&);
	
	//Static getter functions
	//Get screen resolution
	static int get_res_x();
	static int get_res_y();
	
	//FCWindowManager must be able to access FCWindow
	friend class FCWindow;

private:	
	//Static members used for GLUT initialisation
	static FCWindowCommon_t static_data;

	//Static callbacks
	static void reshape(int, int);
	static void display(void);
	static void Keyboard(unsigned char, int, int);
	static void close(void);
	static void idle(void);
	
	//Helper functions
	static void add_window(FCWindow*);
	static void remove_window(int);
	//Check if event loop is still running
	static bool is_running(void);
	//Compare active handle with last and get instance
	static FCWindow* get_instance(int handle);
};

#endif
