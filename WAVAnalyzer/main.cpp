#include <iostream>
#include <string>
#include <vector>
#include <cstdio>

#include "..\..\WAVAnalyzer\WAVAnalyzer\WAVFile.h"
#include "..\..\WAVAnalyzer\WAVAnalyzer\DSP.h"
#include "..\..\WAVAnalyzer\WAVAnalyzer\PEAKS.h"
#include "..\..\WAVAnalyzer\WAVAnalyzer\Biquad.h"
#include "..\..\WAVAnalyzer\WAVAnalyzer\BiquadCascade.h"

//Uses boost library. In Project options set include path to c:\program files\boost
//and library path to C:\Program Files\Boost\stage\lib
//boost lib files must be created first with b2.exe and bjam.exe
//In addition, the define _CRT_SECURE_NO_WARNINGS must be added to the preprocessor defines
#include "..\..\WAVAnalyzer\WAVAnalyzer\gnuplot-iostream.h"

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
//#define PLOT_WAVFILE
//#define PLOT_FFT
//#define PLOT_INV_FFT
//#define PLOT_AUTOCORR
//#define PLOT_WAVFILE_BIQUAD
#define PLOT_BIQUAD_AUTOCORR

template <typename T>
void plot(const buffer<T>& inbuffer, Gnuplot& gp)
{
	gp << "plot '-' with lines\n";
	std::vector<double> data;
	long size = inbuffer.get_size();
	for (long i = 0; i < size; i++)
		data.push_back(inbuffer[i]);
	gp.send1d(data);
}

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
	WAVFile wavfile_biquad;		//Wavfile after biquad filtering

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
	std::ifstream read_wav("med140.wav", std::ios_base::in | std::ios_base::binary);
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
	DSP::apply_window(*wavfile.get_buffer(), wavfile_buffer, DSP::tukey, 0.5);

	//*************************
	//* PLOT WINDOWED WAVFILE *
	//*************************
#ifdef PLOT_WAVFILE
	Gnuplot gp1("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	plot(wavfile_buffer, gp1);
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
	pow2samples = DSP::pow2_size(wavfile_buffer, true); // true for floor, false for ceil
	std::cout << "Pow 2 size: " << pow2samples << "." << std::endl;
	time_domain.init_buffer(pow2samples, sample_rate);
	freq_domain.init_buffer(pow2samples * 2, sample_rate);
	DSP::create_fft_buffer(*wavfile.get_buffer(), time_domain);
	//create_fft_buffer(wavfile_buffer, time_domain); //<- uses windowed data
	DSP::perform_fft(time_domain, freq_domain, +1);
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
	DSP::cut_freq(freq_domain, freq_filt, LO_FREQ, HI_FREQ);

	//***********************
	//* PERFORM INVERSE FFT *
	//***********************
	time_filt.init_buffer(pow2samples * 2, sample_rate);
	DSP::perform_fft(freq_filt, time_filt, -1);

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
	DSP::maximize_volume(wavfile_filt_buffer);

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
	DSP::build_autocorr_array(time_filt, autocorr_array, BPM_MIN_VALUE, BPM_MAX_VALUE);
	DSP::moving_average(autocorr_array, autocorr_filt, MOV_AVG_SIZE);
	DSP::envelope_filter(autocorr_array, env_filt, ENV_FILT_REC);
	bpm_value = DSP::extract_bpm_value(env_filt, BPM_MIN_VALUE, BPM_MAX_VALUE);
	std::cout << "The BPM value is: " << bpm_value << std::endl;

	//*****************
	//* AUTOCORR PLOT *
	//*****************
#ifdef PLOT_AUTOCORR
	Gnuplot gp4("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	plot(autocorr_filt, gp4);
#endif

	//****************
	//* DOWNSAMPLING *
	//****************
	buffer<short> time_downsample1;
	buffer<short> time_downsample2;
	buffer<double> freq_downsample;
	buffer<double> freq_downsample_filt;
	buffer<double> time_downsample_filt;
	buffer<double> autocorr_ds;
	buffer<double> env_downsample;

	// Calculate adapted sample rate and size
	long samples = pow2samples / 4;
	long srate = sample_rate / 4;
	time_downsample1.init_buffer(samples * 2, srate * 2);
	time_downsample2.init_buffer(samples, srate);
	DSP::downsample_buffer(time_domain, time_downsample2, 4);

	freq_downsample.init_buffer(samples * 2, srate);
	DSP::perform_fft(time_downsample2, freq_downsample, +1);

	freq_downsample_filt.init_buffer(samples, srate);
	time_downsample_filt.init_buffer(samples * 2, srate);
	DSP::cut_freq(freq_downsample, freq_downsample_filt, 20.0, 250.0);
	DSP::perform_fft(freq_downsample_filt, time_downsample_filt, -1);

	long autosize = AUTOCORR_RES;
	autocorr_ds.init_buffer(autosize, srate);
	DSP::build_autocorr_array(time_downsample_filt, autocorr_ds, BPM_MIN_VALUE, BPM_MAX_VALUE);

	env_downsample.init_buffer(autosize, srate);
	DSP::envelope_filter(autocorr_ds, env_downsample, ENV_FILT_REC);
	bpm_value = DSP::extract_bpm_value(env_downsample, BPM_MIN_VALUE, BPM_MAX_VALUE);
	std::cout << "The BPM downsampled value is: " << bpm_value << std::endl;

	//*************************
	//* WAVFILE BIQUAD FILTER *
	//*************************
	buffer<double> biquad_buffer_L;
	buffer<double> biquad_buffer_M;
	buffer<double> biquad_buffer_H;
	buffer<short> biquad_wavbuffer_L;
	buffer<short> biquad_wavbuffer_M;
	buffer<short> biquad_wavbuffer_H;
	buffer<double> biquad_autocorr_L;
	buffer<double> biquad_autocorr_M;
	buffer<double> biquad_autocorr_H;
	buffer<double> biquad_env;
	
	//Initialize Biquad Cascades
	BiquadCascade* pCascadeLow = new BiquadCascade(BiquadType_Bandpass, 12, 20.0 / srate, 80.0 / srate);
	BiquadCascade* pCascadeMid = new BiquadCascade(BiquadType_Bandpass, 12, 80.0 / srate, 140.0 / srate);
	BiquadCascade* pCascadeHigh = new BiquadCascade(BiquadType_Bandpass, 12, 140.0 / srate, 200.0 / srate);

	//Initialize the Biquad buffers
	biquad_buffer_L.init_buffer(wavfile.get_size(), sample_rate);
	biquad_buffer_M.init_buffer(wavfile.get_size(), sample_rate);
	biquad_buffer_H.init_buffer(wavfile.get_size(), sample_rate);
	biquad_wavbuffer_L.init_buffer(wavfile.get_size(), sample_rate);
	biquad_wavbuffer_M.init_buffer(wavfile.get_size(), sample_rate);
	biquad_wavbuffer_H.init_buffer(wavfile.get_size(), sample_rate);

	//Copy data from wavfile buffer and process through filters
	for (long i = 0; i < biquad_buffer_L.get_size(); i++)
	{
		biquad_buffer_L[i] = pCascadeLow->process((*wavfile.get_buffer())[i]);
		biquad_buffer_M[i] = pCascadeMid->process((*wavfile.get_buffer())[i]);
		biquad_buffer_H[i] = pCascadeHigh->process((*wavfile.get_buffer())[i]);
	}

	//Shortify - this also scales to short max
	DSP::shortify(biquad_buffer_L, biquad_wavbuffer_L);
	DSP::shortify(biquad_buffer_M, biquad_wavbuffer_M);
	DSP::shortify(biquad_buffer_H, biquad_wavbuffer_H);

	//**********************************
	//* WRITE BIQUAD FILTERED WAV FILE *
	//**********************************
	wavfile_biquad.set_buffer(biquad_wavbuffer_L);
	remove("./wavs/biquad_L.wav");
	std::ofstream biquadwav_L("./wavs/biquad_L.wav", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	wavfile_biquad.write_wav_file(biquadwav_L);
	biquadwav_L.close();

	wavfile_biquad.set_buffer(biquad_wavbuffer_M);
	remove("./wavs/biquad_M.wav");
	std::ofstream biquadwav_M("./wavs/biquad_M.wav", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	wavfile_biquad.write_wav_file(biquadwav_M);
	biquadwav_M.close();

	wavfile_biquad.set_buffer(biquad_wavbuffer_H);
	remove("./wavs/biquad_H.wav");
	std::ofstream biquadwav_H("./wavs/biquad_H.wav", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	wavfile_biquad.write_wav_file(biquadwav_H);
	biquadwav_H.close();

#ifdef PLOT_WAVFILE_BIQUAD
	Gnuplot gp5("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	plot(biquad_wavbuffer, gp5);
#endif

	//*********************************
	//* BIQUAD FILTER BPM CALCULATION *
	//*********************************
	biquad_autocorr_L.init_buffer(AUTOCORR_RES, sample_rate);
	biquad_autocorr_M.init_buffer(AUTOCORR_RES, sample_rate);
	biquad_autocorr_H.init_buffer(AUTOCORR_RES, sample_rate);
	biquad_env.init_buffer(wavfile.get_size(), sample_rate);


	//LOW PASSBAND
	DSP::envelope_filter(biquad_buffer_L, biquad_env, ENV_FILT_REC);
	DSP::build_autocorr_array(biquad_env, biquad_autocorr_L, BPM_MIN_VALUE, BPM_MAX_VALUE);
	double bpm_value_biquad = DSP::extract_bpm_value(biquad_autocorr_L, BPM_MIN_VALUE, BPM_MAX_VALUE);
	std::cout << "LOW PASSBAND: The BPM biquad value is: " << bpm_value_biquad << std::endl;

#ifdef PLOT_BIQUAD_AUTOCORR
	Gnuplot gp6("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	plot(biquad_autocorr_L, gp6);
#endif

	//MID PASSBAND
	DSP::envelope_filter(biquad_buffer_M, biquad_env, ENV_FILT_REC);
	DSP::build_autocorr_array(biquad_env, biquad_autocorr_M, BPM_MIN_VALUE, BPM_MAX_VALUE);
	bpm_value_biquad = DSP::extract_bpm_value(biquad_autocorr_M, BPM_MIN_VALUE, BPM_MAX_VALUE);
	std::cout << "MID PASSBAND: The BPM biquad value is: " << bpm_value_biquad << std::endl;

#ifdef PLOT_BIQUAD_AUTOCORR
	Gnuplot gp7("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	plot(biquad_autocorr_M, gp7);
#endif

	//HIGH PASSBAND
	DSP::envelope_filter(biquad_buffer_H, biquad_env, ENV_FILT_REC);
	DSP::build_autocorr_array(biquad_env, biquad_autocorr_H, BPM_MIN_VALUE, BPM_MAX_VALUE);
	bpm_value_biquad = DSP::extract_bpm_value(biquad_autocorr_H, BPM_MIN_VALUE, BPM_MAX_VALUE);
	std::cout << "HIGH PASSBAND: The BPM biquad value is: " << bpm_value_biquad << std::endl;

#ifdef PLOT_BIQUAD_AUTOCORR
	Gnuplot gp8("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	plot(biquad_autocorr_H, gp8);
#endif
	
	std::vector<buffer<double>*> buffers;
	buffers.push_back(&biquad_autocorr_L);
	buffers.push_back(&biquad_autocorr_M);
	buffers.push_back(&biquad_autocorr_H);
	std::vector<double> widths; widths.push_back(500.0); widths.push_back(500.0); widths.push_back(300.0);
	std::vector<double> thres; thres.push_back(0.3); thres.push_back(0.3); thres.push_back(0.3);
	std::cout << "ADVANCED ALGO: The BPM value is: " << PEAKS::extract_bpm_value(buffers, BPM_MIN_VALUE, BPM_MAX_VALUE, widths, thres) << std::endl;

#ifdef PLOT_BIQUAD_AUTOCORR
	Gnuplot gp9("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	plot(biquad_autocorr_L, gp9);
#endif

#ifdef PLOT_BIQUAD_AUTOCORR
	Gnuplot gp10("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	plot(biquad_autocorr_M, gp10);
#endif

#ifdef PLOT_BIQUAD_AUTOCORR
	Gnuplot gp11("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	plot(biquad_autocorr_H, gp11);
#endif

	delete pCascadeLow;
	delete pCascadeMid;
	delete pCascadeHigh;

	return 0;
}
