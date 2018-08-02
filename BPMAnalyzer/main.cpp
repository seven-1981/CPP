#include <iostream>
#include <fstream>
#include "buffer.h"
#include "DSP.h"
#include "WAVFile.h"
#include "AudioRecord.h"

long sample_rate = 44100;
long duration = 2;

buffer<short> time_domain;			//Buffer with time domain data - truncated to 2^n size
buffer<double> freq_domain;			//Buffer with frequency spectrum data
buffer<double> freq_filt;			//Buffer with modified frequency data
buffer<double> time_filt;			//Buffer with DFT filtered time signal

buffer<double> autocorr_array;		//Array with autocorrelation values
buffer<double> autocorr_filt;		//Array with filtered autocorrelation values (moving average)
buffer<double> env_filt;			//Array with envelope filtered autocorr data

void evaluate(const buffer<short>& bf)
{
	long pow2samples = pow2_size(bf, true);
	create_fft_buffer(bf, time_domain);
	perform_fft(time_domain, freq_domain, +1);
	double freqres = (double)sample_rate / pow2samples;
	cut_freq(freq_domain, freq_filt, 20.0, 250.0);
	perform_fft(freq_filt, time_filt, -1);
	build_autocorr_array(freq_filt, autocorr_array, 100.0, 200.0);
	moving_average(autocorr_array, autocorr_filt, 50);
	envelope_filter(autocorr_array, env_filt, 0.005);
	double bpm_value = extract_bpm_value(env_filt, 100.0, 200.0);
	std::cout << "The BPM value is: " << bpm_value << std::endl;
}

int main()
{
	buffer<short> bf(sample_rate * duration, sample_rate);
	long pow2samples = pow2_size(bf, true);
	time_domain.init_buffer(pow2samples, sample_rate);
	freq_domain.init_buffer(pow2samples * 2, sample_rate);
	freq_filt.init_buffer(pow2samples, sample_rate);
	time_filt.init_buffer(pow2samples * 2, sample_rate);
	autocorr_array.init_buffer(3000, sample_rate);
	env_filt.init_buffer(3000, sample_rate);
	autocorr_filt.init_buffer(3000, sample_rate);

	AudioRecord AR(sample_rate, duration);
	std::cout << "START. " << AR.start() << std::endl;

	int index = 0;
	while (AR.is_full() == false)
	{
		// Check if one of the buffers is already full
		if (AR.is_full(index) == true)
		{
			std::cout << "Buffer " << index << " is full." << std::endl;
			// Increase index and get data
			if (++index > NUM_OF_BUFFERS)
				break;
			short* buf = AR.flush(index);
			// Create buffer from data
			for (long i = 0; i < sample_rate * duration; i++)
				bf[i] = buf[i];

			evaluate(bf);
		}
	}
}
