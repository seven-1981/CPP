#ifndef _FC_WINDOWSPECTRUM_H
#define _FC_WINDOWSPECTRUM_H

#include "FCWindow.hpp"
#include "Timing.hpp"

#include <string>
#include <mutex>
#include <vector>

struct FCWindowSpectrumSize_t
{
	int x_border = 50;
	int y_border_bottom = 50;
	int y_border_top = 100;
	
	double bar_width_frac = 0.5;

	double max_value = 100.0;
	double min_value = 0.0;
};

//Inherit from base data struct
struct FCWindowSpectrumData_t : public FCWindowData_t
{
	std::vector<double> values;
};

//Frequency spectrum display
class FCWindowSpectrum : public FCWindow
{
public:
	~FCWindowSpectrum();

	void update(FCWindowData_t*);
	void set_param(FCWindowSpectrumSize_t&);
	
	//FCWindowManager must be able to access FCWindowSpectrum
	friend class FCWindowManager;
	
protected:
	//Protected constructor (factory method)
	explicit FCWindowSpectrum(FCWindowParam_t&);

private:
	//Font used on this window
	void* font;
	//Mutex for data protection
	std::mutex spectrum_mutex;

	//Data for frequency array
	int size;
	FCWindowSpectrumData_t data;
	//Flag indicates that params have been calculated
	bool params_calculated;
	
	//Adjustable graphic parameters
	int x_border;			//Distance window border - spectrum sides
	int y_border_bottom;	//Distance window border - spectrum bottom
	int y_border_top;		//Distance window border - spectrum top
	double bar_width_frac;	//Fraction bar width / bar margin
	double max_value;		//Minimal y axis value
	double min_value;		//Maximal y axis value

	//Auxiliary parameters
	int max_n_bars;			//Maximum number of bars	
	int n_bars;				//Number of bars
	int x_size;				//Width of one bar
	int x_margin;			//Space between two bars
	int x_begin;			//First bar x coordinates
	int x_diff;				//Increment for next bar
	int y_bar;				//Bottom bars coordinates
	int y_diff;				//Width of y axis
	double m;				//Gain for scale
	double n;				//Offset for scale

	//Helper function for displaying text
	void output(int, int, std::string);
	//Helper function to display non spectrum content, axes
	//and calculate parameters
	void init();
	//Calculate auxiliary parameters
	void calc_param();
	//Draw axes
	void draw_axes();
	
	//Member callbacks - must be overridden by derived class
	void display_(void);
	void reshape_(int, int);
	
	//Timer object
	Timing tim;
};

#endif
