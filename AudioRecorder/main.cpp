#include <iostream>
#include <fstream>
#include "buffer.h"
#include "WAVFile.h"
#include "AudioRecord.h"

int main()
{
	AudioRecord AR(44100, 4);
	std::cout << "START. " << AR.start() << std::endl;
	while (AR.is_full() == false)
	{
		// Wait until samples have been recorded
	}

	short* buf1 = AR.flush(0);
	short* buf2 = AR.flush(1);

	WAVFile wavfile;
	buffer<short> buffer(4 * 44100, 44100);
	for (long i = 0; i < 4 * 44100; i++)
		buffer[i] = buf1[i];

	maximize_volume(buffer);
	wavfile.set_buffer(buffer);
	remove("./wavs/recordwav.wav");
	std::ofstream recordwav("./wavs/recordwav.wav", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	wavfile.write_wav_file(recordwav);
	recordwav.close();
}
