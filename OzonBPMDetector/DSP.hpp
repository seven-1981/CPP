#ifndef _DSP_H
#define _DSP_H

#include "buffer.hpp"
#include <limits>
#include <cmath>
#include <functional>
#include <algorithm>
#include <vector>
#include <string>

namespace DSP
{

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
			if (buffer[i] > max_value)
				max_value = buffer[i];
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
			if (buffer[i] < min_value)
				min_value = buffer[i];
		}

		return min_value;
	}

	template <typename T>
	long get_max_index(const buffer<T>& buffer)
	{
		//Get buffer size
		long size = buffer.get_size();
		T max_value = std::numeric_limits<T>::min();

		//Declare return value
		long index = 0;

		//Get max value index
		for (long i = 0; i < size; i++)
		{
			if (buffer[i] > max_value)
			{
				max_value = buffer[i];
				index = i;
			}
		}

		return index;
	}

	template <typename T>
	void maximize_volume(buffer<T>& buffer)
	{
		//Get buffer size
		long size = buffer.get_size();

		//Get max and min sample values of buffer
		T max = DSP::get_max_sample_value(buffer);
		T min = DSP::get_min_sample_value(buffer);

		//Declare maximization factor
		double factor;

		//Check which one is bigger
		if (max >= fabs(min))
			factor = (double)std::numeric_limits<T>::max() / max;
		else
			factor = fabs(((double)std::numeric_limits<T>::min()) / min);

		//Maximize the buffer
		for (long i = 0; i < size; i++)
		{
			buffer[i] = (T)((double)buffer[i] * factor);
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
			buffer[i] = (T)((double)buffer[i] * factor);
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
			avg += (double)buffer[i] / size;

		return avg;
	}

	template <typename T>
	double get_variance_value(const buffer<T>& buffer)
	{
		//Get buffer size
		long size = buffer.get_size();

		//Variance return value
		double var = 0.0;

		double sq = 0.0, av = 0.0;

		//Go through values and calculate variance
		for (long i = 0; i < size; i++)
		{
			sq += (double)buffer[i] * (double)buffer[i];
			av += (double)buffer[i];
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
			outbuffer[i * 2] = buffer_X[i];
			outbuffer[i * 2 + 1] = buffer_Y[i];
		}
	}

	template <typename T>
	void separate_buffers(const buffer<T>& inbuffer, buffer<T>& outbuffer_X, buffer<T>& outbuffer_Y)
	{
		//Get buffer size
		long size = outbuffer_X.get_size();

		for (long i = 0; i < size; i++)
		{
			outbuffer_X[i] = inbuffer[i * 2];
			outbuffer_Y[i] = inbuffer[i * 2 + 1];
		}
	}

	void doublify(const buffer<short>& sbuffer, buffer<double>& dbuffer)
	{
		//Get buffer size
		long size = sbuffer.get_size();

		for (long i = 0; i < size; i++)
			dbuffer[i] = (double)sbuffer[i];
	}

	void shortify(const buffer<double>& dbuffer, buffer<short>& sbuffer)
	{
		//Get buffer size
		long size = dbuffer.get_size();

		//Get max/min value and scale factors
		double max = DSP::get_max_sample_value(dbuffer);
		double min = DSP::get_min_sample_value(dbuffer);

		//Declare maximization factor
		double factor;

		//Check which one is bigger
		if (max >= fabs(min))
			factor = (double)std::numeric_limits<short>::max() / max;
		else
			factor = fabs(((double)std::numeric_limits<short>::min()) / min);

		//Scale to short max
		for (long i = 0; i < size; i++)
			sbuffer[i] = (short)(dbuffer[i] * factor);
	}

	template <typename T>
	void normalize(buffer<T>& buffer, T maxvalue)
	{
		//Get buffer size
		long size = buffer.get_size();

		//Highest value in buffer is equal to datatype max
		double max = DSP::get_max_sample_value(buffer);
		double min = DSP::get_min_sample_value(buffer);
		double factor;

		if (max >= abs(min))
			factor = (double)maxvalue / max;
		else
			factor = (double)-maxvalue / min;

		for (long i = 0; i < size; i++)
			buffer[i] = (T)((double)buffer[i] * factor);
	}

	template <typename T>
	void rectify(buffer<T>& buffer)
	{
		//Get buffer size
		long size = buffer.get_size();

		for (long i = 0; i < size; i++)
			buffer[i] = fabs(buffer[i]);
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

		//Calculate number of samples corresponding to desired lag
		long lag_samples = (long)ceil((lag / time_max) * size);

		//Calculate average
		double average = DSP::get_mean_value(buffer);

		//Calculate variance
		double variance = DSP::get_variance_value(buffer);

		//Calculate autocorrelation
		double autocorr = 0.0;
		for (long i = 0; i < size - lag_samples; i++)
			autocorr += (((double)buffer[i] - average) * ((double)buffer[i + lag_samples] - average));

		autocorr = autocorr / (size - lag_samples);
		autocorr = autocorr / variance;
		autocorr *= autocorr;

		return autocorr;
	}

	//Faster implementation of "get_autocorr" - used in for loop of "build_autocorr_array"
	template <typename T>
	double get_autocorr(double lag, const buffer<T>& buffer, long size, long sample_rate, double average, double variance)
	{
		//Calculate time axis - lag is in seconds
		double time_max = (double)size / sample_rate;

		//Calculate number of samples corresponding to desired lag
		long lag_samples = (long)ceil((lag / time_max) * size);

		//Calculate autocorrelation
		double autocorr = 0.0;
		for (long i = 0; i < size - lag_samples; i++)
			autocorr += (((double)buffer[i] - average) * ((double)buffer[i + lag_samples] - average));

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
		double rms_value = 0.0;

		for (long i = 0; i < size; i++)
			rms_value += ((double)inbuffer[i] * (double)inbuffer[i]) / size;

		return sqrt(rms_value);
	}

	template <typename T>
	void downsample_buffer(const buffer<T>& inbuffer, buffer<T>& outbuffer, int N)
	{
		//Get buffer size
		long size = outbuffer.get_size();

		//Get every Nth sample
		for (long i = 0; i < size * N; i += N)
			outbuffer[i / N] = (T)((double)inbuffer[i]);
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
			env_in = (T)fabs(inbuffer[i]);
			if (env_in > peak_env)
				peak_env = env_in;
			else
			{
				peak_env = (T)(peak_env * release);
				peak_env = peak_env + (T)((1.0 - release) * (double)env_in);
			}

			outbuffer[i] = peak_env;
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
			freq_dom[i * 2] = (double)time_dom[i];
			freq_dom[i * 2 + 1] = 0;
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
				DSP::SWAP(freq_dom[j - 1], freq_dom[i - 1]);
				DSP::SWAP(freq_dom[j], freq_dom[i]);
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
					tempr = wr * freq_dom[j - 1] - wi * freq_dom[j];
					tempi = wr * freq_dom[j] + wi * freq_dom[j - 1];

					freq_dom[j - 1] = freq_dom[i - 1] - tempr;
					freq_dom[j] = freq_dom[i] - tempi;
					freq_dom[i - 1] += tempr;
					freq_dom[i] += tempi;
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
		//Get buffer sizes
		long time_size = timebuffer.get_size();
		long fft_size = fftbuffer.get_size();

		//Determine if zero padding is necessary
		if (time_size < fft_size)
		{
			for (long i = 0; i < fft_size; i++)
				if (i >= time_size)
					fftbuffer[i] = (T2)timebuffer[i];
				else
					fftbuffer[i] = 0.0;
		}
		else
		{
			for (long i = 0; i < fft_size; i++)
				fftbuffer[i] = (T2)timebuffer[i];
		}
	}

	template <typename T>
	long pow2_size(const buffer<T>& inbuffer, bool floor_ceil)
	{
		if (floor_ceil == true)
			return (long)pow(2.0, floor(log2(inbuffer.get_size())));
		else
			return (long)pow(2.0, ceil(log2(inbuffer.get_size())));
	}

	template <typename T>
	void apply_window(const buffer<T>& inbuffer, buffer<double>& outbuffer, std::function<double(double, double)> f)
	{
		//Get buffer size
		long size = inbuffer.get_size();

		for (long i = 0; i < size; i++)
			outbuffer[i] = (double)inbuffer[i] * f((double)i, (double)size);
	}

	template <typename T>
	void apply_window(buffer<T>& inbuffer, std::function<double(double, double)> f)
	{
		//Get buffer size
		long size = inbuffer.get_size();

		for (long i = 0; i < size; i++)
		{
			double temp = inbuffer[i];
			inbuffer[i] = temp * f((double)i, (double)size);
		}
	}

	template <typename T>
	void apply_window(const buffer<T>& inbuffer, buffer<double>& outbuffer, std::function<double(double, double, double)> f, double a)
	{
		//Get buffer size
		long size = inbuffer.get_size();

		for (long i = 0; i < size; i++)
			outbuffer[i] = (double)inbuffer[i] * f((double)i, (double)size, a);
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

	double weight(double x, double N)
	{
		return exp(-x / N);
	}

	template <typename T>
	void cut_freq(const buffer<T>& inbuffer, buffer<double>& outbuffer, double freq_min, double freq_max)
	{
		//Get buffer size
		long size = outbuffer.get_size();

		//Get frequency resolution
		double freqres = (double)inbuffer.get_sample_rate() / inbuffer.get_size();

		//Get minimum maximum bin for truncation
		long binmin = (long)(freq_min / freqres);
		long binmax = (long)(freq_max / freqres);

		for (long i = 0; i < size; i++)
		{
			if ((i > binmin) && (i < binmax))
				outbuffer[i] = inbuffer[i];
			else
				outbuffer[i] = 0.0;
		}
	}

	template <typename T>
	void moving_average(const buffer<T>& inbuffer, buffer<double>& outbuffer, long N)
	{
		//Moving average filtered values
		double ma_value = 0.0;

		//Get buffer size
		long size = inbuffer.get_size();
		long start, num;

		for (long i = 0; i < size; i++)
		{
			//Calculate start and stop indexes
			if (i < N)
			{
				start = 0;
				num = i + 1;
			}
			else
			{
				start = i - N + 1;
				num = N;
			}

			//Calculate moving average
			for (long j = start; j < start + num; j++)
				ma_value = ma_value + (double)inbuffer[j] / num;

			outbuffer[i] = ma_value;
			ma_value = 0.0;
		}
	}

	template <typename T>
	void build_autocorr_array(const buffer<T>& inbuffer, buffer<double>& autocorr_array, double bpm_min, double bpm_max)
	{
		//Get buffer sizes
		long size_autocorr = autocorr_array.get_size();
		long size_inbuffer = inbuffer.get_size();

		//Get sample rate
		long sample_rate = inbuffer.get_sample_rate();

		//Get mean value and variance
		double average = DSP::get_mean_value(inbuffer);
		double variance = DSP::get_variance_value(inbuffer);

		//Calculate lag values -> beats/minute to seconds/beat
		double min_lag = (double)60 / bpm_max;
		double max_lag = (double)60 / bpm_min;
		double lag;

		for (long i = 0; i < size_autocorr; i++)
		{
			lag = min_lag + (max_lag - min_lag) / (double)size_autocorr * (double)i;
			//Uses faster implementation of "get_autocorr"
			autocorr_array[i] = DSP::get_autocorr(lag, inbuffer, size_inbuffer, sample_rate, average, variance);
		}
	}

	double extract_bpm_value(const buffer<double>& autocorr_array, double bpm_min, double bpm_max)
	{
		//Get buffer size
		long size = autocorr_array.get_size();

		//Declare return value
		long max_array_index = 0;

		//Calculate lag values -> beats/minute to seconds/beat
		double min_lag = (double)60 / bpm_max;
		double max_lag = (double)60 / bpm_min;

		//Get max value index from autocorr array
		max_array_index = DSP::get_max_index(autocorr_array);

		//Return the calculated bpm value from the max index
		double bpm_lag = min_lag + (max_lag - min_lag) / size * max_array_index;

		return 60.0 / bpm_lag;
	}
}

#endif
