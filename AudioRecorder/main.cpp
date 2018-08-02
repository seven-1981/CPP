#include <iostream>
#include <fstream>
#include "buffer.h"
#include "WAVFile.h"
#include "AudioRecord.h"

long sample_rate = 44100;
long duration = 5;

int main()
{
	AudioRecord AR(sample_rate, duration);
	std::cout << "START. " << AR.start() << std::endl;
	while (AR.is_full() == false)
	{
		// Wait until samples have been recorded
	}

	short* buf1 = AR.flush(0);
	short* buf2 = AR.flush(1);

	WAVFile wavfile;
	buffer<short> buffer(sample_rate * duration, sample_rate);
	for (long i = 0; i < sample_rate * duration; i++)
		buffer[i] = buf1[i];

	wavfile.set_buffer(buffer);
	remove("recordwav.wav");
	std::ofstream recordwav("recordwav.wav", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	wavfile.write_wav_file(recordwav);
	recordwav.close();
}
