#include "FCWindowSpectrum.hpp"
#include "FCWindowAxis.hpp"

#include <GL/freeglut.h>
#include <cmath>
#include <iostream>

//Graphic drawing constants
const int SPECTRUM_SIZE_X_MIN = 300;
const int SPECTRUM_SIZE_Y_MIN = 150;

//Cycle time measurement
//#define ENABLE_CYCLE_TIME_MEASUREMENT

//***********************************************************************************************
// FCWindowSpectrum
//***********************************************************************************************

FCWindowSpectrum::FCWindowSpectrum(FCWindowParam_t& param)
 : FCWindow(param)
{
	//Initialize derived type specific data
	this->font = GLUT_BITMAP_TIMES_ROMAN_24;
	this->size = param.size;
	this->params_calculated = false;
	//Initialize vector
	for (int i = 0; i < this->size; i++)
		this->data.values.push_back(0);

	//Adjustable graphic parameters
	this->x_border = 50;
	this->y_border_bottom = 50;
	this->y_border_top = 100;
	this->bar_width_frac = 0.5;
	this->max_value = 100.0;
	this->min_value = 0.0;

	//Draw static content and calculate parameters
	this->init();	
	
	//Initialize timer object
	this->tim.init();
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
		this->data.values.at(i) = pWindowData->values.at(i);
	this->spectrum_mutex.unlock();
}

void FCWindowSpectrum::display_(void)
{
	//Check flag
	if (this->params_calculated == false)
		return;
		
	//Specific member callback (called from static base 'display')
	glutSetWindow(this->get_handle());

	//Specify window for clearance
	//Protect data
  	this->spectrum_mutex.lock();
	glScissor(this->x_border, this->y_border_bottom, this->get_width() - 2 * this->x_border, this->y_diff);
	glEnable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glScissor(0, 0, FCWindowManager::get_res_x(), FCWindowManager::get_res_y());
	glDisable(GL_SCISSOR_TEST);
	
  	//Display spectrum
	for (int i = 0; i < this->size; i++)
	{
		//Get data from array and calculate height
		int x_bar = this->x_begin + i * this->x_diff;
		int x_bar2 = x_bar + this->x_size;

		double lim_val = std::max(this->data.values.at(i), this->min_value);
		lim_val = std::min(lim_val, this->max_value);
		int y_bar2 = y_bar - (int)(lim_val * m + n);

		//Draw polygon
		glBegin(GL_POLYGON);
		glVertex3f(x_bar, y_bar, 0.0);
		glVertex3f(x_bar2, y_bar, 0.0);
		glVertex3f(x_bar2, y_bar2, 0.0);
		glVertex3f(x_bar, y_bar2, 0.0);
		glEnd();
	}
	
	this->spectrum_mutex.unlock();
	
	//Display timing info
	#ifdef ENABLE_CYCLE_TIME_MEASUREMENT
		long long t = this->tim.get_time_us();
		this->font = GLUT_BITMAP_HELVETICA_12;
		this->output(350, 120, std::to_string(t));
	#endif

  	glutSwapBuffers();
	glutPostRedisplay();
}

void FCWindowSpectrum::reshape_(int width, int height)
{
	//Specific member callback (called from static base 'reshape')
	//Overridden, because we additionally need to call init()
  	FCWindow::reshape_(width, height);
  	//Init static content
  	this->init();
}

void FCWindowSpectrum::output(int x, int y, std::string text)
{
	glutSetWindow(this->get_handle());
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
	this->output(50, 50, "* * * AUDIO SPECTRUM * * *");

	//Calculate auxiliary parameters
	this->calc_param();
	
	//Draw axes
	this->draw_axes();
}

void FCWindowSpectrum::calc_param()
{
	//Reset flag
	this->params_calculated = false;
	
	//Calculate auxiliary parameters
	//Check if desired parameters fit into window
	int x_width = this->get_width() - 2 * this->x_border;
	int y_width = this->get_height() - this->y_border_bottom - this->y_border_top;
	if (x_width < SPECTRUM_SIZE_X_MIN)
		return;
	if (y_width < SPECTRUM_SIZE_Y_MIN)
		return;
	
	std::cout << "x_width = " << x_width << std::endl;
	std::cout << "y_width = " << y_width << std::endl;
	
	//Determine max number of bars
	this->max_n_bars = (int)std::ceil((double)x_width / 2);
	std::cout << "max_n_bars = " << max_n_bars << std::endl;
	
	//Determine number of bars
	if (this->size > this->max_n_bars)
		this->n_bars = this->max_n_bars;
	else
		this->n_bars = this->size;
	std::cout << "n_bars = " << this->n_bars << std::endl;
	
	//Calculate margin and size of one spectrum bar
	int bar_margin = x_width / this->n_bars;
	std::cout << "bar+margin = " << bar_margin << std::endl;

	//this->x_size = (this->x - 2 * this->x_border - ((this->size - 1) * this->x_margin)) / this->size;
	this->x_size = (int)((double)bar_margin * this->bar_width_frac);
	this->x_margin = bar_margin - this->x_size;
	std::cout << "x_size = " << this->x_size << std::endl;
	std::cout << "x_margin = " << this->x_margin << std::endl;
	
	//Calculate coordinates for bars
	this->x_begin = this->x_border;
	this->x_diff =this->x_size + this->x_margin;
	this->y_bar = this->get_height() - this->y_border_bottom;
	this->y_diff = this->get_height() - this->y_border_bottom - this->y_border_top;
	this->m = (double)this->y_diff / (double)(this->max_value - this->min_value);
	this->n = -this->m * (double)this->min_value;
	std::cout << "x_begin = " << this->x_begin << std::endl;
	std::cout << "x_diff = " << this->x_diff << std::endl;
	std::cout << "y_bar = " << this->y_bar << std::endl;
	std::cout << "y_diff = " << this->y_diff << std::endl;
	std::cout << "m = " << this->m << std::endl;
	std::cout << "n = " << this->n << std::endl;
	
	//Calculation finished, set flag
	this->params_calculated = true;
}

void FCWindowSpectrum::draw_axes(void)
{
	//Draw x axis
	FCWindowAxisData_t axis;
	axis.x = this->x_begin; axis.y = this->y_bar;
	axis.min = 0.0; axis.max = 2000.0;
	axis.N = 11;
	axis.span = this->get_width() - 2 * this->x_border;
	FCWindowAxis x_axis(AxisTypeX, GLUT_BITMAP_TIMES_ROMAN_10, axis);
	x_axis.draw();
	
	//Draw y axis
	axis.x = this->x_begin; axis.y = this->y_bar;
	axis.min = -100.0; axis.max = 0.0;
	axis.N = 11;
	axis.span = this->get_height() - this->y_border_top - this->y_border_bottom;
	FCWindowAxis y_axis(AxisTypeY, GLUT_BITMAP_TIMES_ROMAN_10, axis);
	y_axis.draw();
}

void FCWindowSpectrum::set_param(FCWindowSpectrumSize_t& param)
{
	this->x_border	      = param.x_border;
	this->y_border_bottom = param.y_border_bottom;
	this->y_border_top    = param.y_border_top;
	
	this->bar_width_frac  = param.bar_width_frac;

	this->max_value       = param.max_value;
	this->min_value       = param.min_value;

	//Redraw static content
	glClear(GL_COLOR_BUFFER_BIT);
	this->init();
}
