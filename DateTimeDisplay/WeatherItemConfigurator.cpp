#include <string>
#include <chrono>
#include <ctime>

#include "WeatherItemConfigurator.hpp"
#include "JSONParser.hpp"
#include "WeatherItems.hpp"


WeatherItemConfigurator::WeatherItemConfigurator()
{
	init_map();
}

WeatherItemConfigurator::~WeatherItemConfigurator()
{
	
}

void WeatherItemConfigurator::init_map()
{
	WeatherItem weather_cond { ITEM_DESC, DESC_DESC, UNIT_DESC, false };
	WeatherItem temperature  { ITEM_TEMP, DESC_TEMP, UNIT_TEMP, true };
	WeatherItem temp_min     { ITEM_TMIN, DESC_TMIN, UNIT_TMIN, true };
	WeatherItem temp_max     { ITEM_TMAX, DESC_TMAX, UNIT_TMAX, true };
	WeatherItem pressure     { ITEM_PRES, DESC_PRES, UNIT_PRES, true };
	WeatherItem humidity     { ITEM_HUMI, DESC_HUMI, UNIT_HUMI, true };
	WeatherItem wind_speed   { ITEM_WSPD, DESC_WSPD, UNIT_WSPD, true };
	WeatherItem wind_dir     { ITEM_WDIR, DESC_WDIR, UNIT_WDIR, true };
	WeatherItem cloudiness   { ITEM_CLDN, DESC_CLDN, UNIT_CLDN, true };
	WeatherItem station      { ITEM_STAT, DESC_STAT, UNIT_STAT, false };
	weather_items.emplace(ItemDescription, 		weather_cond);
	weather_items.emplace(ItemTemperature, 		temperature);
	weather_items.emplace(ItemTemperatureMin, 	temp_min);
	weather_items.emplace(ItemTemperatureMax, 	temp_max);
	weather_items.emplace(ItemPressure, 		pressure);
	weather_items.emplace(ItemHumidity, 		humidity);
	weather_items.emplace(ItemWindSpeed, 		wind_speed);
	weather_items.emplace(ItemWindDirection, 	wind_dir);
	weather_items.emplace(ItemCloudiness, 		cloudiness);
	weather_items.emplace(ItemStation, 			station);
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
	date = DESC_DATE + date;
	time = DESC_TIME + time;	
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
		if (i == (int)ItemDescription)
			element.x = X_MARGIN;
		else if (i == (int)ItemStation)
			element.x = X_MARGIN_STATION;
		else
			element.x = X_MARGIN_SECOND_ROW;
		element.text = disp_string;
		display_data.items.push_back(element);
	}
}
