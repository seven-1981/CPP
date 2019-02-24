#ifndef _FCHDMI_H
#define _FCHDMI_H

#ifndef _WIN32

#include <fstream>
#include <string>

namespace HDMI
{
	//Filename for hdmi status file
	std::string fname("hdmi.txt");

	//Status strings
	//Called from script file 'tvservice -s'
	//Return values:
	//0x120006 if HDMI is plugged in and on
	//0x 40001 if HDMI is plugged off
	//0x 40002 if HDMI was plugged on after reboot
	//Note: hotplug is disabled!
	std::string hdmi_on_string = "state 0x120006";

	//Getter method
	bool get_HDMI_state()
	{
		//Open file
		std::ifstream ifs(fname, std::ios_base::in);

		//Check if file is open
		if (ifs.is_open() == false)
			return false;

		//Get content
		std::string hdmi_string;
		std::getline(ifs, hdmi_string);
		ifs.close();

		//Check if hdmi_on_string is present
		std::size_t pos = hdmi_string.find(hdmi_on_string);
		if (pos == std::string::npos)
			return false;
		else
			return true;
	}
}

#endif

#endif