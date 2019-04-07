#include <iostream>
#include <vector>
#include <string>
#include <cmath> 
#include <alsa/asoundlib.h>

#include "DSP.hpp"
#include "FCWindow.hpp"
#include "FCWindowManager.hpp"
#include "FCWindowSpectrum.hpp"
#include "FCWindowLabel.hpp"
#include "buffer.hpp"
#include "CFFT.hpp"

//Global data for audio capture
snd_pcm_t *capture_handle;
unsigned int sample_rate = 44100;
//Total frame size
unsigned int num_frames_tot = 1024;
//Frame size of one window
unsigned int num_frames_rec = 256;
//Number of bins considered in calculation
const int fft_bins = 120;
//Number of bins displayed in graph
int fft_bars = 11;

//Buffer for audio data
short* buf = (short*)malloc(num_frames_rec * sizeof(short));

//Different buffers for average fft calculation
buffer<double> b1, b2, b3, b4;
buffer<double> c1, c2, c3, c4;
buffer<double> f1, f2, f3, f4;

//Window size information
FCWindowSpectrumSize_t size_param;
//Window key information
int key1 = 0;
int key2 = 0;

//Keyboard callback
void keyboard_callback(unsigned char key, int x, int y)
{
	//Key has been pressed
	switch (key)
	{	
		case 'a':
			size_param.max_value += 1.0;
			break;
		case 'A':
			size_param.max_value += 10.0;
			break;
		case 'y':
			size_param.max_value -= 1.0;
			break;
		case 'Y':
			size_param.max_value -= 10.0;
			break;
			
		case 's':
			size_param.min_value += 1.0;
			break;
		case 'S':
			size_param.min_value += 10.0;
			break;
		case 'x':
			size_param.min_value -= 1.0;
			break;
		case 'X':
			size_param.min_value -= 10.0;
			break;

		case 'f':
			key1++;
			break;
		case 'F':
			key1--;
			break;
		case 'w':
			key2++;
			break;
		case 'W':
			key2--;
			break;
	}
}

//Capture callback
int capture_callback (snd_pcm_sframes_t nframes)
{
	int err;
	if ((err = snd_pcm_readi(capture_handle, buf, nframes)) < 0) {
		std::cout << "write failed " << snd_strerror(err) << std::endl;
	}
	return err;
}

void set_console_location(int col, int row)
{
	//We use special escape sequences to control the cursor
	//Note: this part is probably only Linux compatible
	std::cout << "\033[" << std::to_string(row) << ";" << std::to_string(col) << "H" << std::flush;
}

//Create log spaced array
template<typename T = double>
class Logspace {
private:
    T curValue, base, step;

public:
    Logspace(T first, T last, int num, T base = 10.0) : curValue(first), base(base){
       step = (last - first)/(num-1);
    }

    T operator()() {
        T retval = pow(base, curValue);
        curValue += step;
        return retval;
    }
};

void process_array(double* in_array, double* out_array)
{
	//Use logarithmic spacing
	double fvalues[fft_bars] = { 50, 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 20000 };
	double fmax = (double)sample_rate / 2;
	double freq_res = sample_rate / num_frames_tot;

	int act_index = 0;
	int count = 0;
	int avg_count = 1;
	for (int i = 0; i < fft_bins; i++)
	{
		double act_freq = i * freq_res;
		if (act_freq >= fvalues[count])
		{
			count++;
			avg_count = 1;
			out_array[count] = in_array[i];
		}
		else
		{
			out_array[count] = out_array[count] * avg_count;
			out_array[count] += in_array[i];
			avg_count++;
			out_array[count] /= avg_count;
		}
	}
}

void init_audio()
{
	//Parameters
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_sframes_t frames_to_deliver;

	int err;
	std::string card_name = "hw:1,0";

	if ((err = snd_pcm_open(&capture_handle, card_name.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		std::cout << "cannot open audio device " << snd_strerror(err) << std::endl; 
		exit(1);
	}
		   
	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		std::cout << "cannot allocate hardware parameter structure " << snd_strerror(err) << std::endl;
		exit(1);
	}
				 
	if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
		std::cout << "cannot initialize hardware parameter structure " << snd_strerror(err) << std::endl;
		exit(1);
	}
	
	if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		std::cout << "cannot set access type " << snd_strerror(err) << std::endl;
		exit(1);
	}
	
	if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		std::cout << "cannot set sample format " << snd_strerror(err) << std::endl;
		exit(1);
	}
	
	if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &sample_rate, 0)) < 0) {
		std::cout << "cannot set sample rate " << snd_strerror(err) << std::endl;
		exit(1);
	}
	
	if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, 1)) < 0) {
		std::cout << "cannot set channel count " << snd_strerror(err) << std::endl;
		exit(1);
	}
	
	if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
		std::cout << "cannot set parameters " << snd_strerror(err) << std::endl;
		exit(1);
	}
	
	snd_pcm_hw_params_free(hw_params);
	
	/* tell ALSA to wake us up whenever num_frames or more frames
	   of capture data can be delivered. Also, tell
	   ALSA that we'll start the device ourselves.
	*/
	
	if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
		std::cout << "cannot allocate software parameters structure " << snd_strerror(err) << std::endl;
		exit(1);
	}

	if ((err = snd_pcm_sw_params_current(capture_handle, sw_params)) < 0) {
		std::cout << "cannot initialize software parameters structure " << snd_strerror(err) << std::endl;
		exit(1);
	}

	if ((err = snd_pcm_sw_params_set_avail_min(capture_handle, sw_params, num_frames_rec)) < 0) {
		std::cout << "cannot set minimum available count " << snd_strerror(err) << std::endl;
		exit(1);
	}

	if ((err = snd_pcm_sw_params_set_start_threshold(capture_handle, sw_params, 0U)) < 0) {
		std::cout << "cannot set start mode " << snd_strerror(err) << std::endl;
		exit(1);
	}

	if ((err = snd_pcm_sw_params(capture_handle, sw_params)) < 0) {
		std::cout << "cannot set software parameters " << snd_strerror(err) << std::endl;
		exit(1);
	}
	
	/* the interface will interrupt the kernel every 4096 frames, and ALSA
	   will wake up this program very soon after that.
	*/
	
	if ((err = snd_pcm_prepare(capture_handle)) < 0) {
		std::cout << "cannot prepare audio interface for use " << snd_strerror(err) << std::endl;
		exit(1);
	}

	if ((err = snd_pcm_start(capture_handle)) < 0) {
		std::cout << "cannot start capturing " << snd_strerror(err) << std::endl;
		exit(1);
	}
}

void average_window(buffer<double>& buf, int option, double par = 0.5)
{
	if (option == 1)
	{
		DSP::apply_window(buf, DSP::hanning);
	}
	if (option == 2)
	{	
		DSP::apply_window(buf, DSP::hamming, par);
	}
	if (option == 3)
	{	
		DSP::apply_window(buf, DSP::blackman, par);
	}
	if (option == 4)
	{	
		DSP::apply_window(buf, DSP::blackman_harris);
	}
	if (option == 5)
	{	
		DSP::apply_window(buf, DSP::flat_top);
	}
	if (option == 6)
	{	
		DSP::apply_window(buf, DSP::tukey, par);
	}
}

void average_fft(buffer<double>& in, buffer<double>& out, int option)
{
	long s = in.get_size();
	long N = s / 4;

	for (long i = 0; i < s; i++)
	{
		if (i < N)
			b1[i] = in[i];
		else if (i < 2*N)
			b2[i - N] = in[i];
		else if (i < 3*N)
			b3[i - 2*N] = in[i];
		else 
			b4[i - 3*N] = in[i];
	}

	static int option_old = 0;
	if (option_old != option)
	{
		std::cout << "option = " << option << std::endl;
		option_old = option;
	}

	average_window(b1, option);
	average_window(b2, option);
	average_window(b3, option);
	average_window(b4, option);

	DSP::zero_padding(b1, c1);
	DSP::zero_padding(b2, c2);
	DSP::zero_padding(b3, c3);
	DSP::zero_padding(b4, c4);

	DSP::perform_fft(c1, f1, +1);
	DSP::perform_fft(c2, f2, +1);
	DSP::perform_fft(c3, f3, +1);
	DSP::perform_fft(c4, f4, +1);

	for (long i = 0; i < s * 2; i += 2)
	{
		double val1 = 4.0 / num_frames_tot * (f1[i]*f1[i] + f1[i+1]*f1[i+1]);
		double val2 = 4.0 / num_frames_tot * (f2[i]*f2[i] + f2[i+1]*f2[i+1]);
		double val3 = 4.0 / num_frames_tot * (f3[i]*f3[i] + f3[i+1]*f3[i+1]);
		double val4 = 4.0 / num_frames_tot * (f4[i]*f4[i] + f4[i+1]*f4[i+1]);
		
		out[i / 2] = (val1 + val2 + val3 + val4) / 4.0; 
	}
}

void env_array(double* buffer_old, double* buffer_new, int size, double rec)
{
	for (int i = 0; i < size; i++)
	{
		if (buffer_new[i] < buffer_old[i])
		{
			double diff = buffer_old[i] - buffer_new[i];
			buffer_new[i] = buffer_old[i] - rec * diff;
		}
	}
}
	      
int main (int argc, char **argv)
{
	//Clear screen
	std::cout << "\033[2J\033[1;1H";

	//Initialize buffers
	buffer<double> data_rec; //256
	buffer<double> data_nor; //256
	buffer<double> data_tot; //1024
	buffer<double> data_avg; //1024
	data_rec.init_buffer(num_frames_rec, sample_rate);
	data_nor.init_buffer(num_frames_rec, sample_rate);
	data_tot.init_buffer(num_frames_tot, sample_rate);
	data_avg.init_buffer(num_frames_tot, sample_rate);

	buffer<double> fft; //2048
	fft.init_buffer(num_frames_tot * 2, sample_rate);
	buffer<double> fft2; //1024
	fft2.init_buffer(num_frames_tot, sample_rate);

	static double sarray[fft_bins] = { };

	size_param.max_value = 0.0;
	size_param.min_value = -100.0;
	
	//Init window manager
	FCWindowManager::init(argc, argv);
	
	//Parameters for spectrum window
	FCWindowParam_t win_param;
	win_param.x = 500; win_param.y = 250;
	win_param.title = "AUDIO SPECTRUM";
	win_param.fullscreen = false;
	win_param.size = fft_bins;
	//Create spectrum window using manager
	FCWindow* window = FCWindowManager::create(TypeWindowSpectrum, win_param);
	
	//Parameters for second window
	win_param.title = "Second window";
	//Create second window using manager
	FCWindow* window2 = FCWindowManager::create(TypeWindowLabel, win_param);
	
	//Use derived class pointer for param setting
	FCWindowSpectrum* window_spectrum = dynamic_cast<FCWindowSpectrum*>(window);
	window_spectrum->set_param(size_param);
	
	//Start event loop
	FCWindowManager::start();
	//Set additional callback for keys
	window->set_keyboard_callback(keyboard_callback);
	

	init_audio();

	int err;
	snd_pcm_sframes_t frames_to_deliver;

	b1.init_buffer(num_frames_rec, sample_rate);
	b2.init_buffer(num_frames_rec, sample_rate);
	b3.init_buffer(num_frames_rec, sample_rate);
	b4.init_buffer(num_frames_rec, sample_rate);
	c1.init_buffer(num_frames_tot, sample_rate);
	c2.init_buffer(num_frames_tot, sample_rate);
	c3.init_buffer(num_frames_tot, sample_rate);
	c4.init_buffer(num_frames_tot, sample_rate);
	
	f1.init_buffer(num_frames_tot * 2, sample_rate);
	f2.init_buffer(num_frames_tot * 2, sample_rate);
	f3.init_buffer(num_frames_tot * 2, sample_rate);
	f4.init_buffer(num_frames_tot * 2, sample_rate);

	complex* pSignal = new complex[num_frames_tot];
	
	while(1) 
	{
	
		/* wait till the interface is ready for data, or 1 second
		   has elapsed.
		*/
	
		if ((err = snd_pcm_wait(capture_handle, 1000)) < 0) {
		        std::cout << "poll failed " << snd_strerror(err) << std::endl;
		        break;
		}	           
	
		/* find out how much space is available for playback data */
	
		if ((frames_to_deliver = snd_pcm_avail_update(capture_handle)) < 0) {
			if (frames_to_deliver == -EPIPE) {
				std::cout << "an xrun occured" << std::endl;
				break;
			} else {
				std::cout << "unknown ALSA avail update return value" << std::endl;
				break;
			}
		}

		//std::cout << "Frames to deliver: " << frames_to_deliver << std::endl;
	
		frames_to_deliver = frames_to_deliver > num_frames_rec ? num_frames_rec : frames_to_deliver;
	
		/* deliver the data */
	
		if (capture_callback(frames_to_deliver) != frames_to_deliver) {
		        std::cout << "capture callback failed" << std::endl;
			break;
		}
		
		//Start spectrum analysis
		double maxval = -std::numeric_limits<short>::min();
		for (unsigned int i = 0; i < num_frames_rec; i++)
			data_rec[i] = (double)buf[i] / maxval;

		DSP::cut_dc_offset(data_rec, data_nor);

		//Number of recorded frames -> z
		//For the first recorded frames, we have to fill the
		//buffer with frames, before it can be moved (slide)
		static unsigned int z = 0;

		if (z < 4)
		{
			for (long i = 0; i < num_frames_rec; i++)
				data_tot[z * num_frames_rec + i] = data_nor[i];
		}
		else
		{
			DSP::sliding_window(data_nor, data_tot);

			if (key1 == -1)
			{
				for (long i = 0; i < num_frames_tot; i++)
				{
					pSignal[i].real(data_tot[i]);
					pSignal[i].imag(0.0);
				}
			
				CFFT::Forward(pSignal, num_frames_tot);
				for (long i = 0; i < num_frames_tot; i++)
				{
					fft[2 * i] = pSignal[i].real();
					fft[2 * i + 1] = pSignal[i].imag();
				}
			}

			if (key1 == 0)
			{
				//DSP::apply_window(data_tot, DSP::hanning);
				average_window(data_tot, key2);
				DSP::perform_fft(data_tot, fft, +1);
			}
			if (key1 == 1)
			{
				average_fft(data_tot, fft2, key2);
			}

			double freq_res = sample_rate / num_frames_tot;
	
			std::vector<std::pair<double, double>> fft_values;
			for (long i = 0; i < fft.get_size() / 2; i+=2)
			{
				double val = 1.0 / (double)num_frames_tot * (fft[i] * fft[i] + fft[i + 1] * fft[i + 1]);
				fft_values.push_back(std::pair<double, double>((double)i / 2 * freq_res, val));
			}

			//Create data for spectrum window
			FCWindowSpectrumData_t win_data;
			double array[fft_bins] = { };

			if (key1 == 0 || key1 == -1)
			{
				for (int i = 0; i < fft_bins; i++)
					array[i] = 10 * log10(fft_values.at(i).second);
			}
			if (key1 == 1)
			{
				for (int i = 0; i < fft_bins; i++)
					array[i] = 10 * log10(fft2[i]);
			}				

			//double arr[fft_bars] = { };
			//process_array(array, arr);	
	
			//Envelope for spectrum values (looks smoother)
			env_array(sarray, array, fft_bins, 0.05);
			for (int i = 0; i < fft_bins; i++)
			{
				sarray[i] = array[i];
				win_data.values.push_back(array[i]);
			}
			//Update spectrum data
			window->update(&win_data);
			
			//Create data for second window
			FCWindowLabelData_t label_data;
			FCWindowLabelDataItem_t label_items;
			label_items.text = "Key 1: " + std::to_string(key1);
			label_data.items.push_back(label_items);
			label_items.y = 150;
			label_items.text = "Key 2: " + std::to_string(key2);
			label_data.items.push_back(label_items);
			//Update label data
			window2->update(&label_data);
		}

		z++;
	
		//Check if quit issued
		if (window->get_quit() == true)
			break;
	}
	
	FCWindowManager::stop();
	std::this_thread::sleep_for(std::chrono::seconds(1));

	delete window;
	delete window2;

	delete[] pSignal;
	
	snd_pcm_close(capture_handle);
	return 0;
}
