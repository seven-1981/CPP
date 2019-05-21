#ifndef _FC_WINDOWGRAPH
#define _FC_WINDOWGRAPH

#include "FCWindow.hpp"

#include <string>
#include <vector>
#include <mutex>

struct FCWindowGraphSize_t : public FCWindowSize_t
{
	int x_border = 50;
	int y_border_bottom = 50;
	int y_border_top = 100;

	double max_x_value = 100.0;
	double min_x_value = 0.0;
	double max_y_value = 50.0;
	double min_y_value = -20.0;
};

//Inherit from base data struct
struct FCWindowGraphData_t : public FCWindowData_t
{
	std::vector<double> values;
};

//Derived window classes
//Graph display
class FCWindowGraph : public FCWindow
{
public:
	~FCWindowGraph();

	void update(FCWindowData_t*);
	void set_param(FCWindowSize_t*);
	
	//FCWindowManager must be able to access FCWindowGraph
	friend class FCWindowManager;
	
protected:
	//Protected constructor (factory method)
	explicit FCWindowGraph(FCWindowParam_t&);

private:
	//Font used on this window
	void* font;
	//Mutex for data protection
	std::mutex graph_mutex;

	//Data for graph
	std::vector<std::vector<double>> data;
	//Flag indicates that params have been calculated
	bool params_calculated;
	//Flag indicates that data vector has been initialized
	bool data_initialized;
	
	//Adjustable graphic parameters
	int size;				//Number of x values
	int y_diff;				//Height of graph
	int x_diff;				//Width of graph
	int x_border;			//Distance window border - graph sides
	int y_border_bottom;	//Distance window border - graph bottom
	int y_border_top;		//Distance window border - graph top
	double max_x_value;		//Maximal x axis value
	double min_x_value;		//Minimal x axis value
	double max_y_value;		//Maximal y axis value
	double min_y_value;		//Minimal y axis value

	//Auxiliary parameters
	int y_max;				//Top y coordinate of graph
	int y_min;				//Bottom y coordinate of graph
	int x_max;				//Left x coordinate of graph
	int x_min;				//Right x coordinate of graph
	double x_inc;			//Increment on x axis
	double m;				//Gain for scale
	double n;				//Offset for scale

	//Helper functions for displaying text
	void output(int, int, std::string);
	//Helper function to display non graph content, axes
	//and calculate parameters
	void init();
	//Calculate auxiliary parameters
	void calc_param();
	//Draw axes
	void draw_axes();
	//Draw lines of one graph
	void draw_lines(std::vector<double>&);

	//Member callbacks - must be overridden by derived class
	void display_(void);
	void reshape_(int, int);
};

#endif
