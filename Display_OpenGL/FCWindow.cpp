#include "FCWindow.hpp"

#include <GL/freeglut.h>

//***********************************************************************************************
// FCWindow
//***********************************************************************************************

//Static class data
FCWindowCommon_t FCWindow::static_data;

FCWindow::FCWindow(int x, int y, std::string title)
{
	//Init window size
	glutInitWindowSize(x, y);
	this->x = x; this->y = y;

	//Store handle in member and create window
  	this->handle = glutCreateWindow(title.c_str());
	//Set background color
  	glClearColor(0.0, 0.0, 0.0, 1.0);

	//Set static callbacks
	glutDisplayFunc(FCWindow::display);
	glutReshapeFunc(FCWindow::reshape);

	//Sleep duration for decreasing processor usage
	FCWindow::static_data.sleep = 10;
}

FCWindow::~FCWindow()
{

}

void FCWindow::start(int argc, char** argv)
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

void FCWindow::start(bool fullscreen = false)
{
	//Start has been called - execute event loop
	FCWindow::static_data.mtx.lock();

	//We only launch one thread for the GLUT event loop
	if (FCWindow::static_data.running == false)
	{
		FCWindow::static_data.ftr = std::async(std::launch::async, glutMainLoop);
		FCWindow::static_data.running = true;
	}

	FCWindow::static_data.mtx.unlock();

	//Fullscreen is selected
	if (fullscreen == true)
		glutFullScreen();
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

//***********************************************************************************************
// FCWindowLabel
//***********************************************************************************************

FCWindowLabel::FCWindowLabel(int x, int y, std::string title)
 : FCWindow(x, y, title)
{
	//Initialize derived type specific data
	this->font = GLUT_BITMAP_TIMES_ROMAN_24;
	this->data = "";
	
	//Update static map (handle and instance)
	FCWindow::static_data.mtx.lock();
	FCWindow::static_data.windows.insert(std::pair<int, FCWindowLabel*>(this->handle, this));
	FCWindow::static_data.mtx.unlock();	
}

FCWindowLabel::~FCWindowLabel()
{

}

void FCWindowLabel::update(FCWindowData_t& data)
{
	this->data = std::string(data.string_data);
}

void FCWindowLabel::display_(void)
{
	//Specific member callback (called from static base 'display')
	glutSetWindow(this->handle);
  	glClear(GL_COLOR_BUFFER_BIT);
  	this->output(200, 50, "* * * OZON BPM COUNTER * * *");
	std::string s = "BPM VALUE: " + this->data;
	this->output(100.0f, 200.0f, s.c_str());
  	glutSwapBuffers();
	glutPostRedisplay();
}

void FCWindowLabel::reshape_(int width, int height)
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

void FCWindowLabel::output(float x, float y, std::string text)
{
	glutSetWindow(this->handle);
	glLineWidth(5.0f);
	glEnable(GL_LINE_SMOOTH);
    	glPushMatrix();
    	glTranslatef(x, y, 0);
	glScalef(0.5f, -0.5f, 1.0f);
    	int len = text.length();
    	for(int i = 0; i < len; i++)
    	{
    	    glutStrokeCharacter(GLUT_STROKE_ROMAN, text.at(i));
    	}
    	glPopMatrix();

}

void FCWindowLabel::output(int x, int y, std::string text)
{
	glutSetWindow(this->handle);
  	int len, i;
  	glRasterPos2f(x, y);
  	len = text.length();
  	for (i = 0; i < len; i++) 
	{
    		glutBitmapCharacter(this->font, text.at(i));
  	}
}

//***********************************************************************************************
// FCWindowSpectrum
//***********************************************************************************************

FCWindowSpectrum::FCWindowSpectrum(int x, int y, std::string title, int size)
 : FCWindow(x, y, title)
{
	//Initialize derived type specific data
	this->font = GLUT_BITMAP_TIMES_ROMAN_24;
	this->size = size;
	this->data = new double[size];

	//Graphic parameters
	this->x_border = 50;
	this->y_border_bottom = 50;
	this->y_border_top = 100;
	this->x_margin = 5;
	this->max_value = 100.0;
	this->min_value = 0.0;
	
	//Update static map (handle and instance)
	FCWindow::static_data.mtx.lock();
	FCWindow::static_data.windows.insert(std::pair<int, FCWindowSpectrum*>(this->handle, this));
	FCWindow::static_data.mtx.unlock();	
}

FCWindowSpectrum::~FCWindowSpectrum()
{
	//Delete data array
	this->data = nullptr;
	delete[] this->data;
}

void FCWindowSpectrum::update(FCWindowData_t& data)
{
	this->data = data.double_array_data;
}

void FCWindowSpectrum::display_(void)
{
	//Specific member callback (called from static base 'display')
	glutSetWindow(this->handle);
  	glClear(GL_COLOR_BUFFER_BIT);
	//Calculate auxiliary parameters
	int x_size = (this->x - 2 * this->x_border - ((this->size - 1) * this->x_margin)) / this->size;
	int x_begin = this->x_border;
	int x_diff = x_size + this->x_margin;
  	//Display spectrum
	for (int i = 0; i < this->size; i++)
	{
		//Calculate width coordinates
		int x_bar = x_begin + i * x_diff;
		int x_bar2 = x_bar + x_size;
		//Get data from array and calculate height
		int y_bar = this->y - this->y_border_bottom;
		int y_diff = this->y - this->y_border_bottom - this->y_border_top;
		double m = (double)y_diff / (double)(this->max_value - this->min_value);
		double n = (double)-m * (double)this->min_value;
		int y_bar2 = y_bar - (int)(this->data[i] * m + n);
		//Draw polygon
		glBegin(GL_POLYGON);
		glVertex3f(x_bar, y_bar, 0.0);
		glVertex3f(x_bar2, y_bar, 0.0);
		glVertex3f(x_bar2, y_bar2, 0.0);
		glVertex3f(x_bar, y_bar2, 0.0);
		glEnd();
	}
	glFlush();
	//Display title
	this->output(50, 50, "* * * AUDIO SPECTROGRAM * * *");
  	glutSwapBuffers();
	glutPostRedisplay();
}

void FCWindowSpectrum::reshape_(int width, int height)
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

void FCWindowSpectrum::output(int x, int y, std::string text)
{
	glutSetWindow(this->handle);
  	int len, i;
  	glRasterPos2f(x, y);
  	len = text.length();
  	for (i = 0; i < len; i++) 
	{
    		glutBitmapCharacter(this->font, text.at(i));
  	}
}
