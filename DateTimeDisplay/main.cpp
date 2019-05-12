#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <ctime>
#include <map>

#include "FCWindow.hpp"
#include "FCWindowManager.hpp"
#include "FCWindowLabel.hpp"

#include "WeatherCollector.hpp"
#include "JSONParser.hpp"

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
const int SECONDS_WAIT = 3;
//URL for http weather request - sample
const std::string url_openweathermap = "https://samples.openweathermap.org/data/2.5/weather?lat=47.319542&lon=8.051965&appid=b6907d289e10d714a6e88b30761fae22";
//URL for http weather request - real data
const std::string url_req = "https://api.openweathermap.org/data/2.5/weather?lat=47.319542&lon=8.051965&appid=889a052df8efc1c1c0235058b557c1c9";
//Constants for weather collection
const int CYCLE_REQUEST = 60;

//Weather item holding data for parsing and display
//info whether it is a value or a string
struct WeatherItem
{
	const std::string id;		//Openweathermap identifier
	const std::string title;	//Title to diplay on screen
	const std::string unit;		//Unit for measurement value
	bool is_value;				//TRUE = is measurement value
};
//Map for weather data identifiers
std::map<unsigned int, WeatherItem> weather_items;
const int NUM_WEATHER_ITEMS = 10;
//Fill map with desired items to display
void init_map()
{
	WeatherItem weather_cond { "description", "WEATHER"       , ""     , false };
	WeatherItem temperature  { "temp"       , "TEMPERATURE"   , " degC", true };
	WeatherItem temp_min     { "temp_min"   , "TEMP. MIN"     , " degC", true };
	WeatherItem temp_max     { "temp_max"   , "TEMP. MAX"     , " degC", true };
	WeatherItem pressure     { "pressure"   , "PRESSURE"      , " hPa" , true };
	WeatherItem humidity     { "humidity"   , "HUMIDITY"      , " %"   , true };
	WeatherItem wind_speed   { "speed"      , "WIND SPEED"    , " m/s" , true };
	WeatherItem wind_dir     { "deg"        , "WIND DIRECTION", " deg" , true };
	WeatherItem cloudiness   { "all"        , "CLOUDINESS"    , " %"   , true };
	WeatherItem station      { "name"       , "STATION"       , ""     , false };
	weather_items.emplace(0, weather_cond);
	weather_items.emplace(1, temperature);
	weather_items.emplace(2, temp_min);
	weather_items.emplace(3, temp_max);
	weather_items.emplace(4, pressure);
	weather_items.emplace(5, humidity);
	weather_items.emplace(6, wind_speed);
	weather_items.emplace(7, wind_dir);
	weather_items.emplace(8, cloudiness);
	weather_items.emplace(9, station);
}

//Create date and time string
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
	//Initialize weather collector and map
	WeatherCollector collector(url_req);
	std::string weather_data { };
	unsigned int main_cycle = 0;
	unsigned int weather_cycle = 0;
	bool successful = false;
	init_map();
	//Initialize window manager and start window

  	FCWindowManager::init(argc, argv);
	FCWindowParam_t win_param { 500, 250, TITLE, true };
	FCWindow* window1 = FCWindowManager::create(TypeWindowLabel, win_param);
	//Start must be called after first window has been created
	FCWindowManager::start();
	
	while (window1->get_quit() == false)
	{
		if (main_cycle % 2 == 1) //First cycle must initialize weather data
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
			//Create date & time and random pos and color
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
		}
		else
		{
			if (main_cycle % CYCLE_REQUEST == 0)
			{
				//Update weather data every n'th cycle only
				successful = collector.request(weather_data);
				//std::cout << weather_data << std::endl;
			}
			if (successful == true)
			{
				FCWindowLabelData_t weatherdata;
				FCWindowLabelDataItem_t element;
				//Create title
				element.text = TITLE_DISP;
				element.x = TITLE_X; element.y = TITLE_Y;
				element.use_stroke = true;
				weatherdata.items.push_back(element);
				//Create weather data
				int rx = FCWindowManager::get_res_x();
				int ry = FCWindowManager::get_res_y();
				//Get map entry according to cycle
				auto found = weather_items.find(weather_cycle);
				if (found == weather_items.end())
					continue; //Not found - display date & time only
				WeatherItem item = found->second;
				//Create weather information and random pos and color
				element.text = item.title;
				element.x = rand() % (rx-X_MARGIN); 
				element.y = rand() % (ry-Y_MARGIN) + Y_OFFSET;
				element.color_R = (float)(rand()) / (float)RAND_MAX;
				element.color_G = (float)(rand()) / (float)RAND_MAX;
				element.color_B = (float)(rand()) / (float)RAND_MAX;
				weatherdata.items.push_back(element);
				std::string disp_string { };
				if (item.is_value == true)
				{
					double val = JSONParser::get_value(weather_data, item.id);
					disp_string = JSONParser::round_value(val, 1);
				}
				else
				{
					disp_string = JSONParser::get_string(weather_data, item.id);
				}
				disp_string += item.unit;
				element.text = disp_string;
				element.y = element.y + Y_LINE_OFFSET;
				weatherdata.items.push_back(element);
				window1->update(&weatherdata);
				weather_cycle++;
				if (weather_cycle >= NUM_WEATHER_ITEMS)
					weather_cycle = 0;
			}
			else
				continue; //If no weather data available, display date & time only
		}
		std::this_thread::sleep_for(std::chrono::seconds(SECONDS_WAIT));
		main_cycle++;
	}
  	std::this_thread::sleep_for(std::chrono::seconds(1));
	FCWindowManager::stop();
  	std::this_thread::sleep_for(std::chrono::seconds(1));
	delete window1;

  	return 0;

}
