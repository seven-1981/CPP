#include <iostream>
#include <fstream>
#include <cmath>
#include "gnuplot-iostream.h"
#include "buffer.h"
#include "DSP.h"
#include "WAVFile.h"
#include "AudioRecord.h"

// Defines for bpm detection
#define BPM_MAX_VALUE 200.0
#define BPM_MIN_VALUE 80.0
#define AUTOCORR_RES 3000
#define MOV_AVG_SIZE 50
#define ENV_FILT_REC 0.005

// Frequency pass band
#define LO_FREQ 20.0
#define HI_FREQ 250.0

// RMS Threshold
#define RMS_THRESHOLD 12000

// Sample rate used
long sample_rate = 44100;
// Duration of each buffer
long duration = 2;

// Buffer used for audio record class data
buffer<short> bf;

buffer<short> time_domain;			//Buffer with time domain data - truncated to 2^n size
buffer<double> freq_domain;			//Buffer with frequency spectrum data
buffer<double> freq_filt;			//Buffer with modified frequency data
buffer<double> time_filt;			//Buffer with DFT filtered time signal
buffer<short> wavfile_filt;			//Buffer with DFT filtered wav data

buffer<double> autocorr_array;		//Array with autocorrelation values
buffer<double> autocorr_filt;		//Array with filtered autocorrelation values (moving average)
buffer<double> env_filt;			//Array with envelope filtered autocorr data

void plot(buffer<double>& buffer, double resolution, long samples)
{
	Gnuplot gp("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp << "plot '-' with lines\n";
	std::vector<std::pair<double, double>> pair;
	long size = buffer.get_size();
	for (long i = 0; i < size; i++)
		pair.push_back(std::pair<double, double>((double)i / 2 * resolution, 1.0 / samples * sqrt(freq_domain[i] * freq_domain[i] + freq_domain[i + 1] * freq_domain[i + 1])));
	gp.send1d(pair);
}

void plot(buffer<double>& buffer)
{
	Gnuplot gp("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp << "plot '-' with lines\n";
	std::vector<double> data;
	long size = buffer.get_size();
	for (long i = 0; i < size; i++)
		data.push_back(buffer[i]);
	gp.send1d(data);
}

void evaluate(const buffer<short>& bf)
{
	// Buffer size 2^N for FFT algorithm
	long pow2samples = pow2_size(bf, true);
	// Perform FFT
	create_fft_buffer(bf, time_domain);
	perform_fft(time_domain, freq_domain, +1);

	//plot(freq_domain, (double)sample_rate / pow2samples, pow2samples);

	// Cut non relevant frequencies
	cut_freq(freq_domain, freq_filt, 20.0, 250.0);
	// Inverse FFT
	perform_fft(freq_filt, time_filt, -1);
	//for (long i = 0; i < pow2samples; i++)
	//	wavfile_filt[i] = (short)(1.0 / (double)sample_rate * time_filt[i]);
	gain(time_filt, 1.0 / (double)sample_rate);

	// Perform autocorrelation and filter resulting array
	//build_autocorr_array(wavfile_filt, autocorr_array, BPM_MIN_VALUE, BPM_MAX_VALUE);
	build_autocorr_array(time_filt, autocorr_array, BPM_MIN_VALUE, BPM_MAX_VALUE);
	moving_average(autocorr_array, autocorr_filt, MOV_AVG_SIZE);
	envelope_filter(autocorr_array, env_filt, ENV_FILT_REC);

	//plot(env_filt);
	//plot(autocorr_filt);

	// Extract bpm value
	double bpm_value = extract_bpm_value(env_filt, BPM_MIN_VALUE, BPM_MAX_VALUE);
	std::cout << "The BPM value is: " << bpm_value << std::endl;
}

int main()
{
	// Initialize the used wav buffer
	bf.init_buffer(sample_rate * duration, sample_rate);
	// Get size of FFT buffers and freq resolution
	long pow2samples = pow2_size(bf, true);
	double freqres = (double)sample_rate / pow2samples;
	// Initialize processing buffers
	time_domain.init_buffer(pow2samples, sample_rate);
	freq_domain.init_buffer(pow2samples * 2, sample_rate);
	freq_filt.init_buffer(pow2samples, sample_rate);
	time_filt.init_buffer(pow2samples * 2, sample_rate);
	wavfile_filt.init_buffer(pow2samples, sample_rate);
	autocorr_array.init_buffer(3000, sample_rate);
	env_filt.init_buffer(3000, sample_rate);
	autocorr_filt.init_buffer(3000, sample_rate);

	// Create audio record instance
	AudioRecord AR(sample_rate, duration);
	// Start recording - asynchronous execution
	// Program continues after start() 
	std::cout << "START." << std::endl;
	if (AR.start() != 0)
		exit(-1);

	int index = 0;
	int done = 0;
	while (done != pow(2, NUM_OF_BUFFERS) - 1)
	{
		// Check if one of the buffers is already full
		if (AR.is_full(index) == true)
		{
			// Check index
			if (index > NUM_OF_BUFFERS)
				break;

			std::cout << "Buffer " << index << " is full." << std::endl;

			// Get data
			short* buf = AR.flush(index);

			// Create buffer from data
			for (long i = 0; i < sample_rate * duration; i++)
				bf[i] = buf[i];
	
			// Get rms value - level determination
			double rms_value = get_rms_value(bf);
			std::cout << "The RMS value is " << rms_value << std::endl;
			if (rms_value > RMS_THRESHOLD)
			{
				// Get the bpm value
				std::cout << "Analyzing." << std::endl;
				evaluate(bf);
			}
			else
			{
				std::cout << "Record level too low. No analysis possible." << std::endl;
			}

			// Set done flag
			done |= (0x0001 << index);

			// Increase index
			index++;
		}
	}
}
