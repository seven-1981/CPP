#include "FCWindowLabel.hpp"

#include <GL/freeglut.h>

//***********************************************************************************************
// FCWindowLabel
//***********************************************************************************************

FCWindowLabel::FCWindowLabel(FCWindowParam_t& param)
 : FCWindow(param)
{
	//Initialize derived type specific data
	this->font = GLUT_BITMAP_TIMES_ROMAN_24;
	this->text_items = { };	
}

FCWindowLabel::~FCWindowLabel()
{

}

void FCWindowLabel::update(FCWindowData_t* data)
{
	//First, cast pointer to derived class
	FCWindowLabelData_t* pWindowData = dynamic_cast<FCWindowLabelData_t*>(data);
	//Protect data
	this->label_mutex.lock();
	this->text_items.items = pWindowData->items;
	this->label_mutex.unlock();
}

void FCWindowLabel::display_(void)
{
	//Specific member callback (called from static base 'display')
	glutSetWindow(this->get_handle());
  	glClear(GL_COLOR_BUFFER_BIT);
	//Protect data
	this->label_mutex.lock();
	for (FCWindowLabelDataItem_t& item : this->text_items.items)
	{
		glColor3f(item.color_R, item.color_G, item.color_B);
		if (item.use_stroke == true)
			this->output((float)item.x, (float)item.y, item.text);
		else
			this->output(item.x, item.y, item.text);
	}
	this->label_mutex.unlock();
  	glutSwapBuffers();
	glutPostRedisplay();
}

void FCWindowLabel::output(float x, float y, std::string text)
{
	glutSetWindow(this->get_handle());
	glLineWidth(4.0f);
	glEnable(GL_LINE_SMOOTH);
    	glPushMatrix();
    	glTranslatef(x, y, 0);
	glScalef(0.5f, -0.3f, 1.0f);
	int len = text.length();
    	for(int i = 0; i < len; i++)
    	{
    	    glutStrokeCharacter(GLUT_STROKE_ROMAN, text.at(i));
    	}
    	glPopMatrix();

}

void FCWindowLabel::output(int x, int y, std::string text)
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
