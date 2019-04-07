#include "FCWindow.hpp"

#include <GL/freeglut.h>


//***********************************************************************************************
// FCWindow
//***********************************************************************************************

FCWindow::FCWindow(FCWindowParam_t& param)
{
	//Init window parameters
	glutInitWindowSize(param.x, param.y);
	this->x = param.x; this->y = param.y;
	this->fullscreen = param.fullscreen;
	
	//Init quit signal
	this->quit = false;

	//Store handle in member and create window
  	this->handle = glutCreateWindow(param.title.c_str());
	//Set background color
  	glClearColor(0.0, 0.0, 0.0, 1.0);

	//Set static callbacks
	glutDisplayFunc(FCWindowManager::display);
	glutReshapeFunc(FCWindowManager::reshape);
	glutKeyboardFunc(FCWindowManager::Keyboard);
	glutCloseFunc(FCWindowManager::close);
	glutIdleFunc(FCWindowManager::idle);
	
	//Is fullscreen selected?
	if (this->fullscreen == true)
		glutFullScreen();
		
	//Set keyboard callback
	this->pKeyboard = nullptr;
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

void FCWindow::set_keyboard_callback(void (*pCallback)(unsigned char, int, int))
{
	if (pCallback != nullptr)
		this->pKeyboard = pCallback;
}

bool FCWindow::get_quit()
{
	return this->quit;
}

int FCWindow::get_handle()
{
	return this->handle;
}

bool FCWindow::get_fullscreen()
{
	return this->fullscreen;
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
	
	//Callback, if set
	if (this->pKeyboard != nullptr)
		this->pKeyboard(key, x, y);
}
