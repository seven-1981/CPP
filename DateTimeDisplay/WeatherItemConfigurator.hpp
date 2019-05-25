#ifndef _WEATHERITEMCONFIGURATOR_H
#define _WEATHERITEMCONFIGURATOR_H

#include <map>
#include <string>

#include "FCWindowLabel.hpp"

//Title for window
const std::string TITLE_DISP = "* PI - HOLE *";
//Description string for date and time
const std::string DESC_TIME = "ZEIT: ";
const std::string DESC_DATE = "DATUM: ";

//Constants for time/date string extraction
const int DATE_START = 0;
const int DATE_LENGTH = 10;
const int TIME_START = 11;
const int TIME_LENGTH = 5;

//Constants for location of text
const int TITLE_X = 240; 
const int TITLE_Y = 50;
const int X_MARGIN = 50; 
const int X_MARGIN_STATION = 350;
const int X_MARGIN_SECOND_ROW = 600; 
const int Y_MARGIN = 100; 
const int Y_LINE_OFFSET = 43;

//Weather item holding data for parsing and display
//info whether it is a value or a string
struct WeatherItem
{
	const std::string id;		//Openweathermap identifier
	const std::string title;	//Title to diplay on screen
	const std::string unit;		//Unit for measurement value
	bool is_value;				//TRUE = is measurement value
};

class WeatherItemConfigurator
{
public:
	WeatherItemConfigurator();
	~WeatherItemConfigurator();
	
	void prepare_time_data(FCWindowLabelData_t&, FCWindowLabelDataItem_t&);
	void prepare_weather_data(std::string&, FCWindowLabelData_t&, FCWindowLabelDataItem_t&);
	
private:
	//Map for weather data identifiers
	std::map<unsigned int, WeatherItem> weather_items;
	
	//Fill map with desired items to display
	void init_map();
	//Create timestamp and date
	void create_timestamp(std::string&, std::string&);
};

#endif
