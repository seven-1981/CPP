#ifndef _BPM_AUDIO
#define _BPM_AUDIO

//On windows OS, unix structs are not defined, we have to do it manually
//The structs must not be used!
#ifndef _WIN32
	#include <alsa/asoundlib.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include "bpm_globals.hpp"
#include "SplitConsole.hpp"

//Extern split console instance
extern SplitConsole my_console;

//Typedef for wav header info
typedef struct WAVFile
{
	//Header information
	unsigned int file_size;
	unsigned int header_size;
	short        audio_format;
	short	     num_channels;
	unsigned int sample_rate;
	unsigned int byte_rate;
	short        frame_size;
	short        bits_sample;
	unsigned int data_size;

	//Sample buffer
	short*       buffer;
} WAVFile;

//Class for handling audio recording
class BPMAudio
{
public:
	//Constructor
	BPMAudio();
	//Destructor
	~BPMAudio();

	//Public interface of audio class
	//Captures the samples and stores them in the buffer
	eError capture_samples();

	//Flushes the buffer, can be used to get the data
	short* flush_buffer();

	//Get buffer ready flag state
	eError is_ready();

	//Writes a wav file
	eError write_wav_file(std::ofstream& file, short* data);

	//Create wav file struct from recorded buffer
	WAVFile write_wav_file();

	//Read a wav file and return wav struct containing samples
	eError read_wav_file(std::ifstream& file, WAVFile& wav_file);

	//Capture wav file in class member buffer - overloaded method
	//If no struct is provided, data is copied to this->buffer
	eError read_wav_file(std::ifstream& file);

private:

	//On WIN32, the structs are not defined. 
	//BPMAudio class can't use soundcard.
	#ifndef _WIN32
		//Handle for the PCM device
		snd_pcm_t* pcm_handle;

		//Hardware parameters
		snd_pcm_hw_params_t* hw_params;
	#endif

	//Buffer for recorded samples
	short* buffer;

	//The buffer must be protected with a mutex
	std::mutex mtx;

	//Flag indicates, that data is ready to be exported
	bool buffer_ready;

	//Get number of sound cards used
	int get_num_soundcards();
	//Get the names of all sound cards used and return it as vector
	std::vector<std::string> list_names();

	//We store return values / error codes in a member variable
	eError init_state;

	//Private methods, only used internally
	template <typename Word>
	std::ostream& write_word(std::ostream& out_stream, Word value, unsigned int size);

	template <typename Word>
	Word read_word(std::istream& in_stream, bool little_endian, unsigned int size);
};

#endif