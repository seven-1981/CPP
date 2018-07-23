#include "WAVFile.h"
#include "buffer.h"
#include <iostream>
#include <string>

WAVFile::WAVFile()
{
	//Initialize header - to be completed with "read_wav_file"
	this->header = new WAVHeader;

	this->header->file_size = 0;
	this->header->header_size = PCM_WAV_FMT_LENGTH;
	this->header->audio_format = PCM_WAV_FORMAT_TAG;
	this->header->num_channels = PCM_WAV_CHANNELS;
	this->header->sample_rate = 0;
	this->header->byte_rate = 0;
	this->header->frame_size = PCM_WAV_FRAME_SIZE;
	this->header->bits_sample = PCM_WAV_BITS;
	this->header->data_size = 0;
}

WAVFile::~WAVFile()
{
	delete this->header;
}

int WAVFile::read_wav_file(std::ifstream& file)
{
	std::cout << "Reading wav file. " << std::endl;

	//Check if the file has been opened properly
	if (file.is_open() == false)
		return -1;

	//Read wav file
	//Ignore the first four bytes
	int ignore = read_word<int>(file, false);

	//Check if it is a .wav file
	if (ignore != 1380533830)
		return -1;

	//Now we read the total file size
	unsigned int file_size = read_word<unsigned int>(file, true);

	//Ignore next 8 bytes - WAVE, fmt_
	ignore = read_word<int>(file, false);
	ignore = read_word<int>(file, false);

	//Header size
	unsigned int header_size = read_word<unsigned int>(file, true);

	//Get audio format and number of channels
	short audio_format = read_word<short>(file, true);
	short num_channels = read_word<short>(file, true);

	//Get sample rate and byte rate
	unsigned int sample_rate = read_word<unsigned int>(file, true);
	unsigned int byte_rate = read_word<unsigned int>(file, true);

	//Get frame size and bits per sample
	short frame_size = read_word<short>(file, true);
	short bits_sample = read_word<short>(file, true);

	//Now at last, get the data chunk size - first 4 bytes "data" ignored
	ignore = read_word<int>(file, false);
	unsigned int data_size = read_word<unsigned int>(file, true);

	//Create header struct
	this->header->file_size = file_size + 8;
	this->header->header_size = header_size;
	this->header->audio_format = audio_format;
	this->header->num_channels = num_channels;
	this->header->sample_rate = sample_rate;
	this->header->byte_rate = byte_rate;
	this->header->frame_size = frame_size;
	this->header->bits_sample = bits_sample;
	this->header->data_size = data_size;

	dump_info();

	//Prepare buffer
	this->data = new buffer<short>(data_size / frame_size, sample_rate);

	//Transfer data to buffer
	for (long i = 0; i < (data_size / (unsigned int)frame_size); i++)
		this->data->values[i] = read_word<short>(file, true);

	std::cout << "Success." << std::endl;

	return 0;
}

int WAVFile::write_wav_file(std::ofstream& file)
{
	if (this->header->file_size == 0)
		return -1;

	std::cout << "Writing wav file. " << std::endl;

	//Write .wav file header
	//Note: file must be created in binary mode
	file << "RIFF----WAVEfmt ";	//Chunk size to be filled in later
	write_word(file, this->header->header_size, 4);		//No extension data
	write_word(file, this->header->audio_format, 2);	//PCM - integer samples
	write_word(file, this->header->num_channels, 2);	//Number of channels
	write_word(file, this->header->sample_rate, 4);		//Samples per second / Hz
	write_word(file, this->header->byte_rate, 4);		//Sample rate * bits per sample * channels / 8 - bytes/s
	write_word(file, this->header->frame_size, 2);		//Data block size in bytes
	write_word(file, this->header->bits_sample, 2);		//Number of bits per sample (multiple of 8)	

	//Write the data chunk header
	size_t data_chunk_pos = file.tellp();
	file << "data----";		//Chunk size to be filled in later

	//Write the audio samples
	for (long i = 0; i < this->data->get_size(); i++)
		write_word(file, (short)(this->data->values[i]), this->header->frame_size);

	//Final file size to fix the chunk size above
	size_t file_length = file.tellp();

	//Fix data chunk header to contain data size
	file.seekp(data_chunk_pos + 4);
	write_word(file, file_length - data_chunk_pos - 8);

	//Fix the file header to contain the proper RIFF chunk size, which is file size - 8 bytes
	file.seekp(0 + 4);
	write_word(file, file_length - 8, 4);

	dump_info();

	std::cout << "Success." << std::endl;

	return 0;
}

void WAVFile::set_buffer(buffer<short>& data)
{
	this->data = &data;
	create_header_info();
}

template <typename Word>
Word WAVFile::read_word(std::istream& in_stream, bool little_endian, unsigned int size)
{
	//Function to read a "word" from the input stream ("binary read"), works for both endianness.
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

template <typename Word>
std::ostream& WAVFile::write_word(std::ostream& out_stream, Word value, unsigned int size)
{
	//Function to write "word" into output stream ("binary write"), only works for little endian!
	//For big endian, the for loop must be reversed!
	//"Word" is a template, meaning it could stand for signed int, unsigned int, char etc.
	//We take the data type size as starting value and count down the bytes
	//Extracts one byte from the value and puts it into the output stream
	//Then a bit shift must be done to get the next byte
	for (; size != 0; --size, value >>= 8)
		out_stream.put(static_cast<char>(value & 0xFF));
	return out_stream;
}

void WAVFile::create_header_info()
{
	//The buffer has been set - calculate missing .wav header info
	this->header->sample_rate = this->data->get_sample_rate();
	this->header->byte_rate = this->data->get_sample_rate() * PCM_WAV_FRAME_SIZE;
	this->header->data_size = this->data->get_size() * PCM_WAV_FRAME_SIZE;
	this->header->file_size = this->header->data_size + PCM_WAV_HEADER_SIZE;
}

void WAVFile::dump_info()
{
	std::cout << "* * * WAVFile Info * * *" << std::endl;
	std::cout << "File size = " << std::to_string(this->header->file_size) << " bytes." << std::endl;
	std::cout << "Header size = " + std::to_string(this->header->header_size) << " bytes." << std::endl;
	std::cout << "Audio format = " << std::to_string(this->header->audio_format) << "." << std::endl;
	std::cout << "Channels = " << std::to_string(this->header->num_channels) << "." << std::endl;
	std::cout << "Sample rate = " << std::to_string(this->header->sample_rate) << " Hz." << std::endl;
	std::cout << "Byte rate = " << std::to_string(this->header->byte_rate) << " Hz." << std::endl;
	std::cout << "Frame size = " << std::to_string(this->header->frame_size) << " bytes." << std::endl;
	std::cout << "Bits per sample = " << std::to_string(this->header->bits_sample) << " bits." << std::endl;
	std::cout << "Data bytes = " << std::to_string(this->header->data_size) << " bytes." << std::endl;
	std::cout << "Contains " << std::to_string(this->header->data_size / this->header->frame_size) << " frames." << std::endl;
}
