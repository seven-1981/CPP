#include "FCWindowManager.hpp"
#include "FCWindowLabel.hpp"
//#include "FCWindowSpectrum.hpp"
#include "FCWindowGraph.hpp"

#include <GL/freeglut.h>

const int FCWINDOW_DISPLAY_SLEEP = 10;
const int FCWINDOW_EXIT_WAIT = 1.0;

//Static class data
FCWindowCommon_t FCWindowManager::static_data;

//Factory method for creation of windows
FCWindow* FCWindowManager::create(FCWindowType_e type, FCWindowParam_t& param)
{
	//Create new window according to type
	FCWindow* pWindow;
	switch (type)
	{
		case FCWindowType_e::TypeWindowLabel:
			//Create new window
			pWindow = new FCWindowLabel(param);
			//Update static map (handle and instance)
			FCWindowManager::add_window(pWindow);
			return pWindow;
			break;
		
		case FCWindowType_e::TypeWindowSpectrum:	
			//Create new window
			pWindow = new FCWindowSpectrum(param);
			//Update static map (handle and instance)
			FCWindowManager::add_window(pWindow);
			return pWindow;
			break;
		
		case FCWindowType_e::TypeWindowGraph:
			//Create new window
			pWindow = new FCWindowGraph(param);
			//Update static map (handle and instance)
			FCWindowManager::add_window(pWindow);
			return pWindow;
			break;
			
		default:
			return nullptr;
			break;
	}
}

int FCWindowManager::get_res_x()
{
	return FCWindowManager::static_data.res_x;
}

int FCWindowManager::get_res_y()
{
	return FCWindowManager::static_data.res_y;
}

void FCWindowManager::init(int argc, char** argv)
{
	//Init GLUT and save static data
	FCWindowManager::static_data.mtx.lock();
	FCWindowManager::static_data.argc = argc;
	FCWindowManager::static_data.argv = argv;
	glutInit(&FCWindowManager::static_data.argc, FCWindowManager::static_data.argv);
	//Display callback cycle sleep for decreasing processor usage
	FCWindowManager::static_data.sleep = FCWINDOW_DISPLAY_SLEEP;
	//Set screen resolution
	FCWindowManager::static_data.res_x = glutGet(GLUT_SCREEN_WIDTH);
	FCWindowManager::static_data.res_y = glutGet(GLUT_SCREEN_HEIGHT);
	//Init last handle and instance
	FCWindowManager::static_data.last_handle = -1;
	FCWindowManager::static_data.last_instance = nullptr;
	FCWindowManager::static_data.mtx.unlock();

	//Set options and display mode
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
  	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
}

void FCWindowManager::start()
{
	//Start has been called - execute event loop
	FCWindowManager::static_data.mtx.lock();

	//We only launch one thread for the GLUT event loop
	if (FCWindowManager::static_data.running == false)
	{
		FCWindowManager::static_data.ftr = std::async(std::launch::async, glutMainLoop);
		FCWindowManager::static_data.running = true;
	}
	
	FCWindowManager::static_data.mtx.unlock();
}

void FCWindowManager::stop()
{
	//Stop event loop if not already stopped
	//This 'if' is necessary, because if the last window is closed (by clicking 'x')
	//the underlying event loop is discontinued and in this case, we
	//shall not call the 'leave main loop' function
	//Stop is also called if the last window's close callback is executed
	
	//Check if event loop is running. If yes, stop it
	FCWindowManager::static_data.mtx.lock();
	if (FCWindowManager::is_running() == true)
		glutLeaveMainLoop();
	FCWindowManager::static_data.mtx.unlock();
	std::this_thread::sleep_for(std::chrono::seconds(FCWINDOW_EXIT_WAIT));
}

void FCWindowManager::display(void)
{
	//Static display callback
	//Select current instance using static map
	int current_handle = glutGetWindow();
	FCWindow* current_instance = get_instance(current_handle);
	//Check for valid pointer
	if (current_instance == nullptr)
		return;
	current_instance->display_();
	//Wait for a short amount of time - decreases processor usage
	std::this_thread::sleep_for(std::chrono::milliseconds(FCWindowManager::static_data.sleep));
}

void FCWindowManager::reshape(int width, int height)
{
	//Static reshape callback
	//Select current instance using static map
	int current_handle = glutGetWindow();
	FCWindowManager::static_data.mtx.lock();
	FCWindow* current_instance = FCWindowManager::static_data.windows.find(current_handle)->second;
	FCWindowManager::static_data.mtx.unlock();
  	current_instance->reshape_(width, height);
}

void FCWindowManager::Keyboard(unsigned char key, int x, int y)
{
	//Static Keyboard callback
	//Select current instance using static map
	int current_handle = glutGetWindow();
	FCWindowManager::static_data.mtx.lock();
	FCWindow* current_instance = FCWindowManager::static_data.windows.find(current_handle)->second;
	FCWindowManager::static_data.mtx.unlock();
  	current_instance->Keyboard_(key, x, y);
}

void FCWindowManager::close(void)
{
	//Static close callback
	//Select current instance using static map
	int current_handle = glutGetWindow();
	FCWindowManager::static_data.mtx.lock();
	FCWindow* current_instance = FCWindowManager::static_data.windows.find(current_handle)->second;
	FCWindowManager::static_data.mtx.unlock();
	//Set quit flag to true in specific window
	current_instance->quit = true;
	//Remove item from map
	FCWindowManager::remove_window(current_handle);
	//Check if there are any remaining windows. If no, stop event loop
	if (FCWindowManager::static_data.windows.size() == 0)
		FCWindowManager::stop();  	
}

void FCWindowManager::idle(void)
{

}

void FCWindowManager::add_window(FCWindow* window_to_add)
{
	//Update static map (handle and instance)
	FCWindowManager::static_data.mtx.lock();
	FCWindowManager::static_data.windows.insert(std::pair<int, FCWindow*>(window_to_add->get_handle(), window_to_add));
	FCWindowManager::static_data.mtx.unlock();	
}

void FCWindowManager::remove_window(int window_to_remove)
{
	//Update static map (handle and instance)
	FCWindowManager::static_data.mtx.lock();
	FCWindowManager::static_data.windows.erase(window_to_remove);
	FCWindowManager::static_data.mtx.unlock();
}	

bool FCWindowManager::is_running(void)
{
	return !(FCWindowManager::static_data.ftr.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
}

FCWindow* FCWindowManager::get_instance(int handle)
{
	FCWindow* instance;
	FCWindowManager::static_data.mtx.lock();
	//Check if handle has changed
	if (handle != FCWindowManager::static_data.last_handle)
	{
		FCWindowManager::static_data.last_handle = handle;
		instance = FCWindowManager::static_data.windows.find(handle)->second;
		FCWindowManager::static_data.last_instance = instance;
	}
	else
	{
		instance = FCWindowManager::static_data.last_instance;
	}
	FCWindowManager::static_data.mtx.unlock();
	return instance;
}
