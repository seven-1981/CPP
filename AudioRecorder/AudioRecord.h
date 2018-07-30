#ifndef _AUDIO_RECORD
#define _AUDIO_RECORD

#include "WAVFile.h"
#pragma comment(lib,"winmm.lib")
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>

#define NUM_OF_BUFFERS 2


class AudioRecord
{
public:
	AudioRecord(long sample_rate, long duration);
	~AudioRecord();
	MMRESULT init();
	MMRESULT start();
	MMRESULT stop();
	bool full();
	short* flush(int index);

private:
	// Recording parameters
	WAVEFORMATEX wfx;

	short* waveIn[NUM_OF_BUFFERS];
	WAVEHDR WaveInHdr[NUM_OF_BUFFERS];
	HWAVEIN hWaveIn;

	MMRESULT result;

	static void CALLBACK buffer_callback(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
};

#endif
