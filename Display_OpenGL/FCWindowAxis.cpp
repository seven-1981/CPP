#include "FCWindowAxis.hpp"

#include <GL/freeglut.h>

const int X_AXIS_FONT_MARGIN = 15;
const int X_AXIS_TICK_WIDTH = 5;
const int Y_AXIS_FONT_MARGIN = 30;
const int Y_AXIS_TICK_WIDTH = 5;

FCWindowAxis::FCWindowAxis(FCWindowAxisType_e type, void* font, FCWindowAxisData_t& data)
{
	//Set font
	this->font = font;
	//Set graphic parameters
	this->type = type;
	this->data = data;
	
	//Calculate auxiliary parameters
	this->inc_data = (int)((this->data.max - this->data.min) / (double)(this->data.N - 1));
	this->inc_coord = (int)(this->data.span / (double)(this->data.N - 1));
}

FCWindowAxis::~FCWindowAxis()
{
	
}

void FCWindowAxis::draw()
{
	//Draw labels and ticks on axis
	if (this->type == AxisTypeX)
	{
		glLineWidth(1.0);
		for (int i = 0; i < this->data.N; i++)
		{
			output(this->data.x + i * this->inc_coord, this->data.y + X_AXIS_FONT_MARGIN, std::to_string((int)this->data.min + i * this->inc_data));
			glBegin(GL_LINES);
			glVertex3f(this->data.x + i * this->inc_coord, this->data.y, 0.0);
			glVertex3f(this->data.x + i * this->inc_coord, this->data.y + X_AXIS_TICK_WIDTH, 0.0);
			glEnd();	
		}
	}
	
	if (this->type == AxisTypeY)
	{
		glLineWidth(1.0);
		for (int i = 0; i < this->data.N; i++)
		{
			output(this->data.x - Y_AXIS_FONT_MARGIN, this->data.y - i * this->inc_coord, std::to_string((int)this->data.min + i * this->inc_data));
			glBegin(GL_LINES);
			glVertex3f(this->data.x 				   , this->data.y - i * this->inc_coord, 0.0);
			glVertex3f(this->data.x - Y_AXIS_TICK_WIDTH, this->data.y - i * this->inc_coord, 0.0);
			glEnd();	
		}
	}
}

void FCWindowAxis::output(int x, int y, std::string text)
{
  	int len, i;
  	glRasterPos2f(x, y);
  	len = text.length();
  	for (i = 0; i < len; i++) 
	{
    		glutBitmapCharacter(this->font, text.at(i));
  	}
}
