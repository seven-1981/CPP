#include <iostream>
#include <string>
#include <vector>
#include <cstdio>

#include "WAVFile.h"
#include "DSP.h"
//Uses boost library. In Project options set include path to c:\program files\boost
//and library path to C:\Program Files\Boost\stage\lib
//boost lib files must be created first with b2.exe and bjam.exe
//In addition, the define _CRT_SECURE_NO_WARNINGS must be added to the preprocessor defines
#include "gnuplot-iostream.h"

//Defines for bpm detection
#define BPM_MAX_VALUE 200.0
#define BPM_MIN_VALUE 80.0
#define AUTOCORR_RES 3000
#define MOV_AVG_SIZE 50
#define ENV_FILT_REC 0.005

//Frequency pass band
#define LO_FREQ 20.0
#define HI_FREQ 250.0

//Defines for Plots
#define PLOT_WAVFILE
#define PLOT_FFT
//#define PLOT_INV_FFT
#define PLOT_AUTOCORR


int main()
{
	//************************
	//* VARIABLE DECLARATION *
	//************************
	long sample_rate;			//Wavfile sample rate
	long pow2samples;			//Number of samples truncated or extended to 2^n size

	double freqres;				//Frequency resolution

	double bpm_value;			//Final bpm value

	//************************
	//* WAV FILE DECLARATION *
	//************************
	WAVFile wavfile;			//For analysis
	WAVFile wavfile_filtered;	//Wavfile after filtering process

	//**********************
	//* BUFFER DECLARATION *
	//**********************
	buffer<double> wavfile_buffer;		//Buffer with read wavfile data
	buffer<short> time_domain;			//Buffer with time domain data - truncated to 2^n size
	buffer<double> freq_domain;			//Buffer with frequency spectrum data
	buffer<double> freq_filt;			//Buffer with modified frequency data
	buffer<double> time_filt;			//Buffer with DFT filtered time signal
	buffer<short> wavfile_filt_buffer;	//Buffer with DFT filtered wav data

	buffer<double> autocorr_array;		//Array with autocorrelation values
	buffer<double> autocorr_filt;		//Array with filtered autocorrelation values (moving average)
	buffer<double> env_filt;			//Array with envelope filtered autocorr data

	//*****************
	//* WAV FILE READ *
	//*****************
	std::ifstream read_wav("fast170.wav", std::ios_base::in | std::ios_base::binary);
	if (read_wav.is_open() == false)
		exit(-1);
	wavfile.read_wav_file(read_wav);
	read_wav.close();
	std::cout << "The last frame is: " << (*wavfile.get_buffer())[wavfile.get_size() - 1] << std::endl;
	sample_rate = wavfile.get_header_info()->sample_rate;

	//******************************************
	//* APPLY WINDOW TO WAV DATA (TIME DOMAIN) *
	//******************************************
	wavfile_buffer.init_buffer(wavfile.get_size(), sample_rate);
	apply_window(*wavfile.get_buffer(), wavfile_buffer, tukey, 0.5);

	//*************************
	//* PLOT WINDOWED WAVFILE *
	//*************************
#ifdef PLOT_WAVFILE
	Gnuplot gp1("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp1 << "plot '-' with lines\n";
	std::vector<double> data1;
	long size1 = wavfile_buffer.get_size();
	for (long i = 0; i < size1; i++)
		data1.push_back(wavfile_buffer[i]);
	gp1.send1d(data1);
#else
	remove("windata.txt");
	std::ofstream windowdata("windata.txt", std::ios_base::out | std::ios_base::trunc);
	for (long i = 0; i < wavfile_buffer.get_size(); i++)
		windowdata << wavfile_buffer[i] << "\n";
	windowdata.close();
#endif

	//*************************
	//* APPLY FFT TO WAV DATA *
	//*************************
	pow2samples = pow2_size(wavfile_buffer, true); // true for floor, false for ceil
	std::cout << "Pow 2 size: " << pow2samples << "." << std::endl;
	time_domain.init_buffer(pow2samples, sample_rate);
	freq_domain.init_buffer(pow2samples * 2, sample_rate);
	create_fft_buffer(*wavfile.get_buffer(), time_domain);
	//create_fft_buffer(wavfile_buffer, time_domain); //<- uses windowed data
	perform_fft(time_domain, freq_domain, +1);
	freqres = (double)sample_rate / pow2samples;
	std::cout << "The freq. resolution: " << freqres << std::endl;

	//************************
	//* PLOT FFT SPECTROGRAM *
	//************************
#ifdef PLOT_FFT
	Gnuplot gp2("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp2 << "plot '-' with lines\n";
	std::vector<std::pair<double, double>> pair;
	for (long i = 0; i < 5000; i++)
		pair.push_back(std::pair<double, double>((double)i / 2 * freqres, 1.0 / pow2samples * sqrt(freq_domain[i] * freq_domain[i] + freq_domain[i + 1] * freq_domain[i + 1])));
	gp2.send1d(pair);
#else
	remove("fftdata.txt");
	std::ofstream fftdata("fftdata.txt", std::ios_base::out | std::ios_base::trunc);
	for (long i = 0; i < pow2samples; i += 2)
		fftdata << (double)i / 2 * freqres << ", " << 1.0 / pow2samples * sqrt(freq_domain[i] * freq_domain[i] + freq_domain[i + 1] * freq_domain[i + 1]) << "\n";
	fftdata.close();
#endif

	//*****************************
	//* REMOVE FREQ FROM FFT DATA *
	//*****************************
	freq_filt.init_buffer(pow2samples, sample_rate);
	cut_freq(freq_domain, freq_filt, LO_FREQ, HI_FREQ);

	//***********************
	//* PERFORM INVERSE FFT *
	//***********************
	time_filt.init_buffer(pow2samples * 2, sample_rate);
	perform_fft(freq_filt, time_filt, -1);

#ifdef PLOT_INV_FFT
	Gnuplot gp3("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp3 << "plot '-' with lines\n";
	std::vector<double> data3;
	for (long i = 0; i < pow2samples; i++)
		data3.push_back(1.0 / (double)sample_rate * time_filt.values[i]);
	gp3.send1d(data3);
#endif

	//*****************************
	//* RESTORE FILTERED WAV DATA *
	//*****************************
	wavfile_filt_buffer.init_buffer(pow2samples, sample_rate);
	for (long i = 0; i < pow2samples; i++)
		wavfile_filt_buffer[i] = (short)(1.0 / (double)sample_rate * time_filt[i]);

	//*********************************
	//* MAXIMIZE VOLUME/LEVEL OF DATA *
	//*********************************
	maximize_volume(wavfile_filt_buffer);

	//***************************
	//* WRITE FILTERED WAV FILE *
	//***************************
	wavfile_filtered.set_buffer(wavfile_filt_buffer);
	remove("./wavs/filtwav.wav");
	std::ofstream filtwav("./wavs/filtwav.wav", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	wavfile_filtered.write_wav_file(filtwav);
	filtwav.close();

	//*******************************************
	//* PERFORM AUTOCORRELATION AND EXTRACT BPM *
	//*******************************************
	autocorr_array.init_buffer(AUTOCORR_RES, sample_rate);
	env_filt.init_buffer(AUTOCORR_RES, sample_rate);
	autocorr_filt.init_buffer(AUTOCORR_RES, sample_rate);
	build_autocorr_array(wavfile_filt_buffer, autocorr_array, BPM_MIN_VALUE, BPM_MAX_VALUE);
	moving_average(autocorr_array, autocorr_filt, MOV_AVG_SIZE);
	envelope_filter(autocorr_array, env_filt, ENV_FILT_REC);
	bpm_value = extract_bpm_value(env_filt, BPM_MIN_VALUE, BPM_MAX_VALUE);
	std::cout << "The BPM value is: " << bpm_value << std::endl;

	//*****************
	//* AUTOCORR PLOT *
	//*****************
#ifdef PLOT_AUTOCORR
	Gnuplot gp4("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp4 << "plot '-' with lines\n";
	std::vector<double> data4;
	for (long i = 0; i < AUTOCORR_RES; i++)
		data4.push_back(autocorr_filt[i]);
	gp4.send1d(data4);
#endif

	return 0;
}