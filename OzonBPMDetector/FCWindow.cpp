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
		glutFullScreen();

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

//***********************************************************************************************
// FCWindowLabel
//***********************************************************************************************

FCWindowLabel::FCWindowLabel(int x, int y, std::string title, bool fullscreen)
 : FCWindow(x, y, title, fullscreen)
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

bool FCWindowLabel::get_quit()
{
	return this->quit;
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

void FCWindowLabel::Keyboard_(unsigned char key, int x, int y)
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

FCWindowSpectrum::FCWindowSpectrum(int x, int y, std::string title, int size, bool fullscreen)
 : FCWindow(x, y, title, fullscreen)
{
	//Initialize derived type specific data
	this->font = GLUT_BITMAP_TIMES_ROMAN_24;
	this->size = size;
	this->data = new double[size];

	//Graphic default parameters
	this->x_border = 50;
	this->y_border_bottom = 50;
	this->y_border_top = 100;
	this->x_margin = 5;
	this->max_value = 100.0;
	this->min_value = 0.0;
	this->n_ticks = 11;
	this->tick_width = 5;

	//Set quit signal
	this->quit = false;
	
	//Update static map (handle and instance)
	FCWindow::static_data.mtx.lock();
	FCWindow::static_data.windows.insert(std::pair<int, FCWindowSpectrum*>(this->handle, this));
	FCWindow::static_data.mtx.unlock();

	//Draw static content
	this->init();	
}

FCWindowSpectrum::~FCWindowSpectrum()
{
	//Delete data array
	delete[] this->data;
}

void FCWindowSpectrum::update(FCWindowData_t& data)
{
	for (int i = 0; i < data.size; i++)
		this->data[i] = data.double_array_data[i];
}

bool FCWindowSpectrum::get_quit()
{
	return this->quit;
}

void FCWindowSpectrum::display_(void)
{
	//Specific member callback (called from static base 'display')
	glutSetWindow(this->handle);

	//Specify window for clearance
	glScissor(this->x_border, this->y_border_bottom, this->x - 2 * this->x_border, y_diff);
	glEnable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glScissor(0, 0, FCWindow::static_data.res_x, FCWindow::static_data.res_y);
	glDisable(GL_SCISSOR_TEST);

  	//Display spectrum
	for (int i = 0; i < this->size; i++)
	{
		//Get data from array and calculate height
		int x_bar = this->x_begin + i * this->x_diff;
		int x_bar2 = x_bar + this->x_size;

		double lim_val = std::max(this->data[i], this->min_value);
		lim_val = std::min(lim_val, this->max_value);
		int y_bar2 = y_bar - (int)(lim_val * m + n);

		//Draw polygon
		glBegin(GL_POLYGON);
		glVertex3f(x_bar, y_bar, 0.0);
		glVertex3f(x_bar2, y_bar, 0.0);
		glVertex3f(x_bar2, y_bar2, 0.0);
		glVertex3f(x_bar, y_bar2, 0.0);
		glEnd();

		//Draw labels on x axis
		this->font = GLUT_BITMAP_TIMES_ROMAN_10;
		if (i % 2 == 0)
			output(x_bar, y_bar + 20, std::to_string(i * 43));
	}

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
	//Redraw static content
	this->init();
}

void FCWindowSpectrum::Keyboard_(unsigned char key, int x, int y)
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

void FCWindowSpectrum::init()
{
	//Display title
	this->font = GLUT_BITMAP_TIMES_ROMAN_24;
	this->output(50, 50, "* * * AUDIO SPECTROGRAM * * *");

	//Calculate auxiliary parameters
	this->x_size = (this->x - 2 * this->x_border - ((this->size - 1) * this->x_margin)) / this->size;
	this->x_begin = this->x_border;
	this->x_diff =this->x_size + this->x_margin;
	this->y_bar = this->y - this->y_border_bottom;
	this->y_diff = this->y - this->y_border_bottom - this->y_border_top;
	this->m = (double)this->y_diff / (double)(this->max_value - this->min_value);
	this->n = -this->m * (double)this->min_value;

	//Draw scale on y axis
	glLineWidth(1.0);
	int tick = this->y_diff / (this->n_ticks - 1);
	for (int i = 0; i < this->n_ticks; i++)
	{
		glBegin(GL_LINES);
		glVertex3f(this->x_begin - this->tick_width, this->y_bar - i * tick, 0.0);
		glVertex3f(this->x_begin,                    this->y_bar - i * tick, 0.0);
		glEnd();
	}

	//Draw labels on y axis
	this->font = GLUT_BITMAP_TIMES_ROMAN_10;
	double inc = (this->max_value - this->min_value) / (double)(this->n_ticks - 1);
	for (int i = 0; i < this->n_ticks; i++)
	{
		output(this->x_begin - this->tick_width - 20, this->y_bar - i * tick, std::to_string((int)(this->min_value + i * inc)));
	}
}

void FCWindowSpectrum::set_param(FCWindowSpectrumSize_t& param)
{
	this->x_border	      = param.x_border;
	this->y_border_bottom = param.y_border_bottom;
	this->y_border_top    = param.y_border_top;
	this->x_margin 	      = param.x_margin;

	this->max_value       = param.max_value;
	this->min_value       = param.min_value;

	this->n_ticks	      = param.n_ticks;
	this->tick_width      = param.tick_width;
}
