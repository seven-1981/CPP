#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <ctime>

#include "FCWindow.hpp"
#include "FCWindowManager.hpp"
#include "FCWindowLabel.hpp"

//Constants for time/date string extraction
const int DATE_START = 0;
const int DATE_LENGTH = 10;
const int TIME_START = 11;
const int TIME_LENGTH = 5;
//Constants for displaying text
const std::string TITLE = "RASPI - PI HOLE";
const std::string TITLE_DISP = "* PI - HOLE *";
//Constants for location of text
const int TITLE_X = 240; 
const int TITLE_Y = 50;
const int X_MARGIN = 650; 
const int Y_MARGIN = 150; 
const int Y_OFFSET = 100;
const int Y_LINE_OFFSET = 40;
const int SECONDS_WAIT = 1;

void create_timestamp(std::string& date, std::string& time)
{
	using namespace std;
	using namespace std::chrono;
	//Here system_clock is wall clock time from 
    //the system-wide realtime clock 
    auto timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
    std::string temp_string = std::string(ctime(&timenow));
    //Get date string and time string and add newline
    date = temp_string.substr(DATE_START, DATE_LENGTH);
    time = temp_string.substr(TIME_START, TIME_LENGTH);
}

//Deprecated, not displaying correct time zone
void create_timestamp_old(std::string& msg)
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();

	typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<8>>::type> Days; /* UTC: +8:00 */

	Days days = std::chrono::duration_cast<Days>(duration);
	duration -= days;
	auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
	duration -= hours;
	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	duration -= seconds;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	duration -= milliseconds;
	auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
	duration -= microseconds;
	auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
	
	std::string h = std::to_string(hours.count() + 10 % 24);
	if (h.length() == 1)
		h = "0" + h;
	std::string m = std::to_string(minutes.count());
	if (m.length() == 1)
		m = "0" + m;
	std::string s = std::to_string(seconds.count());
	if (s.length() == 1)
		s = "0" + s;
	std::string ms = std::to_string(milliseconds.count());
	if (ms.length() == 1)
		ms = "00" + ms;
	else if (ms.length() == 2)
		ms = "0" + ms;
	
	msg += h; msg += ":";
	msg += m; msg += ":";
	msg += s; //msg += ".";
	//msg += ms;
}

int main(int argc, char **argv)

{

  	FCWindowManager::init(argc, argv);
	FCWindowParam_t win_param { 500, 250, TITLE, true };
	FCWindow* window1 = FCWindowManager::create(TypeWindowLabel, win_param);
	//Start must be called after first window has been created
	FCWindowManager::start();
	while (window1->get_quit() == false)
	{
		//Create timestamp and time string
		std::string date, time;
		create_timestamp(date, time);
		date = "DATE: " + date;
		time = "TIME: " + time;

		//Create text item data for window
		FCWindowLabelData_t timedata;
		FCWindowLabelDataItem_t element;
		//Create title
		element.text = TITLE_DISP;
		element.x = TITLE_X; element.y = TITLE_Y;
		element.use_stroke = true;
		timedata.items.push_back(element);
		//Create time and random pos and color
		int rx = FCWindowManager::get_res_x();
		int ry = FCWindowManager::get_res_y();
		element.text = date;
		element.x = rand() % (rx-X_MARGIN); 
		element.y = rand() % (ry-Y_MARGIN) + Y_OFFSET;
		element.color_R = (float)(rand()) / (float)RAND_MAX;
		element.color_G = (float)(rand()) / (float)RAND_MAX;
		element.color_B = (float)(rand()) / (float)RAND_MAX;
		timedata.items.push_back(element);
		element.text = time;
		element.y = element.y + Y_LINE_OFFSET;
		timedata.items.push_back(element);
		
		//Update window data
		window1->update(&timedata);
		std::this_thread::sleep_for(std::chrono::seconds(SECONDS_WAIT));
	}
  	std::this_thread::sleep_for(std::chrono::seconds(1));
	FCWindowManager::stop();
  	std::this_thread::sleep_for(std::chrono::seconds(1));
	delete window1;

  	return 0;

}
