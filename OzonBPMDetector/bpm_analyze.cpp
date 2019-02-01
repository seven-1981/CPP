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
#include <vector>
#include <fstream>
#include "bpm_analyze.hpp"
#include "bpm_globals.hpp"
#include "bpm_param.hpp"
#include "SplitConsole.hpp"
#include "PEAKS.hpp"
#include "WAVFile.h"

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

BPMAnalyze::BPMAnalyze()
{
	//Constructor for analyzer class instance
	if (param_list.get<bool>("debug analyze") == true)
		my_console.WriteToSplitConsole("BPM Analyzer Class: Instantiating audio analyzer class.", param_list.get<int>("split audio"));

	//Store sample rate information
	this->sample_rate = PCM_SAMPLE_RATE;
	this->sample_rate_DS = PCM_SAMPLE_RATE / DOWNSAMPLE_FACTOR;
	//Store duration information - in seconds
	this->duration = PCM_BUF_SIZE / PCM_CHANNELS / PCM_SAMPLE_RATE;

	//Initialize data buffer
	this->bf.init_buffer(sample_rate * duration, sample_rate);
	// Get size of FFT buffers and freq resolution
	this->pow2samples = DSP::pow2_size(bf, true);
	this->pow2samples_DS = this->pow2samples / DOWNSAMPLE_FACTOR;
	this->freqres = (double)sample_rate / pow2samples;
	// Initialize processing buffers
	this->time_domain.init_buffer(pow2samples, sample_rate);
	this->time_downsample.init_buffer(pow2samples_DS, sample_rate_DS);
	this->freq_domain.init_buffer(pow2samples_DS * 2, sample_rate_DS);
	this->freq_filt.init_buffer(pow2samples_DS, sample_rate_DS);
	this->time_filt.init_buffer(pow2samples_DS * 2, sample_rate_DS);
	this->autocorr_array.init_buffer(AUTOCORR_RES, sample_rate_DS);
	this->env_filt.init_buffer(AUTOCORR_RES, sample_rate_DS);

	//Initialize biquad filter
	this->passband_L = new BiquadCascade(BIQ_FILT_ORDER);
	this->passband_H = new BiquadCascade(BIQ_FILT_ORDER);
	//Read coefficients
	std::ifstream coeff_file_L(FN_COEFFS_L, std::ios_base::in);
	std::ifstream coeff_file_H(FN_COEFFS_H, std::ios_base::in);
	this->passband_L->get_param(coeff_file_L);
	this->passband_H->get_param(coeff_file_H);
	coeff_file_L.close();
	coeff_file_H.close();

	//Initialize biquad buffers
	this->biquad_buffer_L.init_buffer(sample_rate * duration, sample_rate);
	this->biquad_buffer_H.init_buffer(sample_rate * duration, sample_rate);
	this->biquad_buffer_DS_L.init_buffer(sample_rate_DS * duration, sample_rate_DS);
	this->biquad_buffer_DS_H.init_buffer(sample_rate_DS * duration, sample_rate_DS);
	this->biquad_buffer_env.init_buffer(sample_rate_DS * duration, sample_rate_DS);
	this->biquad_buffer_autocorr_L.init_buffer(AUTOCORR_RES, sample_rate_DS);
	this->biquad_buffer_autocorr_H.init_buffer(AUTOCORR_RES, sample_rate_DS);

	//Initialize timestamps
	this->start = std::chrono::high_resolution_clock::now();
	this->stop = std::chrono::high_resolution_clock::now();

	//Set state - constructor executed, instance created
	this->state = eReadyForData;
}

BPMAnalyze::~BPMAnalyze()
{
	//Constructor for audio analyer instance
	if (param_list.get<bool>("debug analyze") == true)
		my_console.WriteToSplitConsole("BPM Analyzer Class: Releasing audio analyzer resources.", param_list.get<int>("split audio"));

	//Buffers free their memory upon destructor's call
	
	//Delete biquads
	delete this->passband_L;
	delete this->passband_H;
}

eError BPMAnalyze::reset_state()
{
	//Reset the analyzer - use mutex because state is accessed
	this->mtx.lock();
	this->state = eReadyForData;
	this->mtx.unlock();

	return eSuccess;
}

eError BPMAnalyze::prepare_audio_data(short* data)
{
	//Check if init has finished 
	if (this->state == eNotInitialized)
		return eAnalyzer_NotInitialized;

	//Check if analyzer is ready to get data
	if (this->state != eReadyForData)
		return eAnalyzer_NotReadyYet;

	//Lock mutex - section after here writes state member
	this->mtx.lock();

	//We can start copying the data
	this->state = eDataIsBeingCopied;

	//Write debug message
	if (param_list.get<bool>("debug analyze") == true)
		my_console.WriteToSplitConsole("BPM Analyzer Class: Preparing audio data.", param_list.get<int>("split audio"));
	
	//Read PCM samples
	long size = this->duration * this->sample_rate;
	for (long i = 0; i < size; i++)
		this->bf[i] = data[i];	

	//Set the state - data is ready
	this->state = eDataCopyFinished;

	//Unlock mutex
	this->mtx.unlock();

	return eSuccess;
}

double BPMAnalyze::get_bpm_value()
{
	//Check parameter value and call according method
	if (param_list.get<int>("algorithm") == 0)
		return this->get_bpm_value_0();
	if (param_list.get<int>("algorithm") == 1)
		return this->get_bpm_value_1();
	
	//If algo not found, nevertheless return value
	return 0.0;
}

double BPMAnalyze::get_bpm_value_0()
{
	//Check state - only possible if state is eDataCopyFinished
	if (this->state != eDataCopyFinished)
		return 0.0;

	//Lock mutex - section after here writes state member
	this->mtx.lock();

	//We may start the calculation
	this->state = eCalculationInProgress;

	//Declare return value
	double bpm_value = 0.0;

	//Get params
	double bpm_max = param_list.get<double>("bpm max");
	double bpm_min = param_list.get<double>("bpm min");
	double env_filt_rec = param_list.get<double>("env filt rec");
	double lo_freq = param_list.get<double>("lo freq");
	double hi_freq = param_list.get<double>("hi freq");

	//Save timestamp
	this->start = std::chrono::high_resolution_clock::now();

	//Downsample and Perform FFT
	DSP::create_fft_buffer(this->bf, this->time_domain);
	DSP::downsample_buffer(this->time_domain, this->time_downsample, DOWNSAMPLE_FACTOR);
	DSP::perform_fft(this->time_downsample, this->freq_domain, +1);

	//Cut non relevant frequencies
	DSP::cut_freq(this->freq_domain, this->freq_filt, lo_freq, hi_freq);
	//Inverse FFT
	DSP::perform_fft(this->freq_filt, this->time_filt, -1);
	//Scaling of the time values
	DSP::gain(this->time_filt, 1.0 / (double)this->sample_rate);

	//Perform autocorrelation
	DSP::build_autocorr_array(this->time_filt, this->autocorr_array, bpm_min, bpm_max);

	//Add envelope filtering
	DSP::envelope_filter(this->autocorr_array, this->env_filt, env_filt_rec);

	//Extract bpm value
	bpm_value = DSP::extract_bpm_value(this->env_filt, bpm_min, bpm_max);
	
	//Stop timestamp
	this->stop = std::chrono::high_resolution_clock::now();

	//Set state and return the calculated BPM value
	this->state = eReadyForData;

	//Unlock mutex
	this->mtx.unlock();

	//Write debug message
	if (param_list.get<bool>("debug analyze") == true)
		my_console.WriteToSplitConsole("BPM Analyzer Class: Calculated BPM value = " + std::to_string(bpm_value) + "bpm.", param_list.get<int>("split audio"));

	return bpm_value;
}

double BPMAnalyze::get_bpm_value_1()
{
	//Check state - only possible if state is eDataCopyFinished
	if (this->state != eDataCopyFinished)
		return 0.0;

	//Lock mutex - section after here writes state member
	this->mtx.lock();

	//We may start the calculation
	this->state = eCalculationInProgress;

	//Declare return value
	double bpm_value = 0.0;

	//Get params and activate debug
	if (param_list.get<bool>("create peak data") == true)
		PEAKS::activate_debug();
	else
		PEAKS::deactivate_debug();

	double bpm_max = param_list.get<double>("bpm max");
	double bpm_min = param_list.get<double>("bpm min");
	double env_filt_rec = param_list.get<double>("env filt rec");
	double width = param_list.get<double>("peak width");
	double threshold = param_list.get<double>("peak threshold");
	double adj = param_list.get<double>("peak adjacence");

	//Save timestamp
	this->start = std::chrono::high_resolution_clock::now();

	//Biquad filter cascade
	long size = this->duration * this->sample_rate;
	for (long i = 0; i < size; i++)
	{
		this->biquad_buffer_L[i] = this->passband_L->process(this->bf[i]);
		this->biquad_buffer_H[i] = this->passband_H->process(this->bf[i]);
	}

	//After filter process, reset the filters
	//this->passband_L->reset();
	//this->passband_H->reset();

	//Downsample
	DSP::downsample_buffer(this->biquad_buffer_L, this->biquad_buffer_DS_L, DOWNSAMPLE_FACTOR);
	DSP::downsample_buffer(this->biquad_buffer_H, this->biquad_buffer_DS_H, DOWNSAMPLE_FACTOR);

	//LOW PASSBAND
	//Envelope
	DSP::envelope_filter(this->biquad_buffer_DS_L, this->biquad_buffer_env, env_filt_rec);
	//Autocorrelation
	DSP::build_autocorr_array(this->biquad_buffer_env, this->biquad_buffer_autocorr_L, bpm_min, bpm_max);
	
	//HIGH PASSBAND
	//Envelope
	DSP::envelope_filter(this->biquad_buffer_DS_H, this->biquad_buffer_env, env_filt_rec);
	//Autocorrelation
	DSP::build_autocorr_array(this->biquad_buffer_env, this->biquad_buffer_autocorr_H, bpm_min, bpm_max);

	//Debug output of autocorr arrays and wavfiles
	write_debug_files();
	
	//BPM extraction
	//Build vector with buffers
	std::vector<buffer<double>*> buffers;
	buffers.push_back(&this->biquad_buffer_autocorr_L);
	buffers.push_back(&this->biquad_buffer_autocorr_H);

	//PARAMETERS - to be adapted
	std::vector<double> widths; widths.push_back(width); widths.push_back(width);
	std::vector<double> thres; thres.push_back(threshold); thres.push_back(threshold);
	PEAKS::params bpm_params(bpm_min, bpm_max, widths, thres, DSP::weight, (unsigned int)adj);

	//Extract bpm value
	bpm_value = PEAKS::extract_bpm_value(buffers, bpm_params);
	
	//Stop timestamp
	this->stop = std::chrono::high_resolution_clock::now();

	//Set state and return the calculated BPM value
	this->state = eReadyForData;

	//Unlock mutex
	this->mtx.unlock();

	//Write debug message
	if (param_list.get<bool>("debug analyze") == true)
		my_console.WriteToSplitConsole("BPM Analyzer Class: Calculated BPM value = " + std::to_string(bpm_value) + "bpm.", param_list.get<int>("split audio"));

	return bpm_value;
}

eError BPMAnalyze::check_rms_value()
{
	//Declare return value
	eError retval = eAnalyzer_RMSBelowThreshold;

	//Check state - only possible if state is eDataCopyFinished
	if (this->state != eDataCopyFinished)
		return eAnalyzer_NotReadyYet;

	//Calculate the rms value of the recorded data
	//If below threshold, no analysis is possible
	double rms = DSP::get_rms_value(this->bf);

	//Write debug message
	if (param_list.get<bool>("debug analyze") == true)
		my_console.WriteToSplitConsole("BPM Analyzer Class: RMS value = ", rms, param_list.get<int>("split audio"));

	//Check threshold and modify return value if necessary
	if (rms > param_list.get<double>("rms threshold"))
		retval = eSuccess;

	return retval;
}

long long BPMAnalyze::get_calc_time()
{
	//Determine the timestamp difference
	long long us = std::chrono::duration_cast<std::chrono::microseconds>(this->stop - this->start).count();
	
	//Write debug message
	if (param_list.get<bool>("debug analyze") == true)
		my_console.WriteToSplitConsole("BPM Analyzer Class: Processing time = " + std::to_string(us / 1000.0) + "ms.", param_list.get<int>("split audio"));

	return us;
}

void BPMAnalyze::tic()
{
	std::chrono::time_point<std::chrono::high_resolution_clock> yet;
	yet = std::chrono::high_resolution_clock::now();
	long long us = std::chrono::duration_cast<std::chrono::microseconds>(yet - this->start).count();
	
	//Output of lap time
	my_console.WriteToSplitConsole("BPM Analyzer Class: Lap time = " + std::to_string(us / 1000.0) + "ms.", param_list.get<int>("split audio"));
}

void BPMAnalyze::write_debug_files()
{
	if (param_list.get<bool>("create wavfiles") == true)
	{
		//Debug code for wav file generation
		buffer<short> wavfile_buffer_L;
		buffer<short> wavfile_buffer_H;
		wavfile_buffer_L.init_buffer(biquad_buffer_L.get_size(), biquad_buffer_L.get_sample_rate());
		wavfile_buffer_H.init_buffer(biquad_buffer_H.get_size(), biquad_buffer_H.get_sample_rate());
		DSP::shortify(biquad_buffer_L, wavfile_buffer_L);
		DSP::shortify(biquad_buffer_H, wavfile_buffer_H);
		WAVFile wavfile_L;
		WAVFile wavfile_H;
		wavfile_L.set_buffer(wavfile_buffer_L);
		wavfile_H.set_buffer(wavfile_buffer_H);
		static int count = 0;
		std::string filename_L = "filt_Lwav" + std::to_string(count) + ".wav";
		std::string filename_H = "filt_Hwav" + std::to_string(count) + ".wav";
		std::ofstream file_L(filename_L, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		std::ofstream file_H(filename_H, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		wavfile_L.write_wav_file(file_L);
		wavfile_H.write_wav_file(file_H);
		file_L.close();
		file_H.close();
	
		WAVFile wavfile;
		wavfile.set_buffer(this->bf);
		std::string filename = "raw_wav" + std::to_string(count++) + ".wav";
		std::ofstream file(filename, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		wavfile.write_wav_file(file);
		file.close();
	}

	if (param_list.get<bool>("create autocorr files") == true)
	{
		static int counter = 0;
		std::string filename_autocorr_L = "ac_data_L" + std::to_string(counter) + ".txt";
		std::string filename_autocorr_H = "ac_data_H" + std::to_string(counter++) + ".txt";
		std::ofstream file_autocorr_L(filename_autocorr_L, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		std::ofstream file_autocorr_H(filename_autocorr_H, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		for (long i = 0; i < biquad_buffer_autocorr_L.get_size(); i++)
		{
			file_autocorr_L << i << "; " << biquad_buffer_autocorr_L[i] << "\n";
			file_autocorr_H << i << "; " << biquad_buffer_autocorr_H[i] << "\n";
		}
		file_autocorr_L.close();
		file_autocorr_H.close();
	}
}