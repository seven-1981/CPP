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
	//This function retrieves the max value from the buffer
	T max_value = std::numeric_limits<T>::min();
	for (long i = 0; i < buffer.size; i++)
	{
		if (buffer.values[i] > max_value)
			max_value = buffer.values[i];
	}

	return max_value;
}

template <typename T>
T get_min_sample_value(const buffer<T>& buffer)
{
	//This function retrieves the min value from the buffer
	T min_value = std::numeric_limits<T>::max();
	for (long i = 0; i < buffer.size; i++)
	{
		if (buffer.values[i] < min_value)
			min_value = buffer.values[i];
	}

	return min_value;
}

template <typename T>
void maximize_volume(buffer<T>& buffer)
{
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
	for (long i = 0; i < buffer.size; i++)
	{
		buffer.values[i] = (T)((double)buffer.values[i] * factor);
	}
}

template <typename T>
void gain(buffer<T>& buffer, double gain)
{
	//Declare maximization factor
	double factor = gain;

	//Maximize the buffer
	for (long i = 0; i < buffer.size; i++)
	{
		buffer.values[i] = (T)((double)buffer.values[i] * factor);
	}
}

template <typename T>
double get_mean_value(const buffer<T>& buffer)
{
	//Calculate average value of all values in the buffer
	double avg = 0.0;

	//Go through values and calculate mean value
	for (long i = 0; i < buffer.size; i++)
		avg += (double)buffer.values[i] / buffer.size;

	return avg;
}

template <typename T>
double get_variance_value(const buffer<T>& buffer)
{
	double var = 0.0;

	//Get average value
	double avg = get_mean_value(buffer);
	double sq = 0.0, av = 0.0;

	//Go through values and calculate variance
	for (long i = 0; i < buffer.size; i++)
	{
		sq += (double)buffer.values[i] * (double)buffer.values[i];
		av += (double)buffer.values[i];
	}
	
	var = (sq - (av * av / buffer.size)) / (buffer.size - 1.0);

	return var;
}

template <typename T>
void combine_buffers(const buffer<T>& buffer_L, const buffer<T>& buffer_R, buffer<T>& outbuffer)
{
	for (long i = 0; i < buffer_L.size; i++)
	{
		outbuffer.values[i * 2] = buffer_L.values[i];
		outbuffer.values[i * 2 + 1] = buffer_R.values[i];
	}
}

template <typename T>
void separate_buffers(const buffer<T>& inbuffer, buffer<T>& outbuffer_L, buffer<T>& outbuffer_R)
{
	for (long i = 0; i < outbuffer_L.size ; i++)
	{
		outbuffer_L.values[i] = inbuffer.values[i * 2];
		outbuffer_R.values[i] = inbuffer.values[i * 2 + 1];
	}
}

void doublify(const buffer<short>& sbuffer, buffer<double>& dbuffer)
{
	for (long i = 0; i < sbuffer.size; i++)
		dbuffer.values[i] = (double)sbuffer.values[i];
}

void shortify(const buffer<double>& dbuffer, buffer<short>& sbuffer)
{
	for (long i = 0; i < dbuffer.size; i++)
		sbuffer.values[i] = (short)dbuffer.values[i];
}

template <typename T>
void normalize(buffer<T>& buffer, T maxvalue)
{
	double max = get_max_sample_value(buffer);
	double min = get_min_sample_value(buffer);
	double factor;

	if (max >= abs(min))
		factor = (double)maxvalue / max;
	else
		factor = (double)-maxvalue / min;

	for (long i = 0; i < buffer.size; i++)
		buffer.values[i] = (T)((double)buffer.values[i] * factor);
}

template <typename T>
void rectify(buffer<T>& buffer)
{
	for (long i = 0; i < buffer.size; i++)
		buffer.values[i] = abs(buffer.values[i]);
}

template <typename T>
double get_autocorr(double lag, const buffer<T>& buffer)
{
	//Calculate time axis - lag is in seconds
	double time_max = (double)buffer.size / buffer.sample_rate;

	//Calculate number of samples corresponding to desired lag - division /2 due to left-right
	long lag_samples = (long)ceil((lag / time_max) * buffer.size);

	//Calculate average
	double average = get_mean_value(buffer);

	//Calculate variance
	double variance = get_variance_value(buffer);

	//Calculate autocorrelation
	double autocorr = 0.0;
	for (long i = 0; i < buffer.size - lag_samples; i++)
		autocorr += (((double)buffer.values[i] - average) * ((double)buffer.values[i + lag_samples] - average));

	autocorr = autocorr / (buffer.size - lag_samples);
	autocorr = autocorr / variance;
	autocorr *= autocorr;

	return autocorr;
}

template <typename T>
double get_rms_value(const buffer<T>& inbuffer)
{
	buffer<double> dbuffer(inbuffer.size, inbuffer.sample_rate);

	for (long i = 0; i < inbuffer.size; i++)
		dbuffer.values[i] = ((double)inbuffer.values[i] * (double)inbuffer.values[i]);

	double mean = get_mean_value(dbuffer);

	return sqrt(mean);
}

template <typename T>
void downsample_buffer(const buffer<T>& inbuffer, buffer<T>& outbuffer)
{
	for (long i = 0; i < outbuffer.size * 2; i += 2)
		outbuffer.values[i / 2] = (T)(((double)inbuffer.values[i] + (double)inbuffer.values[i + 1]) / 2.0);
}

template <typename T>
void envelope_filter(const buffer<T>& inbuffer, buffer<double>& outbuffer, double recovery)
{
	T env_in;
	T peak_env = 0;
	double release = exp(-1.0 / (inbuffer.sample_rate * recovery));
	for (long i = 0; i < inbuffer.size; i++)
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
	// Put data in complex buffer
	for (long i = 0; i < time_dom.size; i++)
	{
		freq_dom.values[i * 2] = (double)time_dom.values[i];
		freq_dom.values[i * 2 + 1] = 0;
	}

	unsigned long n, mmax, m, j, istep, i;
	double wtemp, wr, wpr, wpi, wi, theta;
	double tempr, tempi;

	// reverse-binary reindexing
	n = time_dom.size << 1;
	j = 1;
	for (i = 1; i < n; i += 2) 
	{
		if (j > i) 
		{
			SWAP(freq_dom.values[j - 1], freq_dom.values[i - 1]);
			SWAP(freq_dom.values[j], freq_dom.values[i]);
		}
		m = time_dom.size;
		while (m >= 2 && j > m) 
		{
			j -= m;
			m >>= 1;
		}
		j += m;
	}

	// here begins the Danielson-Lanczos section
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

template <typename T, typename U>
void create_fft_buffer(const buffer<T>& normbuffer, buffer<U>& fftbuffer)
{
	double num_samples = pow(2.0, ceil(log2(normbuffer.size)));

	for (long i = 0; i < fftbuffer.size; i++)
	{
		if (i < normbuffer.size)
			fftbuffer.values[i] = normbuffer.values[i];
		else
			fftbuffer.values[i] = 0;
	}
}

template <typename T>
void apply_window(const buffer<T>& inbuffer, buffer<double>& outbuffer, std::function<double(double, double)> f)
{
	for (long i = 0; i < inbuffer.size; i++)
		outbuffer.values[i] = (double)inbuffer.values[i] * f((double)i, (double)inbuffer.size);
}

#endif
