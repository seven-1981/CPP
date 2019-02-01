#include "BiquadCascade.hpp"
#include <vector>
#include <cmath>
#include <iostream>
#include <string>

BiquadCascade::BiquadCascade(Biquad_FilterType type, int order, double Fc)
{
	//Check order
	if (order < 2)
		order = 2;
	if (order % 2 != 0)
		order++;

	this->order = order;

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

	//Debug info
	bool debug = (param_list.get<bool>("debug biquad") == true);
	int split = param_list.get<int>("split audio");
	if (debug == true)
		my_console.WriteToSplitConsole("Order= " + std::to_string(order) + "; Filters= " + std::to_string(numQ), split);

	for (int i = 0; i < numQ; i++)
	{
		Q.push_back(1 / (2 * cos(angle)));
		angle += PI / (double)order;
		if (debug == true)
			my_console.WriteToSplitConsole("Number " + std::to_string(i) + ": q= " + std::to_string(Q.at(i)), split);
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

	this->order = order_;

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
	
	//Debug info
	bool debug = (param_list.get<bool>("debug biquad") == true);
	int split = param_list.get<int>("split audio");
	if (debug == true)
		my_console.WriteToSplitConsole("Order= " + std::to_string(order) + "; Filters= " + std::to_string(numQ), split);
	
	for (int i = 0; i < numQ; i++)
	{
		Q.push_back(1 / (2 * cos(angle)));
		angle += PI / (double)order;
		if (debug == true)
			my_console.WriteToSplitConsole("Number " + std::to_string(i) + ": q= " + std::to_string(Q.at(i)), split);
	}

	//Initialize biquads
	//First lowpass, then highpass
	for (int i = 0; i < numQ; i++)
		this->biquads.push_back(new Biquad(BiquadType_Lowpass, Fc_lower, Q.at(i), 0.0));

	for (int i = 0; i < numQ; i++)
		this->biquads.push_back(new Biquad(BiquadType_Highpass, Fc_upper, Q.at(i), 0.0));
}

BiquadCascade::BiquadCascade(int order)
{
	//Set order
	this->order = order;

	//Initialize biquads
	//First lowpass, then highpass
	for (int i = 0; i < this->order; i++)
		this->biquads.push_back(new Biquad());
}

BiquadCascade::~BiquadCascade()
{
	//Delete biquads and clear
	for (int i = 0; i < this->biquads.size(); i++)
		delete this->biquads.at(i);
	this->biquads.clear();
}

void BiquadCascade::get_param(std::ifstream& file)
{
	//Check if file is open
	if (file.is_open() == false)
		return;

	//Reset position of file
	file.clear();
	file.seekg(0, std::ios::beg);

	//Debug info
	bool debug = (param_list.get<bool>("debug biquad") == true);
	int split = param_list.get<int>("split audio");

	//Open file to read parameters
	for (int i = 0; i < this->order; i++)
	{
		//Find "a0" substring
		std::string s = "";
		while (s.find("a0") == std::string::npos)
			std::getline(file, s);

		//Read coefficients
		Biquad_coeff c;
		
		std::getline(file, s);
		s = s.substr(s.find(" "), s.size() - 1);
		c.b1 = std::stod(s);

		std::getline(file, s);
		s = s.substr(s.find(" "), s.size() - 1);
		c.b2 = std::stod(s);

		std::getline(file, s);
		s = s.substr(s.find(" "), s.size() - 1);
		c.a0 = std::stod(s);

		std::getline(file, s);
		s = s.substr(s.find(" "), s.size() - 1);
		c.a1 = std::stod(s);
		
		std::getline(file, s);
		s = s.substr(s.find(" "), s.size() - 1);
		c.a2 = std::stod(s);

		//Debug info
		if (debug == true)
		{
			my_console.WriteToSplitConsole("Biquad #" + std::to_string(i), split);
			my_console.WriteToSplitConsole("b1=" + std::to_string(c.b1), split);
			my_console.WriteToSplitConsole("b2=" + std::to_string(c.b2), split);
			my_console.WriteToSplitConsole("a0=" + std::to_string(c.a0), split);
			my_console.WriteToSplitConsole("a1=" + std::to_string(c.a1), split);
			my_console.WriteToSplitConsole("a2=" + std::to_string(c.a2), split);
		}

		//Set coefficients on biquads
		this->biquads.at(i)->setCoeff(c);
	}
}

void BiquadCascade::reset()
{
	//Reset all biquads
	for (int i = 0; i < this->order; i++)
		this->biquads.at(i)->reset();
}