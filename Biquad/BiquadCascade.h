#ifndef _BIQUAD_CASCADE_H
#define _BIQUAD_CASCADE_H

#include "Biquad.h"
#include <vector>

class BiquadCascade
{
public:
	explicit BiquadCascade(Biquad_FilterType type, int order, double Fc);	//Lowpass, highpass
	explicit BiquadCascade(Biquad_FilterType type, int order, double Fc_lower, double Fc_upper);	//Bandpass
	~BiquadCascade();
	double process(double input);

private:
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
