#ifndef _FC_WINDOWSPECTRUM
#define _FC_WINDOWSPECTRUM

#include "FCWindow.hpp"

#include <string>
#include <mutex>

struct FCWindowSpectrumSize_t
{
	int x_border = 50;
	int y_border_bottom = 50;
	int y_border_top = 100;
	int x_margin = 5;

	double max_value = 100.0;
	double min_value = 0.0;

	int n_ticks = 11;
	int tick_width = 5;
};

//Inherit from base data struct
struct FCWindowSpectrumData_t : public FCWindowData_t
{
	double* values;
};

//Frequency spectrum display
class FCWindowSpectrum : public FCWindow
{
public:
	explicit FCWindowSpectrum(int, int, std::string, int, bool);
	~FCWindowSpectrum();

	void update(FCWindowData_t*);
	void set_param(FCWindowSpectrumSize_t&);

private:
	//Font used on this window
	void* font;
	//Mutex for data protection
	std::mutex spectrum_mutex;

	//Data for frequency array
	int size;
	FCWindowSpectrumData_t data;
	
	//Graphic parameters
	int x_border;
	int y_border_bottom;
	int y_border_top;
	int x_margin;
	double max_value;
	double min_value;
	int n_ticks;
	int tick_width;

	//Auxiliary parameters
	int x_size;
	int x_begin;
	int x_diff;
	int y_bar;
	int y_diff;
	double m;
	double n;

	//Helper function for displaying text
	void output(int, int, std::string);
	//Helper function to initialize non spectrum content
	void init();
	
	//Member callbacks - must be overridden by derived class
	void display_(void);
};

#endif
