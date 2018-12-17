#include "BiquadCascade.h"
#include <vector>
#include <cmath>
#include <iostream>

BiquadCascade::BiquadCascade(Biquad_FilterType type, int order, double Fc)
{
	//Check order
	if (order < 2)
		order = 2;
	if (order % 2 != 0)
		order++;

	//Check type - only low and highpass may be used
	Biquad_FilterType type_to_set;
	switch (type)
	{
		case BiquadType_Lowpass:
		case BiquadType_Highpass:
			type_to_set = type;
			break;
		default:
			type_to_set = BiquadType_Lowpass;
			break;
	}

	//Number of Q values - equals number of biquads
	int numQ = order / 2;
	//Calculate Q values
	std::vector<double> Q;
	double angle = PI / (double)order / 2;

	#ifdef DEBUG_ENABLE
		std::cout << "Order= " << order << "; Filters= " << numQ << std::endl;
	#endif

	for (int i = 0; i < numQ; i++)
	{
		Q.push_back(1 / (2 * cos(angle)));
		angle += PI / (double)order;
		#ifdef DEBUG_ENABLE
			std::cout << "Number " << i << ": q= " << Q.at(i) << std::endl;
		#endif	
	}

	//Initialize biquads
	for (int i = 0; i < numQ; i++)
		this->biquads.push_back(new Biquad(type_to_set, Fc, Q.at(i), 0.0));
}

BiquadCascade::BiquadCascade(Biquad_FilterType type, int order_, double Fc_lower, double Fc_upper)
{
	//Check order - for bandpass order is twice as for lowpass or highpass
	if (order_ < 4)
		order_ = 4;
	if (order_ % 4 != 0)
		order_ += (4 - (order_ % 4));

	int order = order_ / 2;

	//Check type - only bandpass
	Biquad_FilterType type_to_set;
	switch (type)
	{
	case BiquadType_Bandpass:
		type_to_set = type;
		break;
	default:
		type_to_set = BiquadType_Bandpass;
		break;
	}

	//Number of Q values - equals number of biquads
	//Bandpass uses lowpass and highpass combined
	int numQ = order / 2;
	//Calculate Q values
	std::vector<double> Q;
	double angle = PI / (double)order / 2;
	
	#ifdef DEBUG_ENABLE
		std::cout << "Order= " << order << "; Filters= " << numQ << std::endl;
	#endif
	
	for (int i = 0; i < numQ; i++)
	{
		Q.push_back(1 / (2 * cos(angle)));
		angle += PI / (double)order;
		#ifdef DEBUG_ENABLE
			std::cout << "Number " << i << ": q= " << Q.at(i) << std::endl;
		#endif	
	}

	//Initialize biquads
	//First lowpass, then highpass
	for (int i = 0; i < numQ; i++)
		this->biquads.push_back(new Biquad(BiquadType_Lowpass, Fc_lower, Q.at(i), 0.0));

	for (int i = 0; i < numQ; i++)
		this->biquads.push_back(new Biquad(BiquadType_Highpass, Fc_upper, Q.at(i), 0.0));
}

BiquadCascade::~BiquadCascade()
{
	//Delete biquads and clear
	for (int i = 0; i < this->biquads.size(); i++)
		delete this->biquads.at(i);
	this->biquads.clear();
}
