#include "FCWindowSpectrum.hpp"

#include <GL/freeglut.h>

//***********************************************************************************************
// FCWindowSpectrum
//***********************************************************************************************

FCWindowSpectrum::FCWindowSpectrum(int x, int y, std::string title, int size, bool fullscreen)
 : FCWindow(x, y, title, fullscreen)
{
	//Initialize derived type specific data
	this->font = GLUT_BITMAP_TIMES_ROMAN_24;
	this->size = size;

	//Graphic default parameters
	this->x_border = 50;
	this->y_border_bottom = 50;
	this->y_border_top = 100;
	this->x_margin = 5;
	this->max_value = 100.0;
	this->min_value = 0.0;
	this->n_ticks = 11;
	this->tick_width = 5;
	
	//Update static map (handle and instance)
	FCWindow::static_data.mtx.lock();
	FCWindow::static_data.windows.insert(std::pair<int, FCWindowSpectrum*>(this->handle, this));
	FCWindow::static_data.mtx.unlock();

	//Draw static content
	this->init();	
}

FCWindowSpectrum::~FCWindowSpectrum()
{

}

void FCWindowSpectrum::update(FCWindowData_t* data)
{	
	//First, cast pointer to derived class
	FCWindowSpectrumData_t* pWindowData = dynamic_cast<FCWindowSpectrumData_t*>(data);
	//Protect data
	this->spectrum_mutex.lock();
	for (int i = 0; i < this->size; i++)
		this->data.values[i] = pWindowData->values[i];
	this->spectrum_mutex.unlock();	
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
  	//Protect data
  	this->spectrum_mutex.lock();
	for (int i = 0; i < this->size; i++)
	{
		//Get data from array and calculate height
		int x_bar = this->x_begin + i * this->x_diff;
		int x_bar2 = x_bar + this->x_size;

		double lim_val = std::max(this->data.values[i], this->min_value);
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
	this->spectrum_mutex.unlock();

  	glutSwapBuffers();
	glutPostRedisplay();
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
