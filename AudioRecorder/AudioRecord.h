#ifndef _AUDIO_RECORD
#define _AUDIO_RECORD

#include "WAVFile.h"
#pragma comment(lib,"winmm.lib")
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>

#define NUM_OF_BUFFERS 10


class AudioRecord
{
public:
	AudioRecord(long sample_rate, long duration);
	~AudioRecord();

	MMRESULT start();
	bool is_full();
	bool is_full(int index);
	short* flush(int index);

private:
	// Recording parameters
	WAVEFORMATEX wfx;
	// Buffer for data and header
	short* waveIn[NUM_OF_BUFFERS];
	WAVEHDR WaveInHdr[NUM_OF_BUFFERS];
	// Input device
	HWAVEIN hWaveIn;
	// Result of function calls
	MMRESULT result;

	// Called upon instantiation
	void init();
};

#endif
