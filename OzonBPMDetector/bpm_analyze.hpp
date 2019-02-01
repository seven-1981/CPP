#ifndef _BPM_ANALYZE
#define _BPM_ANALYZE

#include <iostream>
#include <chrono>
#include <mutex>
#include "bpm_globals.hpp"
#include "buffer.hpp"
#include "BiquadCascade.hpp"

//Enum for analyzer state
enum eAnalyzerState
{
	eNotInitialized,			//Not initialized, not ready for calculation yet
	eReadyForData,				//Initialization or calculation is done, waiting for data
	eDataIsBeingCopied,			//Data transfer is in progress
	eDataCopyFinished,			//Copy of data has been finished
	eCalculationInProgress			//Data is copied (further data will be ignored) and calculation is ongoing
};

//Class for audio analysis
//Function get_bpm_value
//Analysis is done using the following algorithms:
//1. Get buffer from bpm_audio (short*)
//2. Perform FFT of buffer
//3. Cut desired frequency range
//4. IFFT to get filtered audio data
//5. Calculate autocorrelation for desired BPM range
//6. Apply envelope filter to autocorrelation data
//7. Detect peak in autocorrelation and calculate BPM value
//8. Return BPM value
//Note: this algo is suboptimal, as cutting out frequency bin range is not equal to
//deleting those frequencies from the signal. In fact it's the same as adding frequency
//components of multiple of frequency resolution with phase opposition, thus giving some
//aliasing effects in the lower frequencies -> strange sounds.

//Function get_bpm_value_adv
//This is a more advanced algo, as it uses biquad filter bands to isolate bass frequency and
//makes use of the PEAKS.hpp to determine the peaks. Filtering should be done prior to downsampling
//because of aliasing. Filter is designed with IOWA IIR filter design tool and coefficients are
//read from the coefficients text file (output from filter tool).
//1. Get buffer from bpm_audio (short*)
//2. Apply biquad filtering
//3. Downsampling
//4. Envelope
//5. Autocorrelation
//6. BPM extraction
//Note: this algo uses the "PEAK.hpp" functions. For every passband, a number of most explicit
//peaks are determined and the peaks are compared with each other to find the most "confident" peak.
//See "PEAKS.hpp" for further info.

class BPMAnalyze
{
public:
	//Constructor for analysis class
	BPMAnalyze();

	//Destructor for class
	~BPMAnalyze();

	//Method for handover of audio data
	//Gets the sampled data from class BPMAudio and stores it in a buffer<T> for DSP
	eError prepare_audio_data(short* data);

	//Methods for extracting the BPM value from the data
	//Note: This triggers the chain of algorithms
	//Main method - this one calls different algos depending on parameter
	double get_bpm_value();
	double get_bpm_value_0();
	double get_bpm_value_1();

	//Method for rms value evaluation
	//If value is below a threshold, no bpm analysis is possible
	eError check_rms_value();

	//Getter method
	eAnalyzerState get_state() { return this->state; }
	//Setter method to reset calculation
	eError reset_state();

	//Time measurement - returns the duration of the last
	//executed bpm calculation
	long long get_calc_time();
	void tic();

private:
	//Duration of one buffer
	long duration;
	//Sample rate of buffers
	long sample_rate;
	//Sample rate - after downsampling
	long sample_rate_DS;

	//Size of buffer must be multiple of 2^N
	long pow2samples;
	//Size of buffer - after downsampling
	long pow2samples_DS;

	//Frequency resolution - this one remains unchanged after downsampling
	double freqres;

	//State of analyzer - used to ensure proper sequencing of commands
	//and to avoid operations on unitialized data/memory
	eAnalyzerState state = eNotInitialized;

	// Buffer used for audio record class data
	buffer<short> bf;

	//Internal buffers used for basic calculation
	buffer<short> time_domain;			//Buffer with time domain data - truncated/padded to 2^n size
	buffer<short> time_downsample;			//Buffer with downsampled time domain data
	buffer<double> freq_domain;			//Buffer with frequency spectrum data
	buffer<double> freq_filt;			//Buffer with modified frequency data
	buffer<double> time_filt;			//Buffer with DFT filtered time signal
	buffer<double> autocorr_array;			//Array with autocorrelation values
	buffer<double> env_filt;			//Array with envelope filtered autocorr data

	//Biquad filters - only two bands used at the moment
	BiquadCascade* passband_L;			//Low passband
	//BiquadCascade* passband_M;			//Mid passband
	BiquadCascade* passband_H;			//High passband

	//Internal buffers used for biquad calculation
	buffer<double> biquad_buffer_L;			//After biquad filter process
	//buffer<double> biquad_buffer_M;
	buffer<double> biquad_buffer_H;
	buffer<double> biquad_buffer_DS_L;		//After downsampling
	//buffer<double> biquad_buffer_DS_M;
	buffer<double> biquad_buffer_DS_H;
	buffer<double> biquad_buffer_env;		//Envelope buffer - common
	buffer<double> biquad_buffer_autocorr_L;	//After autocorrelation
	//buffer<double> biquad_buffer_autocorr_M;
	buffer<double> biquad_buffer_autocorr_H;

	//Time measurement
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::chrono::time_point<std::chrono::high_resolution_clock> stop;

	//Mutex for multi-thread handling
	//The state can be accessed from multiple locations
	//Therefore we must use this in any function that sets the state
	std::mutex mtx;

	//Debug functions
	void write_debug_files();
};

#endif