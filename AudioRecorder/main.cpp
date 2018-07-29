/*#pragma comment(lib,"winmm.lib")
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>

int main()
{
	std::cout << "Audio recorder..." << std::endl;

	const int NUMPTS = 44100 * 10;
	int sampleRate = 44100;
	short int waveIn[NUMPTS];

	HWAVEIN hWaveIn;
	WAVEHDR WaveInHdr;
	MMRESULT result;

	// Specify recording parameters
	WAVEFORMATEX pFormat;
	pFormat.wFormatTag = WAVE_FORMAT_PCM;     // simple, uncompressed format
	pFormat.nChannels = 1;                    //  1=mono, 2=stereo
	pFormat.nSamplesPerSec = sampleRate;      // 44100
	pFormat.nAvgBytesPerSec = sampleRate * 2; // = nSamplesPerSec * n.Channels * wBitsPerSample/8
	pFormat.nBlockAlign = 2;                  // = n.Channels * wBitsPerSample/8
	pFormat.wBitsPerSample = 16;              //  16 for high quality, 8 for telephone-grade
	pFormat.cbSize = 0;

	result = waveInOpen(&hWaveIn, WAVE_MAPPER, &pFormat, 0L, 0L, CALLBACK_NULL | WAVE_FORMAT_DIRECT);

	if (result)
	{
		std::cout << "Failed to open waveform input device." << std::endl;
		exit(-1);
	}

	// Set up and prepare header for input
	WaveInHdr.lpData = (LPSTR)waveIn;
	WaveInHdr.dwBufferLength = NUMPTS * 2;
	WaveInHdr.dwBytesRecorded = 0;
	WaveInHdr.dwUser = 0L;
	WaveInHdr.dwFlags = 0L;
	WaveInHdr.dwLoops = 0L;
	waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));

	// Insert a wave input buffer
	result = waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
	if (result)
	{
		std::cout << "Failed to read block from device." << std::endl;
		exit(-1);
	}

	// Commence sampling input
	result = waveInStart(hWaveIn);
	if (result)
	{
		std::cout << "Failed to start recording." << std::endl;
		exit(-1);
	}

	// Wait until finished recording
	do { std::cout << "."; } while (waveInUnprepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR) == WAVERR_STILLPLAYING));

	waveInClose(hWaveIn);

	std::cout << "End." << std::endl;
	return 0;
}*/





#pragma comment(lib,"winmm.lib")

#include <Windows.h>
#include <mmsystem.h>
#include <fstream>
#include <iostream>

int main()
{
	// Fill the WAVEFORMATEX struct to indicate the format of our recorded audio
	//   For this example we'll use "CD quality", ie:  44100 Hz, stereo, 16-bit
	WAVEFORMATEX wfx = {};
	wfx.wFormatTag = WAVE_FORMAT_PCM;       // PCM is standard
	wfx.nChannels = 2;                      // 2 channels = stereo sound
	wfx.nSamplesPerSec = 44100;             // Samplerate.  44100 Hz
	wfx.wBitsPerSample = 16;                // 16 bit samples
											// These others are computations:
	wfx.nBlockAlign = wfx.wBitsPerSample * wfx.nChannels / 8;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;


	// Open our 'waveIn' recording device
	HWAVEIN wi;
	waveInOpen(&wi,         // fill our 'wi' handle
		WAVE_MAPPER,		// use default device (easiest)
		&wfx,				// tell it our format
		NULL, NULL,			// we don't need a callback for this example
		CALLBACK_NULL | WAVE_FORMAT_DIRECT   // tell it we do not need a callback
	);

	// At this point, we have our device, now we need to give it buffers (with headers) that it can
	//  put the recorded audio somewhere
	char buffers[2][44100 * 2 * 2 / 2];    // 2 buffers, each half of a second long
	WAVEHDR headers[2] = { {},{} };           // initialize them to zeros
	for (int i = 0; i < 2; ++i)
	{
		headers[i].lpData = buffers[i];						// give it a pointer to our buffer
		headers[i].dwBufferLength = 44100 * 2 * 2 / 2;      // tell it the size of that buffer in bytes
															// the other parts of the header we don't really care about for this example, and can be left at zero

															// Prepare each header
		waveInPrepareHeader(wi, &headers[i], sizeof(headers[i]));

		// And add it to the queue
		//  Once we start recording, queued buffers will get filled with audio data
		waveInAddBuffer(wi, &headers[i], sizeof(headers[i]));
	}

	// In this example, I'm just going to dump the audio data to a binary file
	std::ofstream outfile("my_recorded_audio.bin", std::ios_base::out | std::ios_base::binary);

	// Print some simple directions to the user
	std::cout << "Now recording audio.  Press Escape to stop and exit." << std::endl;

	// start recording!
	waveInStart(wi);

	// Now that we are recording, keep polling our buffers to see if they have been filled.
	//   If they have been, dump their contents to the file and re-add them to the queue so they
	//   can get filled again, and again, and again
	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000))  // keep looping until the user hits escape
	{
		for (auto& h : headers)      // check each header
		{
			if (h.dwFlags & WHDR_DONE)           // is this header done?
			{
				// if yes, dump it to our file
				outfile.write(h.lpData, h.dwBufferLength);

				// then re-add it to the queue
				h.dwFlags = 0;          // clear the 'done' flag
				h.dwBytesRecorded = 0;  // tell it no bytes have been recorded

										// re-add it  (I don't know why you need to prepare it again though...)
				waveInPrepareHeader(wi, &h, sizeof(h));
				waveInAddBuffer(wi, &h, sizeof(h));
			}
		}
	}

	// Once the user hits escape, stop recording, and clean up
	waveInStop(wi);
	for (auto& h : headers)
	{
		waveInUnprepareHeader(wi, &h, sizeof(h));
	}
	waveInClose(wi);

	// All done!
}
