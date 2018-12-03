#ifndef _BIQUAD_H
#define _BIQUAD_H

#define PI (4 * atan(1))

enum Biquad_FilterType
{
	BiquadType_Lowpass = 0,
	BiquadType_Highpass,
	BiquadType_Bandpass,
	BiquadType_Notch,
	BiquadType_Peak,
	BiquadType_Lowshelf,
	BiquadType_Highshelf
};

class Biquad
{
public:
	Biquad();
	Biquad(Biquad_FilterType type, double Fc, double Q, double peakGain_dB);
	~Biquad();
	void setType(Biquad_FilterType type);
	void setQ(double Q);
	void setFc(double Fc);
	void setPeakGain_dB(double peakGain_dB);
	void setBiquad(Biquad_FilterType type, double Fc, double Q, double peakGain_dB);
	double process(double input);

private:
	void calcBiquad();

	Biquad_FilterType type;
	double a0, a1, a2, b1, b2;
	double Fc, Q, peakGain_dB;
	double z1, z2;
};

//We use inline, which should produce faster code,
//however, binary will grow in size...
inline double Biquad::process(double input)
{
	double output = input * a0 + z1;
	z1 = input * a1 + z2 - b1 * output;
	z2 = input * a2 - b2 * output;
	return output;
}

#endif
