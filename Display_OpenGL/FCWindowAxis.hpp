#ifndef _FC_WINDOWAXIS_H
#define _FC_WINDOWAXIS_H

#include <string>

enum FCWindowAxisType_e
{
	AxisTypeX,
	AxisTypeY
};

struct FCWindowAxisData_t
{
	int x, y;
	int N;
	double min, max;
	int span;
};

class FCWindowAxis
{
public:
	explicit FCWindowAxis(FCWindowAxisType_e, void*, FCWindowAxisData_t&);
	~FCWindowAxis();
	
	//Draw axis method
	void draw();
	
private:
	//Font used for axes
	void* font;
	//Axis parameters
	FCWindowAxisType_e type;
	FCWindowAxisData_t data;
	
	//Auxiliary parameters
	int inc_data;
	int inc_coord;
	
	//Draw string
	void output(int, int, std::string);
};

#endif
