#include <string>
#include <chrono>
#include <ctime>

#include "WeatherItemConfigurator.hpp"
#include "JSONParser.hpp"


WeatherItemConfigurator::WeatherItemConfigurator()
{
	init_map();
}

WeatherItemConfigurator::~WeatherItemConfigurator()
{
	
}

void WeatherItemConfigurator::init_map()
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

void WeatherItemConfigurator::create_timestamp(std::string& date, std::string& time)
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

void WeatherItemConfigurator::prepare_time_data(FCWindowLabelData_t& display_data, FCWindowLabelDataItem_t& element)
{
	//Create timestamp and time string
	std::string date, time;
	create_timestamp(date, time);
	date = "DATUM: " + date;
	time = "ZEIT: " + time;	
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
}

void WeatherItemConfigurator::prepare_weather_data(std::string& weather_data, FCWindowLabelData_t& display_data, FCWindowLabelDataItem_t& element)
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
		double val = 0.0;
		if (item.is_value == true)
		{
			val = JSONParser::get_value(weather_data, item.id);
			disp_string = JSONParser::round_value(val, 1);
		}
		else
			disp_string = JSONParser::get_string(weather_data, item.id);
		disp_string += item.unit;
		//First and last weather item has different location
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
