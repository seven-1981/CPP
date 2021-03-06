#ifndef _WAVFILE_H
#define _WAVFILE_H

#include "buffer.hpp"
#include <fstream>

//Defines for .wav file creation
//Length of header
#define PCM_WAV_FMT_LENGTH 16
//Format of .wav - 1 = canonical PCM
#define PCM_WAV_FORMAT_TAG 1
//Number of bits per sample
#define PCM_WAV_BITS 16
//Size of one frame = channels * (bits/sample + 7) / 8, without division rest
#define PCM_WAV_FRAME_SIZE 2
//Number of channels
#define PCM_WAV_CHANNELS 1
//Size of .wav header in bytes (until data start)
#define PCM_WAV_HEADER_SIZE 44

//Struct for wav header info
struct WAVHeader
{
	//Header information
	unsigned int file_size = 0;
	unsigned int header_size = PCM_WAV_FMT_LENGTH;
	short        audio_format = PCM_WAV_FORMAT_TAG;
	short	     num_channels = PCM_WAV_CHANNELS;
	unsigned int sample_rate = 0;
	unsigned int byte_rate = 0;
	short        frame_size = PCM_WAV_FRAME_SIZE;
	short        bits_sample = PCM_WAV_BITS;
	unsigned int data_size = 0;
};

class WAVFile
{
public:
	WAVFile();
	~WAVFile();

	int read_wav_file(std::ifstream& file);
	int write_wav_file(std::ofstream& file);

	WAVHeader* get_header_info() { return &this->header; }
	buffer<short>* get_buffer() { return &this->data; }
	void set_buffer(buffer<short>& data);

	unsigned int get_size() { return this->header.data_size / this->header.frame_size; }

private:
	WAVHeader header;
	buffer<short> data;

	template <typename Word>
	Word read_word(std::istream& in_stream, bool little_endian, unsigned int size = sizeof(Word));

	template <typename Word>
	std::ostream& write_word(std::ostream& out_stream, Word value, unsigned int size = sizeof(Word));

	void create_header_info();

	void dump_info();
};

#endif