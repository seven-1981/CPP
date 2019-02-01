#ifndef _BIQUAD_CASCADE_H
#define _BIQUAD_CASCADE_H

#include "Biquad.hpp"
#include "SplitConsole.hpp"
#include "bpm_param.hpp"
#include <vector>
#include <fstream>

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

class BiquadCascade
{
public:
	explicit BiquadCascade(Biquad_FilterType type, int order, double Fc);	//Lowpass, highpass
	explicit BiquadCascade(Biquad_FilterType type, int order, double Fc_lower, double Fc_upper);	//Bandpass
	explicit BiquadCascade(int order);	//Manually set coefficients
	~BiquadCascade();
	double process(double input);
	void get_param(std::ifstream& file); //Read filter coefficients from IOWA IIR Filter Design Tool
	void reset();
	
private:
	int order;
	std::vector<Biquad*> biquads;
};

inline double BiquadCascade::process(double input)
{
	double output = input;
	for (int i = 0; i < this->biquads.size(); i++)
	{
		output = this->biquads.at(i)->process(output);
	}
	return output;
}

#endif