#include "FCWindow.hpp"

#include <GL/freeglut.h>


//***********************************************************************************************
// FCWindow
//***********************************************************************************************

//Static class data
FCWindowCommon_t FCWindow::static_data;

FCWindow::FCWindow(int x, int y, std::string title, bool fullscreen)
{
	//Init window size
	glutInitWindowSize(x, y);
	this->x = x; this->y = y;
	//Init quit signal
	this->quit = false;

	//Store handle in member and create window
  	this->handle = glutCreateWindow(title.c_str());
	//Set background color
  	glClearColor(0.0, 0.0, 0.0, 1.0);

	//Set static callbacks
	glutDisplayFunc(FCWindow::display);
	glutReshapeFunc(FCWindow::reshape);
	glutKeyboardFunc(FCWindow::Keyboard);

	//Sleep duration for decreasing processor usage
	FCWindow::static_data.sleep = 200;
	//Set fullscreen option
	FCWindow::static_data.fullscreen = fullscreen;
	
	//Set screen resolution
	FCWindow::static_data.res_x = glutGet(GLUT_WINDOW_WIDTH);
	FCWindow::static_data.res_y = glutGet(GLUT_WINDOW_HEIGHT);
}

FCWindow::~FCWindow()
{

}

int FCWindow::get_width()
{
	return this->x;
}

int FCWindow::get_height()
{
	return this->y;
}

int FCWindow::get_res_x()
{
	return FCWindow::static_data.res_x;
}

int FCWindow::get_res_y()
{
	return FCWindow::static_data.res_y;
}

bool FCWindow::get_fullscreen()
{
	return FCWindow::static_data.fullscreen;
}

bool FCWindow::get_quit()
{
	return this->quit;
}

void FCWindow::init(int argc, char** argv)
{
	//Init GLUT and save static data
	FCWindow::static_data.mtx.lock();
	FCWindow::static_data.argc = argc;
	FCWindow::static_data.argv = argv;
	glutInit(&FCWindow::static_data.argc, FCWindow::static_data.argv);
	FCWindow::static_data.mtx.unlock();

	//Set options and display mode
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
  	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
}

void FCWindow::start()
{
	//Start has been called - execute event loop
	FCWindow::static_data.mtx.lock();

	//We only launch one thread for the GLUT event loop
	if (FCWindow::static_data.running == false)
	{
		FCWindow::static_data.ftr = std::async(std::launch::async, glutMainLoop);
		FCWindow::static_data.running = true;
	}

	//Is fullscreen selected?
	if (FCWindow::static_data.fullscreen == true)
	{
		glutFullScreen();
		//Update resolution
		FCWindow::static_data.res_x = glutGet(GLUT_SCREEN_WIDTH);
		FCWindow::static_data.res_y = glutGet(GLUT_SCREEN_HEIGHT);
	}
	
	FCWindow::static_data.mtx.unlock();
}

void FCWindow::stop()
{
	//Stop has been called - stop event loop if not already stopped
	//This 'if' is necessary, because if a window is closed (by clicking 'x')
	//the underlying event loop is discontinued and in this case, we
	//shall not call the 'leave main loop' function
	FCWindow::static_data.mtx.lock();
	if (!(FCWindow::static_data.ftr.wait_for(std::chrono::seconds(0)) == std::future_status::ready))
		glutLeaveMainLoop();
	FCWindow::static_data.mtx.unlock();
}

void FCWindow::display(void)
{
	//Static display callback
	//Select current instance using static map
	int current_handle = glutGetWindow();
	FCWindow::static_data.mtx.lock();
	FCWindow* current_instance = FCWindow::static_data.windows.find(current_handle)->second;
	FCWindow::static_data.mtx.unlock();
	current_instance->display_();
	//Wait for a short amount of time - decreases processor usage
	std::this_thread::sleep_for(std::chrono::milliseconds(FCWindow::static_data.sleep));
}

void FCWindow::reshape(int width, int height)
{
	//Static reshape callback
	//Select current instance using static map
	int current_handle = glutGetWindow();
	FCWindow::static_data.mtx.lock();
	FCWindow* current_instance = FCWindow::static_data.windows.find(current_handle)->second;
	FCWindow::static_data.mtx.unlock();
  	current_instance->reshape_(width, height);
}

void FCWindow::Keyboard(unsigned char key, int x, int y)
{
	//Static Keyboard callback
	//Select current instance using static map
	int current_handle = glutGetWindow();
	FCWindow::static_data.mtx.lock();
	FCWindow* current_instance = FCWindow::static_data.windows.find(current_handle)->second;
	FCWindow::static_data.mtx.unlock();
  	current_instance->Keyboard_(key, x, y);
}

void FCWindow::reshape_(int width, int height)
{
	//Specific member callback (called from static base 'reshape')
  	glViewport(0, 0, width, height);
	//Update window size
	this->x = width; this->y = height;
  	glMatrixMode(GL_PROJECTION);
  	glLoadIdentity();
  	gluOrtho2D(0, width, height, 0);
  	glMatrixMode(GL_MODELVIEW);
}

void FCWindow::Keyboard_(unsigned char key, int x, int y)
{
	//Key has been pressed
	switch (key)
	{	
		case 27:
			glutDestroyWindow(this->handle);
			this->quit = true;
		break;
	}
}
