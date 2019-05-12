#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <ctime>
#include <map>
#include <random>

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
const int X_MARGIN = 50; 
const int X_MARGIN_STATION = 350;
const int X_MARGIN_SECOND_ROW = 600; 
const int Y_MARGIN = 100; 
const int Y_LINE_OFFSET = 43;
const int SECONDS_WAIT = 3;
//URL for http weather request - sample
const std::string url_openweathermap = "https://samples.openweathermap.org/data/2.5/weather?lat=47.319542&lon=8.051965&appid=b6907d289e10d714a6e88b30761fae22";
//URL for http weather request - real data
const std::string url_req = "https://api.openweathermap.org/data/2.5/weather?lat=47.319542&lon=8.051965&appid=889a052df8efc1c1c0235058b557c1c9&lang=de";
//Constants for coordinate selection
const int LAT_MIN = 46.731;
const int LAT_MAX = 47.580;
const int LON_MIN = 7.0400;
const int LON_MAX = 9.4111;
//Random generator seed
std::random_device rd;
std::mt19937 gen(rd());
//Uniform distribution for coordinates
std::uniform_real_distribution<> dis_lat(LAT_MIN, LAT_MAX);
std::uniform_real_distribution<> dis_lon(LON_MIN, LON_MAX);	

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
	WeatherItem weather_cond { "description", ""              , ""     , false };
	WeatherItem temperature  { "temp"       , "TEMPERATUR:"   , " degC", true };
	WeatherItem temp_min     { "temp_min"   , "TEMP. MIN:"    , " degC", true };
	WeatherItem temp_max     { "temp_max"   , "TEMP. MAX:"    , " degC", true };
	WeatherItem pressure     { "pressure"   , "LUFTDRUCK:"    , " hPa" , true };
	WeatherItem humidity     { "humidity"   , "FEUCHTE:"      , " %"   , true };
	WeatherItem wind_speed   { "speed"      , "WINDSTAERKE:"  , " m/s" , true };
	WeatherItem wind_dir     { "deg"        , "WINDRICHTUNG:" , " deg" , true };
	WeatherItem cloudiness   { "all"        , "WOLKENDECKE:"  , " %"   , true };
	WeatherItem station      { "name"       , "STATION: "     , ""     , false };
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

double get_random(bool lat) 
{
	//Returns a random double between min and max
	std::random_device rd;
	std::mt19937 gen(rd());
	if (lat == true)
		return (double)dis_lat(gen);
	else
		return (double)dis_lon(gen);
}

void random_url_pos(std::string& url)
{
	double lat = get_random(true);
	double lon = get_random(false);
	std::string lat_str = std::to_string(lat);
	std::string lon_str = std::to_string(lon);
	std::size_t lat_pos = url.find("lat=");
	std::size_t lat_end = url.find("&", lat_pos + 1);
	std::size_t lat_len = lat_end - lat_pos - 4;
	std::size_t lon_pos = url.find("lon=");
	std::size_t lon_end = url.find("&", lon_pos + 1);
	std::size_t lon_len = lon_end - lon_pos - 4;
	url = url.replace(lat_pos + 4, lat_len, lat_str);
	url = url.replace(lon_pos + 4, lon_len, lon_str);
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
		//Create timestamp and time string
		std::string date, time;
		create_timestamp(date, time);
		date = "DATE: " + date;
		time = "TIME: " + time;
		//Create text item data for window
		FCWindowLabelData_t display_data;
		FCWindowLabelDataItem_t element;
		//Create title
		element.text = TITLE_DISP;
		element.x = TITLE_X; element.y = TITLE_Y;
		element.use_stroke = true;
		display_data.items.push_back(element);
		//Create date & time and random color
		element.text = date;
		element.x = X_MARGIN;
		element.y = Y_MARGIN;
		element.color_R = (float)(rand()) / (float)RAND_MAX;
		element.color_G = (float)(rand()) / (float)RAND_MAX;
		element.color_B = (float)(rand()) / (float)RAND_MAX;
		display_data.items.push_back(element);
		element.text = time;
		element.y += Y_LINE_OFFSET;
		display_data.items.push_back(element);
		
		//Create weather data
		std::string url_to_request = url_req;
		if (main_cycle % 2 == 0)
		{
			//Every second cycle, display some random location's weather data
			random_url_pos(url_to_request);
		}
		collector.update_url(url_to_request);
		bool successful = collector.request(weather_data);
		//std::cout << weather_data << std::endl;
		
		if (successful == true)
		{
			for (int i = 0; i < NUM_WEATHER_ITEMS; i++)
			{
				//Get map entry according to cycle
				auto found = weather_items.find(i);
				if (found == weather_items.end())
					continue; //Not found
				WeatherItem item = found->second;
				//Create weather information and location
				element.text = item.title;
				element.x = X_MARGIN;
				element.y = Y_MARGIN + (i+2) * Y_LINE_OFFSET;
				display_data.items.push_back(element);
				std::string disp_string { };
				if (item.is_value == true)
				{
					double val = JSONParser::get_value(weather_data, item.id);
					disp_string = JSONParser::round_value(val, 1);
				}
				else
					disp_string = JSONParser::get_string(weather_data, item.id);
				disp_string += item.unit;
				if (i == 0)
					element.x = X_MARGIN;
				else if (i == NUM_WEATHER_ITEMS - 1)
					element.x = X_MARGIN_STATION;
				else
					element.x = X_MARGIN_SECOND_ROW;
				element.text = disp_string;
				display_data.items.push_back(element);
			}
		}
			
		//Update window data
		window1->update(&display_data);
		std::this_thread::sleep_for(std::chrono::seconds(SECONDS_WAIT));
		main_cycle++;
	}
  	std::this_thread::sleep_for(std::chrono::seconds(1));
	FCWindowManager::stop();
  	std::this_thread::sleep_for(std::chrono::seconds(1));
	delete window1;

  	return 0;

}
