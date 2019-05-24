#ifndef _URLMODIFIER_H
#define _URLMODIFIER_H

#include <string>
#include <random>

//Constants for coordinate selection
const double LAT_MIN = 46.731;
const double LAT_MAX = 47.580;
const double LON_MIN = 7.0400;
const double LON_MAX = 9.4111;


class URLModifier
{
public:
	URLModifier()
	{
		url_sample = "https://samples.openweathermap.org/data/2.5/weather?lat=47.319542&lon=8.051965&appid=b6907d289e10d714a6e88b30761fae22";
		url_req = "https://api.openweathermap.org/data/2.5/weather?lat=47.319542&lon=8.051965&appid=889a052df8efc1c1c0235058b557c1c9&lang=de";
	}
	~URLModifier() { }

	void create_weather_url(bool own_location, std::string& url_string)
	{
		std::string retval = url_req;
		if (own_location == false)
		{
			//Put some random location in url request
			random_url_pos(retval);
		}
		else
		{
			//Leave original url request
		}
		url_string = retval;
	}

private:
	//URL for http weather request - sample
	std::string url_sample;
	//URL for http weather request - real data
	std::string url_req;
	
	//Uniform distribution for coordinates
	std::uniform_real_distribution<double> dis_lat;
	std::uniform_real_distribution<double> dis_lon;	
	
	//Returns a random double between LAT/LON MIN/MAX
	double get_random(bool lat)
	{
		//Random generator seed
		std::random_device rd;
		std::mt19937 gen(rd());
		using pick = std::uniform_real_distribution<double>::param_type;
		if (lat == true)
			return (double)dis_lat(gen, pick(LAT_MIN, LAT_MAX));
		else
			return (double)dis_lon(gen, pick(LON_MIN, LON_MAX));
	}
	
	//Replace home location with random location
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
};

#endif
