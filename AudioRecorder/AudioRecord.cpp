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
	for (int i = 0; i < NUM_OF_BUFFERS; i++)
		this->waveIn[i] = (short*)malloc(duration * sample_rate * sizeof(short));
	std::cout << "Allocated " << NUM_OF_BUFFERS << "x " << duration * sample_rate * sizeof(short) << " bytes for audio record data." << std::endl;

	// Set up and prepare headers for input
	for (int i = 0; i < NUM_OF_BUFFERS; i++)
	{
		this->WaveInHdr[i].lpData = (LPSTR)this->waveIn[i];
		this->WaveInHdr[i].dwBufferLength = duration * sample_rate * sizeof(short);
		this->WaveInHdr[i].dwBytesRecorded = 0;
		this->WaveInHdr[i].dwUser = 0L;
		this->WaveInHdr[i].dwFlags = 0L;
		this->WaveInHdr[i].dwLoops = 0L;
	}

	// Call initialisation
	this->init();
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

MMRESULT AudioRecord::start()
{
	if (this->result != 0)
		return this->result;

	this->result = waveInStart(this->hWaveIn);

	return result;
}

bool AudioRecord::is_full()
{
	bool buf_retval[NUM_OF_BUFFERS];
	bool retVal = true;

	for (int i = 0; i < NUM_OF_BUFFERS; i++)
	{
		buf_retval[i] = this->WaveInHdr[i].dwFlags & WHDR_DONE;
		retVal = buf_retval[i] && retVal;
	}

	return retVal;
}

bool AudioRecord::is_full(int index)
{
	if (index < NUM_OF_BUFFERS)
		return (bool)(this->WaveInHdr[index].dwFlags & WHDR_DONE);
	else
		return false;
}

short* AudioRecord::flush(int index)
{
	return this->waveIn[index];
}

void AudioRecord::init()
{
	// Open waveform input recording device - fills hWaveIn
	this->result = waveInOpen(&this->hWaveIn, WAVE_MAPPER, &this->wfx, NULL, NULL, CALLBACK_NULL | WAVE_FORMAT_DIRECT);
	if (this->result != 0)
		return;

	for (int i = 0; i < NUM_OF_BUFFERS; i++)
	{
		// Prepare the headers
		this->result = waveInPrepareHeader(this->hWaveIn, &this->WaveInHdr[i], sizeof(this->WaveInHdr[i]));
		if (this->result != 0)
			return;

		// Insert the wave input buffers
		this->result = waveInAddBuffer(this->hWaveIn, &this->WaveInHdr[i], sizeof(this->WaveInHdr[i]));
		if (this->result != 0)
			return;
	}

	std::cout << "Initialization done." << std::endl;
}
