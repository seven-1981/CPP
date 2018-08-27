#ifndef _BPM_ANALYZE
#define _BPM_ANALYZE

#include <iostream>
#include "bpm_globals.hpp"
#include "SplitConsole.hpp"

//Defines for analysis class
#define SHORT_MAX       32767
#define SHORT_MIN      -32768

//Defines for max and min bpm value
#define BPM_MIN_VALUE     100
#define BPM_MAX_VALUE     200

//Define autocorrelation resolution
#define AUTOCORR_RES       200

//Define moving average filter
#define MOV_AVG_SIZE     50

//Extern split console instance
extern SplitConsole my_console;

//Enum for analyzer state
enum eAnalyzerState
{
	eNotInitialized,
	eReadyForData,
	eDataCopied,
	eFilteringDone,
	eDataCombined
};

//Class for audio analysis
//The plan is to do the following with the captured PCM data from the sound card
//1. Prepare the data to do the calculation
//2. Apply filter
//3. Auto correlation - compare the data to a time-shifted copy of itself
//4. Analyze the output data and calculate BPM value out of it
class BPMAnalyze
{
public:
	//Constructor for analysis class
	BPMAnalyze();

	//Destructor for class
	~BPMAnalyze();

	//Method for handover of audio data
	//Gets the sampled data from class BPMAudio and stores it in audio_data;
	eError prepare_audio_data(short* data);

	//Method for putting together the filtered data
	eError combine_filtered_data();

	//Method for extracting the filtered data
	short* get_filtered_data();

	//Simple first order lowpass filter
	eError lowpass_filter(double cutoff);

	//Maximize the buffer - should increase the volume
	eError maximize_volume();

	//Perform auto correlation of the filtered data
	double perform_auto_correlation(double lag);

	//Extract bpm value from autocorrelation
	double extract_bpm_value();

	//Getter methods
	eAnalyzerState get_state() { return this->state; }

private:
	//State of analyzer - used to ensure proper sequencing of commands
	//and to avoid operations on unitialized data/memory
	eAnalyzerState state = eNotInitialized;

	//Sample buffers - sampled using BPMAudio::capture_samples()
	//Left and right channel buffer
	short* left;
	short* right;
	short* left_filt;
	short* right_filt;

	//Filtered audio data
	short* data_out;

	//Autocorrelated audio data
	short* data_auto;

	//Size info of captured array
	long buffer_size;

	//Autocorrelation values array
	double* autocorr_array;

	//Get max sample value
	short get_max_sample_value(short* buffer, long size);

	//Get min sample value
	short get_min_sample_value(short* buffer, long size);

	//Get mean value of samples
	double get_mean_value(short* buffer, long size);

	//Get variance of samples
	double get_variance_value(short* buffer, long size);
};

#endif