#include "AudioRecord.h"

AudioRecord::AudioRecord(long sample_rate, long duration)
{
	// Specify recording parameters
	this->wfx.wFormatTag = PCM_WAV_FORMAT_TAG;
	this->wfx.nChannels = PCM_WAV_CHANNELS;
	this->wfx.nSamplesPerSec = sample_rate;
	this->wfx.nAvgBytesPerSec = sample_rate * PCM_WAV_FRAME_SIZE;
	this->wfx.nBlockAlign = PCM_WAV_FRAME_SIZE;
	this->wfx.wBitsPerSample = PCM_WAV_BITS;

	// Allocate some memory for the wav data
	this->waveIn[0] = (short*)malloc(duration * sample_rate * sizeof(short));
	this->waveIn[1] = (short*)malloc(duration * sample_rate * sizeof(short));
	std::cout << "Allocated 2 x " << duration * sample_rate * sizeof(short) << " bytes for audio record data." << std::endl;

	// Set up and prepare headers for input
	for (int i = 0; i < NUM_OF_BUFFERS; i++)
	{
		this->WaveInHdr[i].lpData = (LPSTR)this->waveIn[i];
		this->WaveInHdr[i].dwBufferLength = duration * sample_rate;
		this->WaveInHdr[i].dwBytesRecorded = 0;
		this->WaveInHdr[i].dwUser = 0L;
		this->WaveInHdr[i].dwFlags = 0L;
		this->WaveInHdr[i].dwLoops = 0L;
	}

	this->result = 0;
}

AudioRecord::~AudioRecord()
{
	waveInClose(this->hWaveIn);

	for (int i = 0; i < NUM_OF_BUFFERS; i++)
	{
		waveInUnprepareHeader(this->hWaveIn, &this->WaveInHdr[i], sizeof(this->WaveInHdr[i]));
		free(this->waveIn[i]);
	}

	std::cout << "Releasing resources. Freed audio record memory." << std::endl;
}

MMRESULT AudioRecord::init()
{
	// Open waveform input recording device - fills hWaveIn
	this->result = waveInOpen(&this->hWaveIn, WAVE_MAPPER, &this->wfx, (DWORD_PTR)(AudioRecord::buffer_callback), NULL, CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT);
	if (this->result != 0)
		return this->result;

	for (int i = 0; i < NUM_OF_BUFFERS; i++)
	{
		// Prepare the headers
		this->result = waveInPrepareHeader(this->hWaveIn, &this->WaveInHdr[i], sizeof(this->WaveInHdr[i]));
		if (this->result != 0)
			return this->result;

		// Insert the wave input buffers
		this->result = waveInAddBuffer(this->hWaveIn, &this->WaveInHdr[i], sizeof(this->WaveInHdr[i]));
		if (this->result != 0)
			return this->result;
	}

	std::cout << "Initialization done." << std::endl;

	return this->result;
}

MMRESULT AudioRecord::start()
{
	if (this->result != 0)
		return this->result;

	this->result = waveInStart(this->hWaveIn);

	return result;
}

bool AudioRecord::full()
{
	bool retVal = false;
	if ((this->WaveInHdr[0].dwFlags & WHDR_DONE) && (this->WaveInHdr[1].dwFlags & WHDR_DONE))
			retVal = true;

	return retVal;
}

MMRESULT AudioRecord::stop()
{
	if (this->result != 0)
		return this->result;
	
	this->result = waveInStop(this->hWaveIn);

	return result;
}

short* AudioRecord::flush(int index)
{
	return this->waveIn[index];
}

void CALLBACK AudioRecord::buffer_callback(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg == WIM_DATA)
		std::cout << ".";
}
