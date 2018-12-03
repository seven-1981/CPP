#include <cmath>
#include "Biquad.h"

Biquad::Biquad()
{
	type = BiquadType_Lowpass;
	a0 = 1.0;
	a1 = a2 = b1 = b2 = 0.0;
	Fc = 0.5;
	Q = 0.707;
	peakGain_dB = 0.0;
	z1 = z2 = 0.0;
}

Biquad::Biquad(Biquad_FilterType type, double Fc, double Q, double peakGain_dB)
{
	setBiquad(type, Fc, Q, peakGain_dB);
	z1 = z2 = 0.0;
}

Biquad::~Biquad()
{

}

void Biquad::setType(Biquad_FilterType type)
{
	this->type = type;
	calcBiquad();
}
	
void Biquad::setQ(double Q)
{
	this->Q = Q;
	calcBiquad();
}

void Biquad::setFc(double Fc)
{
	this->Fc = Fc;
	calcBiquad();
}

void Biquad::setPeakGain_dB(double peakGain_dB)
{
	this->peakGain_dB = peakGain_dB;
	calcBiquad();
}
	
void Biquad::setBiquad(Biquad_FilterType type, double Fc, double Q, double peakGain_dB)
{
	this->type = type;
	this->Fc = Fc;
	this->Q = Q;
	this->peakGain_dB = peakGain_dB;
	calcBiquad();
}

void Biquad::calcBiquad()
{
	double norm;
	double V = pow(10, fabs(peakGain_dB) / 20.0);
	double K = tan(PI * Fc);

	switch (type)
	{
	case BiquadType_Lowpass:
		norm = 1 / (1 + K / Q + K * K);
		a0 = K * K * norm;
		a1 = 2 * a0;
		a2 = a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case BiquadType_Highpass:
		norm = 1 / (1 + K / Q + K * K);
		a0 = 1 * norm;
		a1 = -2 * a0;
		a2 = a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case BiquadType_Bandpass:
		norm = 1 / (1 + K / Q + K * K);
		a0 = K / Q * norm;
		a1 = 0;
		a2 = -a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case BiquadType_Notch:
		norm = 1 / (1 + K / Q + K * K);
		a0 = (1 + K * K) * norm;
		a1 = 2 * (K * K - 1) * norm;
		a2 = a0;
		b1 = a1;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case BiquadType_Peak:
		if (peakGain_dB >= 0) 
		{    
			// boost
			norm = 1 / (1 + 1 / Q * K + K * K);
			a0 = (1 + V / Q * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - V / Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1 - 1 / Q * K + K * K) * norm;
		}
		else 
		{    
			// cut
			norm = 1 / (1 + V / Q * K + K * K);
			a0 = (1 + 1 / Q * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - 1 / Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1 - V / Q * K + K * K) * norm;
		}
		break;

	case BiquadType_Lowshelf:
		if (peakGain_dB >= 0) 
		{    
			// boost
			norm = 1 / (1 + sqrt(2) * K + K * K);
			a0 = (1 + sqrt(2 * V) * K + V * K * K) * norm;
			a1 = 2 * (V * K * K - 1) * norm;
			a2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
			b1 = 2 * (K * K - 1) * norm;
			b2 = (1 - sqrt(2) * K + K * K) * norm;
		}
		else 
		{    
			// cut
			norm = 1 / (1 + sqrt(2 * V) * K + V * K * K);
			a0 = (1 + sqrt(2) * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - sqrt(2) * K + K * K) * norm;
			b1 = 2 * (V * K * K - 1) * norm;
			b2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
		}
		break;

	case BiquadType_Highshelf:
		if (peakGain_dB >= 0) 
		{    
			// boost
			norm = 1 / (1 + sqrt(2) * K + K * K);
			a0 = (V + sqrt(2 * V) * K + K * K) * norm;
			a1 = 2 * (K * K - V) * norm;
			a2 = (V - sqrt(2 * V) * K + K * K) * norm;
			b1 = 2 * (K * K - 1) * norm;
			b2 = (1 - sqrt(2) * K + K * K) * norm;
		}
		else 
		{    
			// cut
			norm = 1 / (V + sqrt(2 * V) * K + K * K);
			a0 = (1 + sqrt(2) * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - sqrt(2) * K + K * K) * norm;
			b1 = 2 * (K * K - V) * norm;
			b2 = (V - sqrt(2 * V) * K + K * K) * norm;
		}
		break;
	}
}
