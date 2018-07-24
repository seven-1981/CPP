#ifndef _DSP_H
#define _DSP_H

#include "buffer.h"
#include <limits>
#include <cmath>
#include <functional>

double tempval;	//Used for SWAP macro

#define SWAP(a, b) tempval=(a); (a) = (b); (b) = tempval
#define PI (4 * atan(1))

template <typename T>
T get_max_sample_value(const buffer<T>& buffer)
{
	//Get buffer size
	long size = buffer.get_size();

	//This function retrieves the max value from the buffer
	T max_value = std::numeric_limits<T>::min();
	for (long i = 0; i < size; i++)
	{
		if (buffer.values[i] > max_value)
			max_value = buffer.values[i];
	}

	return max_value;
}

template <typename T>
T get_min_sample_value(const buffer<T>& buffer)
{
	//Get buffer size
	long size = buffer.get_size();

	//This function retrieves the min value from the buffer
	T min_value = std::numeric_limits<T>::max();
	for (long i = 0; i < size; i++)
	{
		if (buffer.values[i] < min_value)
			min_value = buffer.values[i];
	}

	return min_value;
}

template <typename T>
void maximize_volume(buffer<T>& buffer)
{
	//Get buffer size
	long size = buffer.get_size();

	//Get max and min sample values of buffer
	T max = get_max_sample_value(buffer);
	T min = get_min_sample_value(buffer);

	//Declare maximization factor
	double factor;

	//Check which one is bigger
	if (max >= abs(min))
		factor = (double)std::numeric_limits<T>::max() / max;
	else
		factor = abs(((double)std::numeric_limits<T>::min()) / min);

	//Maximize the buffer
	for (long i = 0; i < size; i++)
	{
		buffer.values[i] = (T)((double)buffer.values[i] * factor);
	}
}

template <typename T>
void gain(buffer<T>& buffer, double gain)
{
	//Get buffer size
	long size = buffer.get_size();

	//Declare maximization factor
	double factor = gain;

	//Maximize the buffer
	for (long i = 0; i < size; i++)
	{
		buffer.values[i] = (T)((double)buffer.values[i] * factor);
	}
}

template <typename T>
double get_mean_value(const buffer<T>& buffer)
{
	//Get buffer size
	long size = buffer.get_size();

	//Calculate average value of all values in the buffer
	double avg = 0.0;

	//Go through values and calculate mean value
	for (long i = 0; i < size; i++)
		avg += (double)buffer.values[i] / size;

	return avg;
}

template <typename T>
double get_variance_value(const buffer<T>& buffer)
{
	//Get buffer size
	long size = buffer.get_size();

	//Variance return value
	double var = 0.0;

	//Get average value
	double avg = get_mean_value(buffer);
	double sq = 0.0, av = 0.0;

	//Go through values and calculate variance
	for (long i = 0; i < size; i++)
	{
		sq += (double)buffer.values[i] * (double)buffer.values[i];
		av += (double)buffer.values[i];
	}

	var = (sq - (av * av / size)) / (size - 1.0);

	return var;
}

template <typename T>
void combine_buffers(const buffer<T>& buffer_X, const buffer<T>& buffer_Y, buffer<T>& outbuffer)
{
	//Get buffer size
	long size = buffer_X.get_size();

	for (long i = 0; i < size; i++)
	{
		outbuffer.values[i * 2] = buffer_X.values[i];
		outbuffer.values[i * 2 + 1] = buffer_Y.values[i];
	}
}

template <typename T>
void separate_buffers(const buffer<T>& inbuffer, buffer<T>& outbuffer_X, buffer<T>& outbuffer_Y)
{
	//Get buffer size
	long size = outbuffer_X.get_size();

	for (long i = 0; i < size; i++)
	{
		outbuffer_X.values[i] = inbuffer.values[i * 2];
		outbuffer_Y.values[i] = inbuffer.values[i * 2 + 1];
	}
}

void doublify(const buffer<short>& sbuffer, buffer<double>& dbuffer)
{
	//Get buffer size
	long size = sbuffer.get_size();

	for (long i = 0; i < size; i++)
		dbuffer.values[i] = (double)sbuffer.values[i];
}

void shortify(const buffer<double>& dbuffer, buffer<short>& sbuffer)
{
	//Get buffer size
	long size = dbuffer.get_size();

	for (long i = 0; i < size; i++)
		sbuffer.values[i] = (short)dbuffer.values[i];
}

template <typename T>
void normalize(buffer<T>& buffer, T maxvalue)
{
	//Get buffer size
	long size = buffer.get_size();

	//Highest value in buffer is equal to datatype max
	double max = get_max_sample_value(buffer);
	double min = get_min_sample_value(buffer);
	double factor;

	if (max >= abs(min))
		factor = (double)maxvalue / max;
	else
		factor = (double)-maxvalue / min;

	for (long i = 0; i < size; i++)
		buffer.values[i] = (T)((double)buffer.values[i] * factor);
}

template <typename T>
void rectify(buffer<T>& buffer)
{
	//Get buffer size
	long size = buffer.get_size();

	for (long i = 0; i < size; i++)
		buffer.values[i] = abs(buffer.values[i]);
}

template <typename T>
double get_autocorr(double lag, const buffer<T>& buffer)
{
	//Get buffer size
	long size = buffer.get_size();

	//Get sample rate
	long sample_rate = buffer.get_sample_rate();

	//Calculate time axis - lag is in seconds
	double time_max = (double)size / sample_rate;

	//Calculate number of samples corresponding to desired lag - division /2 due to left-right
	long lag_samples = (long)ceil((lag / time_max) * size);

	//Calculate average
	double average = get_mean_value(buffer);

	//Calculate variance
	double variance = get_variance_value(buffer);

	//Calculate autocorrelation
	double autocorr = 0.0;
	for (long i = 0; i < size - lag_samples; i++)
		autocorr += (((double)buffer.values[i] - average) * ((double)buffer.values[i + lag_samples] - average));

	autocorr = autocorr / (size - lag_samples);
	autocorr = autocorr / variance;
	autocorr *= autocorr;

	return autocorr;
}

template <typename T>
double get_rms_value(const buffer<T>& inbuffer)
{
	//Get buffer size
	long size = inbuffer.get_size();

	//RMS value
	double rms_value;

	for (long i = 0; i < size; i++)
		rms_value += ((double)inbuffer.values[i] * (double)inbuffer.values[i]) / size;

	return sqrt(rms_value);
}

template <typename T>
void downsample_buffer(const buffer<T>& inbuffer, buffer<T>& outbuffer)
{
	//Get buffer size
	long size = outbuffer.get_size();

	for (long i = 0; i < size * 2; i += 2)
		outbuffer.values[i / 2] = (T)(((double)inbuffer.values[i] + (double)inbuffer.values[i + 1]) / 2.0);
}

template <typename T>
void envelope_filter(const buffer<T>& inbuffer, buffer<double>& outbuffer, double recovery)
{
	//Get buffer size
	long size = inbuffer.get_size();

	T env_in;
	T peak_env = 0;
	double release = exp(-1.0 / (inbuffer.get_sample_rate() * recovery));

	for (long i = 0; i < size; i++)
	{
		env_in = (T)abs(inbuffer.values[i]);
		if (env_in > peak_env)
			peak_env = env_in;
		else
		{
			peak_env = (T)(peak_env * release);
			peak_env = peak_env + (T)((1.0 - release) * (double)env_in);
		}

		outbuffer.values[i] = peak_env;
	}
}

template <typename T>
void perform_fft(const buffer<T>& time_dom, buffer<double>& freq_dom, int sign)
{
	//Get buffer size
	long size = time_dom.get_size();

	// Put data in complex buffer - only real data, imaginary is zero
	for (long i = 0; i < size; i++)
	{
		freq_dom.values[i * 2] = (double)time_dom.values[i];
		freq_dom.values[i * 2 + 1] = 0;
	}

	unsigned long n, mmax, m, j, istep, i;
	double wtemp, wr, wpr, wpi, wi, theta;
	double tempr, tempi;

	//Reverse-binary reindexing
	n = size << 1;
	j = 1;
	for (i = 1; i < n; i += 2)
	{
		if (j > i)
		{
			SWAP(freq_dom.values[j - 1], freq_dom.values[i - 1]);
			SWAP(freq_dom.values[j], freq_dom.values[i]);
		}
		m = size;
		while (m >= 2 && j > m)
		{
			j -= m;
			m >>= 1;
		}
		j += m;
	}

	//Here begins the Danielson-Lanczos section
	mmax = 2;
	while (n > mmax)
	{
		istep = mmax << 1;
		theta = -sign * (2 * PI / mmax); // - for forward, + for inverse
		wtemp = sin(0.5 * theta);
		wpr = -2.0 * wtemp * wtemp;
		wpi = sin(theta);
		wr = 1.0;
		wi = 0.0;
		for (m = 1; m < mmax; m += 2)
		{
			for (i = m; i <= n; i += istep)
			{
				j = i + mmax;
				tempr = wr * freq_dom.values[j - 1] - wi * freq_dom.values[j];
				tempi = wr * freq_dom.values[j] + wi * freq_dom.values[j - 1];

				freq_dom.values[j - 1] = freq_dom.values[i - 1] - tempr;
				freq_dom.values[j] = freq_dom.values[i] - tempi;
				freq_dom.values[i - 1] += tempr;
				freq_dom.values[i] += tempi;
			}
			wtemp = wr;
			wr += wr * wpr - wi * wpi;
			wi += wi * wpr + wtemp * wpi;
		}
		mmax = istep;
	}
}

template <typename T1, typename T2>
void create_fft_buffer(const buffer<T1>& timebuffer, buffer<T2>& fftbuffer)
{
	//Check if fftbuffer is already initialized
	if (fftbuffer.values == nullptr)
	{
		//Not initialized
		fftbuffer.init_buffer(pow2_size(timebuffer), timebuffer.get_sample_rate());
	}

	//Get buffer sizes
	long time_size = timebuffer.get_size();
	long fft_size = fftbuffer.get_size();

	for (long i = 0; i < fft_size; i++)
		fftbuffer.values[i] = (T2)timebuffer.values[i];
}

template <typename T>
long pow2_size(const buffer<T>& inbuffer)
{
	return (long)pow(2.0, floor(log2(inbuffer.get_size())));
}

template <typename T>
void apply_window(const buffer<T>& inbuffer, buffer<double>& outbuffer, std::function<double(double, double)> f)
{
	//Get buffer size
	long size = inbuffer.get_size();

	for (long i = 0; i < size; i++)
		outbuffer.values[i] = (double)inbuffer.values[i] * f((double)i, (double)size);
}

template <typename T>
void apply_window(const buffer<T>& inbuffer, buffer<double>& outbuffer, std::function<double(double, double, double)> f, double a)
{
	//Get buffer size
	long size = inbuffer.get_size();

	for (long i = 0; i < size; i++)
		outbuffer.values[i] = (double)inbuffer.values[i] * f((double)i, (double)size, a);
}

double hanning(double x, double N)
{
	return 0.5 * (1 - cos(2 * PI * x / (N - 1)));
}

double tukey(double x, double N, double alpha)
{
	if (x >= 0 && x < (alpha * (N - 1) / 2))
		return 0.5 * (1 + cos(PI * ((2 * x / (alpha * (N - 1))) - 1)));
	else if ((N - 1) * (1 - alpha / 2) < x && x <= (N - 1))
		return 0.5 * (1 + cos(PI * (1 - 2 / alpha + (2 * x / (alpha * (N - 1))))));
	else
		return 1.0;
}

template <typename T>
void lowcut_freq(const buffer<T>& inbuffer, buffer<double>& outbuffer, double freq)
{
	//Get buffer size
	long size = outbuffer.get_size();

	//Get frequency resolution
	double freqres = (double)inbuffer.get_sample_rate() / inbuffer.get_size();

	//Get maximum bin for truncation
	long binmax = (long)(freq / freqres);

	for (long i = 0; i < size; i++)
	{
		if (i < binmax)
			outbuffer.values[i] = inbuffer.values[i];
		else
			outbuffer.values[i] = 0.0;
	}
}

#endif
