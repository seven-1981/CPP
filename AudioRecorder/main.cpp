#include <iostream>
#include "WAVFile.h"
#include "AudioRecord.h"

int main()
{
	AudioRecord AR(44100, 4);
	std::cout << "Executing init... " << AR.init() << std::endl;
	std::cout << "START. " << AR.start() << std::endl;;
	while (AR.full() == false)
	{
		// Wait until samples have been recorded
	}

	std::cout << "STOP. " << AR.stop() << std::endl;

	short* buf1 = AR.flush(0);
	short* buf2 = AR.flush(1);

	remove("waveform1.bin");
	std::ofstream buffer1("waveform1.bin", std::ios_base::out | std::ios_base::binary);
	for (int i = 0; i < 44100 * 4 * 2; i++)
		buffer1.write((char*)&buf1[i], sizeof(short));

	buffer1.close();
	
	return 0;
}
