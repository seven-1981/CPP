#include <thread>
#include <chrono>
#include <string>

//Allowing C-header file to be compiled with C++
extern "C" {
#include <xdo.h>
}

#include "FCWindow.hpp"
#include "FCWindowManager.hpp"
#include "FCWindowLabel.hpp"
#include "FCWindowGraph.hpp"

#include "JSONParser.hpp"
#include "WeatherCollector.hpp"
#include "WeatherItemConfigurator.hpp"
#include "WeatherItems.hpp"
#include "URLModifier.hpp"

//Constants for displaying text
const std::string TITLE = "RASPI - PI HOLE";
const int SIZE_X_INIT = 500;
const int SIZE_Y_INIT = 250;
//Graph window data/axis size
const int SIZE_GRAPH_DATA = 500;
//Cycle time wait period used in main
const int SECONDS_WAIT = 30;

//Constants for alt-tab simulation
const int KEY_WAIT_US = 5000;
const char* KEY_ALT_TAB = "Alt_L+Tab";


void simulate_alt_tab()
{
	xdo_t* x = xdo_new(NULL);
	xdo_send_keysequence_window(x, CURRENTWINDOW, KEY_ALT_TAB, KEY_WAIT_US);
	xdo_free(x);
}

int main(int argc, char **argv)

{
	//Initialize weather collector and map
	WeatherCollector collector;
	std::string weather_data { };
	unsigned int main_cycle = 0;

	//Initialize window manager and start window 1 - text display

  	FCWindowManager::init(argc, argv);
	FCWindowParam_t win_param { SIZE_X_INIT, SIZE_Y_INIT, TITLE, true };
	FCWindow* window1 = FCWindowManager::create(FCWindowType_e::TypeWindowLabel, win_param);
	win_param.size = SIZE_GRAPH_DATA; // + 1 for proper scaling tick length
	FCWindow* window2 = FCWindowManager::create(FCWindowType_e::TypeWindowGraph, win_param);
	//Initialize window 2 - graph display
	FCWindowGraphSize_t graph_size;
	graph_size.max_x_value = SIZE_GRAPH_DATA;
	window2->set_param(&graph_size);
	
	//Start must be called after first window has been created
	FCWindowManager::start();
	
	//Create weather item configurator
	WeatherItemConfigurator configurator;
	//Create URL modifier
	URLModifier modifier;
	
	//Temperature graph
	FCWindowGraphData_t graph_data;
	graph_data.values.push_back(0);
	graph_data.values.push_back(0);
	graph_data.values.push_back(0);
	
	while (window1->get_quit() == false)
	{		
		//Create weather data url request
		std::string url_to_request { };
		modifier.create_weather_url(main_cycle % 2 == 0, url_to_request);
		collector.update_url(url_to_request);
		bool successful = collector.request(weather_data);
		//std::cout << weather_data << std::endl;
		
		//Create display data
		FCWindowLabelData_t display_data;
		//Create text item data for window
		FCWindowLabelDataItem_t element;
		//Configure date/time display elements
		configurator.prepare_time_data(display_data, element);
		//Continue with weather data, if request was successful
		if (successful == true)
		{
			//Try to parse weather data
			configurator.prepare_weather_data(weather_data, display_data, element);
			
			//Add temperature to graph
			if (main_cycle % 2 == 0)
			{
				double temperature = JSONParser::get_value(weather_data, ITEM_TEMP);
				double temperature_max = JSONParser::get_value(weather_data, ITEM_TMAX);
				double temperature_min = JSONParser::get_value(weather_data, ITEM_TMIN);
				graph_data.values.at(0) = temperature;
				graph_data.values.at(1) = temperature_max;
				graph_data.values.at(2) = temperature_min;
				window2->update(&graph_data);
			}
		}
			
		//Update window data
		window1->update(&display_data);
		std::this_thread::sleep_for(std::chrono::seconds(SECONDS_WAIT / 2));
		simulate_alt_tab();
		main_cycle++;
		std::this_thread::sleep_for(std::chrono::seconds(SECONDS_WAIT / 2));
	}
	
  	std::this_thread::sleep_for(std::chrono::seconds(1));
	FCWindowManager::stop();
  	std::this_thread::sleep_for(std::chrono::seconds(1));
	delete window1;
	delete window2;

  	return 0;

}
