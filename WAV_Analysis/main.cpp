#include <iostream>
#include <string>
#include <vector>

#include "WAVFile.h"
#include "DSP.h"
//Uses boost library. In Project options set include path to c:\program files\boost
//and library path to C:\Program Files\Boost\stage\lib
//boost lib files must be created first with b2.exe and bjam.exe
//#include "gnuplot-iostream.h"

//Defines for audio parameters
#define PCM_BUF_SIZE       88200
#define PCM_SAMPLE_RATE    44100

//Defines for bpm detection
#define BPM_MAX_VALUE 200.0
#define BPM_MIN_VALUE 80.0
#define AUTOCORR_RES 3000
#define MOV_AVG_SIZE 50

//Defines for Plots
//#define PLOT_WAVFILE
//#define PLOT_FFT
//#define PLOT_INV_FFT
//#define PLOT_AUTOCORR

double extract_bpm_value(buffer<double>& autocorr_array, buffer<short>& inbuffer, buffer<double>& mov_avg)
{
	//Declare return value
	double max_autocorr_value = 0.0;
	long max_array_value = 0;

	//Calculate lag values -> beats/minute to seconds/beat
	double min_lag = (double)60 / (double)BPM_MAX_VALUE;
	double max_lag = (double)60 / (double)BPM_MIN_VALUE;
	double lag;

	//Moving average filtered values
	double ma_value = 0.0;

	for (long i = 0; i < AUTOCORR_RES; i++)
	{
		lag = min_lag + (max_lag - min_lag) / (double)AUTOCORR_RES * (double)i;
		autocorr_array.values[i] = get_autocorr(lag, inbuffer);

		//Calculate moving average. Start if correct number of calculations
		//have been made.
		if (i >= MOV_AVG_SIZE)
		{
			for (int j = 0; j < MOV_AVG_SIZE; j++)
			{
				ma_value = ma_value + autocorr_array.values[i - MOV_AVG_SIZE + j] / MOV_AVG_SIZE;
			}
			mov_avg.values[i - MOV_AVG_SIZE] = ma_value;
			ma_value = 0.0;

			//Save max value
			if (mov_avg.values[i - MOV_AVG_SIZE] > max_autocorr_value)
			{
				max_autocorr_value = mov_avg.values[i - MOV_AVG_SIZE];
				max_array_value = i - MOV_AVG_SIZE;
			}
		}
	}

	//The lag of the moving average filter must be compensated.
	//The max index value is shifted.
	max_array_value *= AUTOCORR_RES / (AUTOCORR_RES - MOV_AVG_SIZE);

	//Return the calculated bpm value from the max index
	double bpm_lag = min_lag + (max_lag - min_lag) / AUTOCORR_RES * max_array_value;

	return 60.0 / bpm_lag;
}

int main()
{
	//WAV FILE READ
	WAVFile wavfile;
	std::ifstream read_wav("./wavs/sinewave1000Hz.wav", std::ios_base::in | std::ios_base::binary);
	wavfile.read_wav_file(read_wav);
	read_wav.close();
	std::cout << "The last byte is: " << wavfile.get_buffer()->values[PCM_BUF_SIZE - 1] << std::endl;
	long sample_rate = wavfile.get_header_info()->sample_rate;

	//CREATE WINDOW BUFFER AND APPLY WINDOW
	buffer<double> window_buffer(wavfile.get_size(), sample_rate);
	apply_window(*wavfile.get_buffer(), window_buffer, tukey, 0.5);

	//PLOT WINDOWED WAVFILE
#ifdef PLOT_WAVFILE
	Gnuplot gp1("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp1 << "plot '-' with lines\n";
	std::vector<double> data1;
	long size1 = window_buffer.get_size();
	for (long i = 0; i < size1; i++)
		data1.push_back(window_buffer.values[i]);
	gp1.send1d(data1);
#else
	std::ofstream windowdata("windata.txt", std::ios_base::out | std::ios_base::trunc);
	for (long i = 0; i < window_buffer.get_size(); i++)
		windowdata << window_buffer.values[i] << "\n";
	windowdata.close();
#endif

	//APPLY FFT TO WAV AND PLOT
	long pow2samples = pow2_size(window_buffer, true); // true for floor, false for ceil
	std::cout << "Pow 2 size: " << pow2samples << "." << std::endl;
	buffer<short> time_domain(pow2samples, sample_rate);
	buffer<double> freq_domain(pow2samples * 2, sample_rate);
	create_fft_buffer(*wavfile.get_buffer(), time_domain);
	//create_fft_buffer(window_buffer, time_domain); //<- uses windowed data
	perform_fft(time_domain, freq_domain, +1);

	double freqres = (double)freq_domain.get_sample_rate() / pow2samples;
	std::cout << "The freq. resolution: " << freqres << std::endl;
#ifdef PLOT_FFT
	Gnuplot gp2("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp2 << "plot '-' with lines\n";
	std::vector<std::pair<double, double>> pair;
	for (long i = 0; i < 5000; i++)
		pair.push_back(std::pair<double, double>((double)i*freqres, 1.0 / (pow2samples * 2) * sqrt(freq_domain.values[i] * freq_domain.values[i] + freq_domain.values[i + 1] * freq_domain.values[i + 1])));
	gp2.send1d(pair);
#else
	std::ofstream fftdata("fftdata.txt", std::ios_base::out | std::ios_base::trunc);
	for (long i = 0; i < pow2samples; i+=2)
		fftdata << (double)i / 2 * freqres << ", " << 1.0 / (pow2samples) * sqrt(freq_domain.values[i] * freq_domain.values[i] + freq_domain.values[i + 1] * freq_domain.values[i + 1]) << "\n";
	fftdata.close();
#endif

	//REMOVE BASS FREQ FROM FFT
	buffer<double> freq_filt(pow2samples, sample_rate);
	cut_freq(freq_domain, freq_filt, 20.0, 250.0);

	//INVERSE FFT AND PLOT
	buffer<double> time_filt(pow2samples * 2, sample_rate);
	perform_fft(freq_filt, time_filt, -1);

#ifdef PLOT_INV_FFT
	Gnuplot gp3("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp3 << "plot '-' with lines\n";
	std::vector<double> data3;
	for (long i = 0; i < pow2samples; i++)
		data3.push_back(1.0 / (double)sample_rate * time_filt.values[i]);
	gp3.send1d(data3);
#endif

	//EXTRACT WAV DATA
	buffer<short> endbuffer(pow2samples, sample_rate);
	for (long i = 0; i < pow2samples; i++)
		endbuffer.values[i] = (short)(1.0 / (double)sample_rate * time_filt.values[i]);

	//MAXIMIZE VOLUME OF DATA
	maximize_volume(endbuffer);

	//WRITE FILTERED WAV FILE
	WAVFile filt_wav;
	filt_wav.set_buffer(endbuffer);
	std::ofstream filtwav("./wavs/filtwav.wav", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	filt_wav.write_wav_file(filtwav);
	filtwav.close();

	//PERFORM AUTOCORRELATION AND EXTRACT BPM
	buffer<double> autocorr_array(AUTOCORR_RES, PCM_SAMPLE_RATE);
	buffer<double> mov_avg(AUTOCORR_RES - MOV_AVG_SIZE, PCM_SAMPLE_RATE);
	double value = extract_bpm_value(autocorr_array, endbuffer, mov_avg);

	//ENVELOPE FILTER ON AUTOCORR
	buffer<double> env(AUTOCORR_RES, PCM_SAMPLE_RATE);
	envelope_filter(autocorr_array, env, 0.005);

	std::cout << "The BPM value is: " << value << std::endl;

	//AUTOCORR PLOT
#ifdef PLOT_AUTOCORR
	Gnuplot gp4("\"C:\\Programme\\gnuplot\\bin\\gnuplot.exe -persist\"");
	gp4 << "plot '-' with lines\n";
	std::vector<double> data4;
	for (long i = 0; i < AUTOCORR_RES; i++)
		data4.push_back(env.values[i]);
	gp4.send1d(data4);
#endif

return 0;
}
