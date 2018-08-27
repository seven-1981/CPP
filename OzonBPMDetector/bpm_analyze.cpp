//The math header file is included differently on both OS
//Note: the includes must be done in this specific order, otherwise the
//inclusion is not working properly!
#ifdef _WIN32
	#include <math.h>
	#define _USE_MATH_DEFINES
#else
	#include <cmath>
#endif

#include <iostream>
#include "bpm_analyze.hpp"
#include "bpm_globals.hpp"

BPMAnalyze::BPMAnalyze()
{
	//Constructor for analyzer class instance
	//#ifdef DEBUG_AUDIO
		my_console.WriteToSplitConsole("Instantiating audio analyzer class.", SPLIT_AUDIO);
	//#endif

	//Store size information - in frames
	this->buffer_size = PCM_BUF_SIZE;

	//Initialize data buffers
	//It would be possible to have only one or two different buffers, but it's more
	//convenient this way and we can extract the data at any stage during analysis processing
	this->data_out = (short*)malloc(buffer_size * 2);

	this->left = (short*)malloc(buffer_size);
	this->right = (short*)malloc(buffer_size);
	this->left_filt = (short*)malloc(buffer_size);
	this->right_filt = (short*)malloc(buffer_size);

	//Initialize autocorrelation values buffer
	this->autocorr_array = (double*)malloc(AUTOCORR_RES * 8);

	//Set state - constructor executed, instance created
	this->state = eReadyForData;
}

BPMAnalyze::~BPMAnalyze()
{
	//Constructor for audio analyer instance
	#ifdef DEBUG_AUDIO
		my_console.WriteToSplitConsole("Releasing audio analyzer resources.", SPLIT_MAIN);
	#endif

	//Don't forget to free the allocated buffers again
	free(data_out);
	
	free(left);
	free(right);
	free(left_filt);
	free(right_filt);

	free(autocorr_array);
}

eError BPMAnalyze::prepare_audio_data(short* data)
{
	//Check if init has finished 
	if (this->state == eNotInitialized)
		return eAnalyzer_NotInitialized;

	//Check if analyzer is ready to get data
	if (this->state != eReadyForData)
		return eAnalyzer_NotReadyYet;

	//Write debug message
	#ifdef DEBUG_AUDIO
		my_console.WriteToSplitConsole("Preparing audio data. Spliting captured records into two buffers.", SPLIT_AUDIO);
	#endif

	//Read PCM samples and divide them into left and right array
	for(long i = 0; i < this->buffer_size; i += 2)
	{
		this->left[i / 2]  = data[i];
		this->right[i / 2] = data[i + 1];
	}

	//Set the state - data is ready
	this->state = eDataCopied;

	return eSuccess;
}

eError BPMAnalyze::lowpass_filter(double cutoff)
{
	//Check the state
	if (this->state != eDataCopied)
		return eAnalyzer_NotReadyYet;

	//Write debug message
	#ifdef DEBUG_AUDIO
		my_console.WriteToSplitConsole("Applying low pass filter to captured data...", SPLIT_AUDIO);
		my_console.WriteToSplitConsole("Cutoff frequency = " + std::to_string(cutoff) + "Hz.", SPLIT_AUDIO);
	#endif

	//Simple first order lowpass filter
	//Calculate alpha out of the parameters
	double RC = 1.0 / (cutoff * 2 * M_PI);
	double dt = 1.0 / PCM_SAMPLE_RATE;
	double a  = dt  / (RC + dt);

	//Calculate low pass filtered data for left channel
	this->left_filt[0] = this->left[0];
	for (long i = 1; i < this->buffer_size / 2; ++i)
	{
		this->left_filt[i] = this->left_filt[i - 1] + (a * (this->left[i] - this->left_filt[i - 1]));
	}

	//Calculate low pass filtered data for right channel
	this->right_filt[0] = this->right[0];
	for (long i = 1; i < this->buffer_size / 2; ++i)
	{
		this->right_filt[i] = this->right_filt[i - 1] + (a * (this->right[i] - this->right_filt[i - 1]));
	}

	//Set the state - filtering done
	this->state = eFilteringDone;

	return eSuccess;
}

eError BPMAnalyze::combine_filtered_data()
{
	//Check the state
	if (this->state != eFilteringDone)
		return eAnalyzer_NotReadyYet;

	//Put filtered data together into one buffer
	for(long i = 0; i < this->buffer_size; i += 2)
	{
		this->data_out[i]     = this->left_filt[i / 2];
		this->data_out[i + 1] = this->right_filt[i / 2];
	}

	//Set the state - out data prepared
	this->state = eDataCombined;

	return eSuccess;
}

short* BPMAnalyze::get_filtered_data()
{
	//Check the state
	if (this->state != eFilteringDone && this->state != eDataCombined)
		return nullptr;

	//Put filtered data together into one buffer
	for(long i = 0; i < this->buffer_size; i += 2)
	{
		this->data_out[i]     = this->left_filt[i / 2];
		this->data_out[i + 1] = this->right_filt[i / 2];
	}
	
	return this->data_out;
}

short BPMAnalyze::get_max_sample_value(short* buffer, long size)
{
	//This function retrieves the max value from the buffer
	short max_value = SHORT_MIN;
	for(long i = 0; i < size; ++i)
	{
		if (buffer[i] > max_value)
			max_value = buffer[i];
	}
	
	return max_value;	
}

short BPMAnalyze::get_min_sample_value(short* buffer, long size)
{
	//This function retrieves the min value from the buffer
	short min_value = SHORT_MAX;
	for(long i = 0; i < size; ++i)
	{
		if (buffer[i] < min_value)
			min_value = buffer[i];
	}
	
	return min_value;	
}

eError BPMAnalyze::maximize_volume()
{
	//Check the state
	if (this->state != eDataCombined)
		return eAnalyzer_NotReadyYet;

	//Get max and min sample values of buffer
	short max = get_max_sample_value(this->data_out, this->buffer_size);
	short min = get_min_sample_value(this->data_out, this->buffer_size);
	
	//Declare maximization factor
	double factor;
	
	//Check which one is bigger
	if (max >= abs(min))
		factor = (double)SHORT_MAX / (double)max;
	else
		factor = abs((double)SHORT_MIN / (double)min);

	//Maximize the buffer
	for(long i = 0; i < this->buffer_size; ++i)
	{
		this->data_out[i] = (short)(this->data_out[i] * factor);
	}	

	return eSuccess;	
}

double BPMAnalyze::get_mean_value(short* buffer, long size)
{
	//Calculate average value of all values in the buffer
	double avg = 0.0;

	//Go through values and calculate mean value
	for(long i = 0; i < size; ++i)
		avg += ((double)buffer[i] / size);

	return avg;
}

double BPMAnalyze::get_variance_value(short* buffer, long size)
{
	//Calculate the variance of all the samples
	//(it's actually the squared variance)
	double var = 0.0;

	//Get average value
	double avg = get_mean_value(buffer, size);

	//Go through values and calculate variance
	for(long i = 0; i < size; ++i)
		var += (pow((double)buffer[i] - avg, 2.0) / size);

	return var;
}

double BPMAnalyze::perform_auto_correlation(double lag)
{
	//Write debug message
	//#ifdef DEBUG_AUDIO
	//	my_console.WriteToSplitConsole("Calculating auto correlation, lag = " + std::to_string(lag) + "s.", SPLIT_AUDIO);
	//#endif

	//Compute the autocorrelation
	//The outcome is a double value, telling us how much self-similarity the signal has
	//if we compare it to a time-shifted copy from itself (shifted by 'lag' seconds)
	
	//Calculate time axis - lag is in seconds
	double time_max = this->buffer_size / PCM_SAMPLE_RATE / PCM_CHANNELS;

	//Calculate number of samples corresponding to desired lag - division /2 due to left-right
	long lag_samples = ceil((lag / time_max) * this->buffer_size / 2);

	//Calculate average
	short* buffer = this->left_filt;
	double average = get_mean_value(buffer, this->buffer_size / 2);

	//Calculate variance
	double variance = get_variance_value(buffer, this->buffer_size / 2);

	//Calculate autocorrelation
	double autocorr = 0.0;
	for(long i = 0; i < this->buffer_size / 2 - lag_samples; ++i)
		autocorr += (((double)buffer[i] - average) * ((double)buffer[i + lag_samples] - average));

	autocorr = autocorr / (this->buffer_size / 2 - lag_samples);
	autocorr = autocorr / variance;
	autocorr *= autocorr;
	
	//Write debug message
	//#ifdef DEBUG_AUDIO
	//	my_console.WriteToSplitConsole("Max time value  = " + std::to_string(time_max) + "s.", SPLIT_AUDIO);
	//	my_console.WriteToSplitConsole("Lag samples     = " + std::to_string(lag_samples), SPLIT_AUDIO);
	//	my_console.WriteToSplitConsole("Average value   = " + std::to_string(average), SPLIT_AUDIO);
	//	my_console.WriteToSplitConsole("Variance value  = " + std::to_string(variance), SPLIT_AUDIO);
	//	my_console.WriteToSplitConsole("Autocorrelation = " + std::to_string(autocorr), SPLIT_AUDIO);
	//#endif

	return autocorr;
}

double BPMAnalyze::extract_bpm_value()
{
	//Check state
	if (this->state != eFilteringDone)
		return 0.0;

	//Declare return value
	double bpm_value = 0.0;
	long max_array_value = 0;

	//Calculate lag values -> beats/minute to seconds/beat
	double min_lag = (double)60 / (double)BPM_MAX_VALUE;
	double max_lag = (double)60 / (double)BPM_MIN_VALUE;
	double lag;	

	//Moving average filtered values
	double mov_avg[AUTOCORR_RES - MOV_AVG_SIZE];
	double ma_value = 0.0;

	for (long i = 0; i < AUTOCORR_RES; ++i)
	{
		lag = min_lag + (max_lag - min_lag) / (double)AUTOCORR_RES * (double)i;
		this->autocorr_array[i] = perform_auto_correlation(lag);

		//Calculate moving average. Start if correct number of calculations
		//have been made.
		if (i >= MOV_AVG_SIZE)
		{
			for (int j = 0; j < MOV_AVG_SIZE; j++)
			{
				ma_value = ma_value + autocorr_array[i - MOV_AVG_SIZE + j] / MOV_AVG_SIZE;
			}
			mov_avg[i - MOV_AVG_SIZE] = ma_value;
			ma_value = 0.0;

			//Save max value
			if (mov_avg[i - MOV_AVG_SIZE] > max_array_value)
			{
				max_array_value = i;
				bpm_value = mov_avg[i - MOV_AVG_SIZE];
			}
		}
	}

	//The lag of the moving average filter must be compensated.
	//The max index value is shifted.
	max_array_value = (long)((double)max_array_value * AUTOCORR_RES / (AUTOCORR_RES - MOV_AVG_SIZE));

	//Return the calculated bpm value from the max index
	double bpm_lag = min_lag + (max_lag - min_lag) / AUTOCORR_RES * max_array_value;

	//We set state 1 to indicate that calculation has finished and buffer can be overwritten again
	this->state = eReadyForData;
	
	return 60.0 / bpm_lag;
}