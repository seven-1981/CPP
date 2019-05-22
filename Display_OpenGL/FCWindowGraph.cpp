#include "FCWindowGraph.hpp"
#include "FCWindowAxis.hpp"
#include "FCWindowColor.hpp"

#include <GL/freeglut.h>
#include <cmath>

//***********************************************************************************************
// FCWindowGraph
//***********************************************************************************************

FCWindowGraph::FCWindowGraph(FCWindowParam_t& param)
 : FCWindow(param)
{
	//Initialize derived type specific data
	this->font = GLUT_BITMAP_TIMES_ROMAN_24;
	this->size = param.size;
	this->params_calculated = false;
	this->data_initialized = false;

	//Adjustable graphic parameters
	this->x_border = 50;
	this->y_border_bottom = 50;
	this->y_border_top = 100;
	this->max_x_value = 100.0;
	this->min_x_value = 0.0;
	this->max_y_value = 50.0;
	this->min_y_value = -20.0;

	//Draw static content and calculate parameters
	this->init();	
}

FCWindowGraph::~FCWindowGraph()
{

}

void FCWindowGraph::update(FCWindowData_t* data)
{
	//First, cast pointer to derived class
	FCWindowGraphData_t* pWindowData = dynamic_cast<FCWindowGraphData_t*>(data);
	//Get size of data
	int size = pWindowData->values.size();
	
	//Protect data
	this->graph_mutex.lock();
	
	//Check if data has already been initialized
	if (this->data_initialized == false)
	{
		//Initialize data member with init vectors
		this->init_data(size);
	}	
	//Update data
	this->update_data(size, pWindowData);
		
	this->graph_mutex.unlock();
}

void FCWindowGraph::display_(void)
{
	//Check flag
	if (this->params_calculated == false)
		return;
		
	//Specific member callback (called from static base 'display')
	glutSetWindow(this->get_handle());

	//Protect data
  	this->graph_mutex.lock();
  	//Specify window for clearance
	glScissor(this->x_border, this->y_border_bottom, this->get_width() - 2 * this->x_border, this->y_diff);
	glEnable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glScissor(0, 0, FCWindowManager::get_res_x(), FCWindowManager::get_res_y());
	glDisable(GL_SCISSOR_TEST);
	glLineWidth(1.0);

	//Set some color to graph
	FCWindowColor_e color[3];
	color[0] = FCWindowColor_e::ColorGreen;
	color[1] = FCWindowColor_e::ColorRed;
	color[2] = FCWindowColor_e::ColorBlue;

	for (int i = 0; i < this->data.size(); ++i)
	{
		FCWindowColor::set(color[i]);
		draw_lines(this->data.at(i));
	}
	
	//Draw axes
	FCWindowColor::set(FCWindowColor_e::ColorWhite);
	this->draw_axes();
	
	this->graph_mutex.unlock();

  	glutSwapBuffers();
	glutPostRedisplay();
}

void FCWindowGraph::reshape_(int width, int height)
{
	//Specific member callback (called from static base 'reshape')
	//Overridden, because we additionally need to call init()
  	FCWindow::reshape_(width, height);
  	//Init static content
  	this->init();
}

void FCWindowGraph::output(int x, int y, std::string text)
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

void FCWindowGraph::init()
{
	//Display title
	this->font = GLUT_BITMAP_TIMES_ROMAN_24;
	this->output(50, 50, "* * * TEMPERATURE GRAPH * * *");

	//Calculate auxiliary parameters
	this->calc_param();
	
	//Draw axes
	this->draw_axes();
}

void FCWindowGraph::calc_param()
{
	//Reset flag
	this->params_calculated = false;
	
	//Calculate auxiliary parameters
	this->y_diff = this->get_height() - this->y_border_top - this->y_border_bottom;
	this->x_diff = this->get_width() - 2 * this->x_border;
	this->x_inc = this->x_diff / (double)(this->size - 1);
	this->y_max = this->y_border_top;
	this->y_min = this->get_height() - this->y_border_bottom;
	this->x_min = this->x_border;
	this->x_max = this->get_width() - this->x_border;
	this->m = this->y_diff / (double)(this->max_y_value - this->min_y_value);
	this->n = -this->m * (double)this->min_y_value;
	
	//Calculation finished, set flag
	this->params_calculated = true;
}

void FCWindowGraph::draw_axes(void)
{
	//Draw x axis
	FCWindowAxisData_t axis;
	int y_zero = this->y_min - this->n;
	axis.x = this->x_min; axis.y = y_zero; //this->y_min;
	axis.min = this->min_x_value; axis.max = this->max_x_value;
	axis.N = 11;
	axis.span = this->get_width() - 2 * this->x_border;
	FCWindowAxis x_axis(AxisTypeX, GLUT_BITMAP_TIMES_ROMAN_10, axis);
	x_axis.draw();
	
	//Draw y axis
	axis.x = this->x_min; axis.y = this->y_min;
	axis.min = this->min_y_value; axis.max = this->max_y_value;
	axis.N = 8;
	axis.span = this->get_height() - this->y_border_top - this->y_border_bottom;
	FCWindowAxis y_axis(AxisTypeY, GLUT_BITMAP_TIMES_ROMAN_10, axis);
	y_axis.draw();
}

void FCWindowGraph::draw_lines(std::vector<double>& data)
{
  	//Display graph - number of lines is number of points - 1
  	int num_values = data.size() - 1;
	for (int i = 0; i < num_values; ++i)
	{
		//Get data from array and calculate coordinates
		int x_left = (int)(this->x_min + i * this->x_inc);
		int x_right = x_left + std::ceil(this->x_inc);

		double lim_val_left = std::max(data.at(i), this->min_y_value);
		double lim_val_right = std::max(data.at(i + 1), this->min_y_value);
		lim_val_left = std::min(lim_val_left, this->max_y_value);
		lim_val_right = std::min(lim_val_right, this->max_y_value);
		int y_val_left = this->y_min - (int)(lim_val_left * m + n);
		int y_val_right = this->y_min - (int)(lim_val_right * m + n);

		//Draw line
		glBegin(GL_LINES);
		glVertex3f(x_left, y_val_left, 0.0);
		glVertex3f(x_right, y_val_right, 0.0);
		glEnd();
	}	
}

void FCWindowGraph::init_data(int size)
{
	//Initialize vector with data vectors
	this->data_initialized = true;
	std::vector<double> init;
	for (int i = 0; i < size; ++i)
	{
		this->data.push_back(init);
	}	
}

void FCWindowGraph::update_data(int size, FCWindowGraphData_t* pData)
{
	//We got a few new values inside pData
	//Add to existing data, if vectors are full, shift values
	for (int i = 0; i < size; ++i)
	{
		int data_size = this->data.at(i).size();
		if (data_size >= this->size)
		{
			for (int j = 0; j < data_size - 1; ++j)
			{
				this->data.at(i).at(j) = this->data.at(i).at(j + 1);
			}	
			this->data.at(i).at(data_size - 1) = pData->values.at(i);	
		}
		else
		{
			this->data.at(i).push_back(pData->values.at(i));
		}
	}	
}

void FCWindowGraph::set_param(FCWindowSize_t* param)
{
	//First, cast pointer to derived class
	FCWindowGraphSize_t* pParam = dynamic_cast<FCWindowGraphSize_t*>(param);
		
	//Protect data
	this->x_border	      = pParam->x_border;
	this->y_border_bottom = pParam->y_border_bottom;
	this->y_border_top    = pParam->y_border_top;

	this->max_x_value     = pParam->max_x_value;
	this->min_x_value     = pParam->min_x_value;
	this->max_y_value     = pParam->max_y_value;
	this->min_y_value     = pParam->min_y_value;

	//Redraw static content
	glClear(GL_COLOR_BUFFER_BIT);
	this->init();
}
