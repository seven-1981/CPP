#include <string>
#include <vector>
#include <fstream>
#include "bpm_audio.hpp"
#include "bpm_globals.hpp"
#include "bpm_param.hpp"
#include "SplitConsole.hpp"

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

BPMAudio::BPMAudio()
{
	//Constructor for audio handler instance
	if (param_list.get<bool>("debug audio") == true)
		my_console.WriteToSplitConsole("BPM Audio Class: Instantiating audio handler class.", param_list.get<int>("split audio"));

	//Define return value
	this->init_state = eSuccess;

	//Set ready flag
	this->buffer_ready = false;

	//The soundcard initialisation procedure is not executed on WIN32
#ifndef _WIN32

	//Get soundcard info
	std::vector<std::string> dev_list = list_names();

	//Prepare device name string
	std::string s = "hw:";
	bool name_assigned = false;

	//Here we check for the search string "USB" and select that soundcard to generate device name
	//The resulting device name should be 'hw[X],[Y]', where X is card# and Y is device#
	//Soundcard# could probably change if soundcard is plugged in other USB port
	//Device# shouldn't change however
	if (dev_list.empty() == true)
	{
		//No sound cards found
		//The function list_names() has already set the init_state variable
	}
	else
	{
		//Check for USB sound card
		for (int i = 0; i < dev_list.size(); i++)
		{
			if (dev_list[i].find(std::string(PCM_DEVICE)) != std::string::npos)
			{
				s = s + std::to_string(i);
				name_assigned = true;
				break;
			}
		}

		//Check if USB card was found
		if (name_assigned == false)
		{
			//No USB sound card found
			this->init_state = eAudio_NoSoundCardsFound;
		}
		else
		{
			//Finish the string
			s = s + "," + std::to_string(PCM_SUBDEVICE);

			//Open audio device
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Opening audio device " + s, param_list.get<int>("split audio"));

			int err;
			err = snd_pcm_open(&pcm_handle, s.c_str(), PCM_CAPTURE_MODE, 0);

			if (err < 0)
			{
				if (param_list.get<bool>("debug audio") == true)
					my_console.WriteToSplitConsole("BPM Audio Class: Error opening audio device.", param_list.get<int>("split errors"));

				this->init_state = eAudio_ErrorOpeningAudioDevice;
			}
			else
			{
				if (param_list.get<bool>("debug audio") == true)
					my_console.WriteToSplitConsole("BPM Audio Class: Success opening audio device.", param_list.get<int>("split audio"));
			}
		}
	}

	//At this point, we check if the sound card device has been successfully opened
	//The code below is only executed on success
	int err;

	//Allocate hardware parameter structure
	if (this->init_state == eSuccess)
	{
		//Continue setting up audio parameters
		//Allocate hardware parameter structure
		err = snd_pcm_hw_params_malloc(&hw_params);
		if (err < 0)
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Error allocating audio hw parameter structure.", param_list.get<int>("split errors"));

			this->init_state = eAudio_ErrorAllocatingHwStruc;
		}
		else
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Success allocating hw parameter structure.", param_list.get<int>("split audio"));
		}
	}

	//Initialize parameter structure
	if (this->init_state == eSuccess)
	{
		//Init parameter structure
		err = snd_pcm_hw_params_any(pcm_handle, hw_params);
		if (err < 0)
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Error initializing audio hw parameter structure.", param_list.get<int>("split errors"));

			this->init_state = eAudio_ErrorInitHwStruc;
		}
		else
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Success initializing hw parameter structure.", param_list.get<int>("split audio"));
		}
	}

	//Set access
	if (this->init_state == eSuccess)
	{
		//Set access mode
		err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, PCM_ACCESS_MODE);
		if (err < 0)
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Error setting access mode.", param_list.get<int>("split errors"));

			this->init_state = eAudio_ErrorSettingAccessMode;
		}
		else
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Success setting access mode.", param_list.get<int>("split audio"));
		}
	}

	//Set audio format
	if (this->init_state == eSuccess)
	{
		//Set the audio format to capture
		err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, PCM_AUDIO_FORMAT);
		if (err < 0)
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Error setting audio format.", param_list.get<int>("split errors"));

			this->init_state = eAudio_ErrorSettingFormat;
		}
		else
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Success setting audio format.", param_list.get<int>("split audio"));
		}
	}

	//Set sampling rate
	if (this->init_state == eSuccess)
	{
		//Set sampling rate
		unsigned int rate = PCM_SAMPLE_RATE;
		err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0);
		if (err < 0)
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Error setting sample rate.", param_list.get<int>("split errors"));

			this->init_state = eAudio_ErrorSettingSampleRate;
		}
		else
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Success setting sample rate.", param_list.get<int>("split audio"));
		}
	}

	//Set number of channels
	if (this->init_state == eSuccess)
	{
		//Set channels
		err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, PCM_CHANNELS);
		if (err < 0)
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Error setting channels.", param_list.get<int>("split errors"));

			this->init_state = eAudio_ErrorSettingChannels;
		}
		else
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Success setting channels.", param_list.get<int>("split audio"));
		}
	}

	//Set hardware parameters
	if (this->init_state == eSuccess)
	{
		//Activate the hardware parameters
		err = snd_pcm_hw_params(pcm_handle, hw_params);
		if (err < 0)
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Error setting hardware parameters.", param_list.get<int>("split errors"));

			this->init_state = eAudio_ErrorSettingHwParameters;
		}
		else
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Success setting hardware parameters.", param_list.get<int>("split audio"));

			//Free memory 
			snd_pcm_hw_params_free(hw_params);
		}
	}

	//Prepare audio interface
	if (this->init_state == eSuccess)
	{
		//Activate the audio interface
		err = snd_pcm_prepare(pcm_handle);
		if (err < 0)
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Error activating audio interface.", param_list.get<int>("split errors"));

			this->init_state = eAudio_ErrorActivateAudioInterf;
		}
		else
		{
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Success activating audio interface.", param_list.get<int>("split audio"));
		}
	}

	//At this point, the audio interface is fully configured and ready to capture audio data
	//We only have to initialize the buffer
	//The malloc command provides a pointer to a mem pool, size in bytes specified by argument
	//Since we have 16bit audio, we use a short
	this->buffer = (short*)malloc(PCM_BUF_SIZE * snd_pcm_format_width(PCM_AUDIO_FORMAT) / 8 * PCM_CHANNELS);

	//The constructor now has finished setting up the audio handler class.
	//Now we can use public interface methods for letting the sound card record a bunch
	//of samples and store it into the buffer.
	//Of course, the caller has to evaluate the init_state attribute before doing something.

#else
	//At this point, we have to initialize the audio buffer for WIN32 use.
	this->buffer = (short*)malloc(PCM_BUF_SIZE * sizeof(short) * PCM_CHANNELS);
#endif
}

BPMAudio::~BPMAudio()
{
	//Destructor
	if (param_list.get<bool>("debug audio") == true)
		my_console.WriteToSplitConsole("BPM Audio Class: Closing audio device.", param_list.get<int>("split main"));

#ifndef _WIN32

	snd_pcm_close(this->pcm_handle);
	free(this->buffer);

#endif	
}

eError BPMAudio::get_state()
{
	//Return the init state
	return this->init_state;
}

int BPMAudio::get_num_soundcards()
{
	//No cards found yet
	int totalCards = 0;

	//Function not implemented on WIN32
#ifndef _WIN32

	//First card is 0, so we have to start with -1
	int cardNum = -1;
	int err;

	while (1)
	{
		//Get next sound card's card number
		err = snd_card_next(&cardNum);
		if (err < 0)
			break;

		if (cardNum < 0)
			break;

		++totalCards;
	}

	//ALSA allocates some memory to load its config file when we call
	//snd_card_next. Now that we're done getting the info, tell ALSA
	//to unload the info and release the memory.
	snd_config_update_free_global();

#endif

	return totalCards;
}

std::vector<std::string> BPMAudio::list_names()
{
	//Get the number of soundcards on the raspberry
	int n = get_num_soundcards();

	//If no sound cards found, issue error
	if (n < 1)
		this->init_state = eAudio_NoSoundCardsFound;

	//Declare return value
	std::vector<std::string> retval;
	int err;

	//Function not implemented on WIN32
#ifndef _WIN32

	for (int i = 0; i < n; i++)
	{
		//Handle for sound card interface
		snd_ctl_t *cardHandle;

		//Open this card's control interface.
		//We specify only the card number - not any device nor sub-device too
		std::string s = "hw:" + std::to_string(i);
		err = snd_ctl_open(&cardHandle, s.c_str(), 0);
		if (err < 0)
		{
			this->init_state = eAudio_CantOpenSoundcard;
			continue;
		}

		//Used to hold card information
		snd_ctl_card_info_t *cardInfo;

		//We need to get a snd_ctl_card_info_t. Just allocate it on the stack.
		snd_ctl_card_info_alloca(&cardInfo);

		//Tell ALSA to fill in our snd_ctl_card_info_t with info about this card
		err = snd_ctl_card_info(cardHandle, cardInfo);
		if (err < 0)
			this->init_state = eAudio_CantGetCardInfo;
		else
		{
			std::string s = snd_ctl_card_info_get_name(cardInfo);
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Card " + std::to_string(i) + " = " + s, param_list.get<int>("split audio"));

			retval.push_back(s);
		}

		// Close the card's control interface after we're done with it
		snd_ctl_close(cardHandle);
	}

	//ALSA allocates some mem to load its config file when we call some of the
	//above functions. Now that we're done getting the info, let's tell ALSA
	//to unload the info and free up that mem
	snd_config_update_free_global();

#endif

	//Return name vector
	return retval;
}

eError BPMAudio::capture_samples()
{
	//This method captures samples from the audio device
	//and stores them into the buffer

	//Declare return value
	eError retval = eSuccess;

	//Function not implemented on WIN32
#ifndef _WIN32

	//Check if the audio interface has been properly set up
	if (this->init_state != eSuccess)
		return init_state;

	//First, lock the mutex
	this->mtx.lock();

	//Capture audio data
	int err = snd_pcm_readi(pcm_handle, buffer, PCM_BUF_SIZE);

	//Check if everything went fine
	if (err < 0)
	{
		retval = eAudio_ErrorCapturingAudio;
		if (param_list.get<bool>("debug audio") == true)
			my_console.WriteToSplitConsole("BPM Audio Class: Error during capture of audio data: " + std::to_string(err), param_list.get<int>("split errors"));
		int err_drop = snd_pcm_drop(pcm_handle);
		if (param_list.get<bool>("debug audio") == true)
			my_console.WriteToSplitConsole("BPM Audio Class: Dropping samples: " + std::to_string(err_drop), param_list.get<int>("split errors"));
		int err_rec = snd_pcm_recover(pcm_handle, err, 0);
		if (param_list.get<bool>("debug audio") == true)
			my_console.WriteToSplitConsole("BPM Audio Class: Recovering: " + std::to_string(err_rec), param_list.get<int>("split errors"));

	}
	else
	{
		//Set the ready flag
		this->buffer_ready = true;

		static int counter = 0;
		if (param_list.get<bool>("debug audio") == true)
			my_console.WriteToSplitConsole("BPM Audio Class: Successfully captured audio: " + std::to_string(err) + ", #" + std::to_string(counter++), param_list.get<int>("split audio"));
	}

	//Unlock the mutex
	this->mtx.unlock();

#endif

	return retval;
}

eError BPMAudio::stop_recording()
{
	//Declare return value
	eError retval = eSuccess;

#ifndef _WIN32

	//Stop the PCM recording
	int err = snd_pcm_drop(pcm_handle);

	if (param_list.get<bool>("debug audio") == true)
		my_console.WriteToSplitConsole("BPM Audio Class: Stopping.", param_list.get<int>("split audio"));

	//Check if everything went fine
	if (err < 0)
	{
		retval = eAudio_ErrorCapturingAudio;
		if (param_list.get<bool>("debug audio") == true)
			my_console.WriteToSplitConsole("BPM Audio Class: Error during stopping: " + std::to_string(err), param_list.get<int>("split errors"));
	}
	else
	{
		//Prepare again for next use
		int err_prep = snd_pcm_prepare(pcm_handle);
		//Check if everything went fine
		if (err_prep < 0)
		{
			retval = eAudio_ErrorCapturingAudio;
			if (param_list.get<bool>("debug audio") == true)
				my_console.WriteToSplitConsole("BPM Audio Class: Error during prepare: " + std::to_string(err_prep), param_list.get<int>("split errors"));
		}

	}

#endif

	return retval;
}

short* BPMAudio::flush_buffer()
{
	//Declare return value
	short* retval;

	//Lock the mutex
	this->mtx.lock();

	retval = this->buffer;

	//Reset the ready flag
	this->buffer_ready = false;

	//Unlock the mutex
	this->mtx.unlock();

	return retval;
}

eError BPMAudio::is_ready()
{
	//Return the buffer ready status
	if (this->buffer_ready == true)
		return eSuccess;
	else
		return eAudio_ErrorBufferNotReady;
}

template <typename Word>
std::ostream& BPMAudio::write_word(std::ostream& out_stream, Word value, unsigned int size = sizeof(Word))
{
	//Function to write "word" into output stream ("binary write"), only works for little endian!
	//For big endian, the for loop must be reversed!
	//"Word" is a template, meaning it could stand for signed int, unsigned int, char etc.
	//We take the data type size as starting value and counting down the bytes
	//Extracts one byte from the value and puts it into the output stream
	//Then a bit shift must be done to get the next byte
	for (; size != 0; --size, value >>= 8)
		out_stream.put(static_cast<char>(value & 0xFF));
	return out_stream;
}

template <typename Word>
Word BPMAudio::read_word(std::istream& in_stream, bool little_endian, unsigned int size = sizeof(Word))
{
	//Function for reading a "word" from the input stream ("binary read"), works for both endianness.
	//Check out write_word method for further information
	char c;
	Word word = 0;
	unsigned int width = size;

	for (; size != 0; --size)
	{
		in_stream.get(c);
		if (little_endian == false)
		{
			word |= c;
			if (size > 1)
				word <<= 8;
		}
		else
		{
			Word temp_word = (Word)(c & 0xFF);
			temp_word = temp_word << (8 * (width - size));
			word |= temp_word;
		}
	}

	return word;
}

eError BPMAudio::write_wav_file(std::ofstream& file, short* data)
{
	//Declare return value
	eError retval = eSuccess;

	//Check if the audio interface has been properly set up
	if (this->init_state != eSuccess)
		return init_state;

	//Write .wav file header
	//Note: file must be created in binary mode
	file << "RIFF----WAVEfmt ";	//Chunk size to be filled in later
	write_word(file, PCM_WAV_FMT_LENGTH, 4);	//No extension data
	write_word(file, PCM_WAV_FORMAT_TAG, 2);	//PCM - integer samples
	write_word(file, PCM_CHANNELS, 2);	//One channel - mono file
	write_word(file, PCM_WAV_SAMPLE_RATE, 4);	//Samples per second / Hz
	write_word(file, PCM_WAV_BYTES_SEC, 4);	//Sample rate * bits per sample * channels / 8 - bytes/s
	write_word(file, PCM_WAV_FRAME_SIZE, 2);	//Data block size in bytes
	write_word(file, PCM_WAV_BITS, 2);	//Number of bits per sample (multiple of 8)	

										//Write the data chunk header
	size_t data_chunk_pos = file.tellp();
	file << "data----";		//Chunk size to be filled in later

							//Write the audio samples - following code block works only for mono probably
	for (long i = 0; i < PCM_BUF_SIZE; i++)
		write_word(file, (short)(data[i]), PCM_WAV_FRAME_SIZE);

	//Final file size to fix the chunk size above
	size_t file_length = file.tellp();

	//Fix data chunk header to contain data size
	file.seekp(data_chunk_pos + 4);
	write_word(file, file_length - data_chunk_pos - 8);

	//Fix the file header to contain the proper RIFF chunk size, which is file size - 8 bytes
	file.seekp(0 + 4);
	write_word(file, file_length - 8, 4);

	return retval;
}

WAVFile BPMAudio::write_wav_file()
{
	//Create a wav file
	WAVFile retval;
	retval.data_size = 0;
	retval.buffer = nullptr;


	//Now we just copy the received samples into the wav file
	//If there's no data, put this info into the struct
	if (this->buffer_ready == false)
	{
		//We do nothing
	}
	else
	{
		retval.data_size = PCM_BUF_SIZE;
		for (long i = 0; i < PCM_BUF_SIZE; ++i)
			retval.buffer[i] = this->buffer[i];
	}

	return retval;
}

eError BPMAudio::read_wav_file(std::ifstream& file, WAVFile& wav_file)
{
	//Declare return value
	eError retval = eSuccess;

	//Check if the audio interface has been properly set up
	if (file.is_open() == false)
		return eAudio_ErrorInputStreamNotOpen;

	//Read wav file

	//Here we would have to decode the wav file header to get the
	//configuration and the amount of data. But since we know the 
	//sample rate and the buffer size, we don't have to do this. Thus, it
	//only works for wav files made by this program!

	//Ignore the first four bytes
	int ignore = read_word<int>(file, false);

	//Now we read the total file size
	unsigned int file_size = read_word<unsigned int>(file, true);
	if (param_list.get<bool>("debug wavfile") == true)
		my_console.WriteToSplitConsole("Reading wav file. Size = " + std::to_string(file_size) + " bytes.", param_list.get<int>("split audio"));

	//Ignore next 8 bytes - WAVE, fmt_
	ignore = read_word<int>(file, false);
	ignore = read_word<int>(file, false);

	//Header size
	unsigned int header_size = read_word<unsigned int>(file, true);
	if (param_list.get<bool>("debug wavfile") == true)
		my_console.WriteToSplitConsole("BPM Audio Class: Header size = " + std::to_string(header_size) + " bytes.", param_list.get<int>("split audio"));

	//Get audio format and number of channels
	short audio_format = read_word<short>(file, true);
	short num_channels = read_word<short>(file, true);
	if (param_list.get<bool>("debug wavfile") == true)
	{
		my_console.WriteToSplitConsole("BPM Audio Class: Audio format = " + std::to_string(audio_format), param_list.get<int>("split audio"));
		my_console.WriteToSplitConsole("BPM Audio Class: Channels = " + std::to_string(num_channels), param_list.get<int>("split audio"));
	}

	//Get sample rate and byte rate
	unsigned int sample_rate = read_word<unsigned int>(file, true);
	unsigned int byte_rate = read_word<unsigned int>(file, true);
	if (param_list.get<bool>("debug wavfile") == true)
	{
		my_console.WriteToSplitConsole("BPM Audio Class: Sample rate = " + std::to_string(sample_rate) + " Hz.", param_list.get<int>("split audio"));
		my_console.WriteToSplitConsole("BPM Audio Class: Byte rate = " + std::to_string(byte_rate) + " Hz.", param_list.get<int>("split audio"));
	}

	//Get frame size and bits per sample
	short frame_size = read_word<short>(file, true);
	short bits_sample = read_word<short>(file, true);
	if (param_list.get<bool>("debug wavfile") == true)
	{
		my_console.WriteToSplitConsole("BPM Audio Class: Frame size = " + std::to_string(frame_size) + " bytes.", param_list.get<int>("split audio"));
		my_console.WriteToSplitConsole("BPM Audio Class: Bits per sample = " + std::to_string(bits_sample) + " bits.", param_list.get<int>("split audio"));
	}

	//Now at last, get the data chunk size - first 4 bytes "data" ignored
	ignore = read_word<int>(file, false);
	unsigned int data_size = read_word<unsigned int>(file, true);
	if (param_list.get<bool>("debug wavfile") == true)
		my_console.WriteToSplitConsole("BPM Audio Class: Data bytes = " + std::to_string(data_size) + " bytes.", param_list.get<int>("split audio"));

	//Create wav file struct
	wav_file.file_size = file_size;
	wav_file.header_size = header_size;
	wav_file.audio_format = audio_format;
	wav_file.num_channels = num_channels;
	wav_file.sample_rate = sample_rate;
	wav_file.byte_rate = byte_rate;
	wav_file.frame_size = frame_size;
	wav_file.bits_sample = bits_sample;
	wav_file.data_size = data_size;

	//Prepare buffer - malloc takes size in bytes
	//Important! Caller has to handle corresponding 'free' for this memory block!
	wav_file.buffer = (short*)malloc(data_size);

	if (param_list.get<bool>("debug wavfile") == true)
		my_console.WriteToSplitConsole("BPM Audio Class: Reading " + std::to_string(data_size / frame_size) + " frames.", param_list.get<int>("split audio"));

	//Transfer data to struct
	for (long i = 0; i < (data_size / frame_size); ++i)
		wav_file.buffer[i] = read_word<short>(file, true);

	if (param_list.get<bool>("debug wavfile") == true)
		my_console.WriteToSplitConsole("BPM Audio Class: Success.", param_list.get<int>("split audio"));

	return eSuccess;
}

eError BPMAudio::read_wav_file(std::ifstream& file)
{
	//Declare return value
	eError retval = eSuccess;

	//Check if the audio interface has been properly set up
	if (file.is_open() == false)
		return eAudio_ErrorInputStreamNotOpen;

	//Prepare struct
	WAVFile temp_wav_file;

	//Call member function to read wav file
	read_wav_file(file, temp_wav_file);

	//Now we just copy the received samples into the buffer
	for (long i = 0; i < (temp_wav_file.data_size / temp_wav_file.frame_size); ++i)
		this->buffer[i] = temp_wav_file.buffer[i];

	//We have to free the memory allocated with malloc in the other read_wav_file function
	free(temp_wav_file.buffer);

	return eSuccess;
}

eError BPMAudio::read_std_wav_file()
{
	//Declare return value
	eError retval = eSuccess;

	//Here, we set the wav file ourselves
	std::ifstream file(STANDARD_WAVFILE, std::ios_base::out);

	//Check if the audio interface has been properly set up
	if (file.is_open() == false)
		return eAudio_ErrorInputStreamNotOpen;

	//Prepare struct
	WAVFile temp_wav_file;

	//Call member function to read wav file
	read_wav_file(file, temp_wav_file);

	//Now we just copy the received samples into the buffer
	for (long i = 0; i < (temp_wav_file.data_size / temp_wav_file.frame_size); ++i)
		this->buffer[i] = temp_wav_file.buffer[i];

	//We have to free the memory allocated with malloc in the other read_wav_file function
	free(temp_wav_file.buffer);

	//Close the file handle
	file.close();

	return eSuccess;

}