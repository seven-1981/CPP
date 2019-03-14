#include "FCWindowLabel.hpp"

#include <GL/freeglut.h>

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

void FCWindowLabel::display_(void)
{
	//Specific member callback (called from static base 'display')
	glutSetWindow(this->handle);
  	glClear(GL_COLOR_BUFFER_BIT);
  	this->output(340, 50, "* * * OZON BPM COUNTER * * *");
	std::string s = "BPM VALUE: " + this->data;
	this->output(100.0f, 200.0f, s.c_str());
  	glutSwapBuffers();
	glutPostRedisplay();
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

