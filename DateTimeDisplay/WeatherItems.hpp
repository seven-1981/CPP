#ifndef _WEATHERITEMS_H
#define _WEATHERITEMS_H

#include <string>

//JSON openweathermap identifiers
const std::string ITEM_DESC = "description";
const std::string ITEM_TEMP = "temp";
const std::string ITEM_TMIN = "temp_min";
const std::string ITEM_TMAX = "temp_max";
const std::string ITEM_PRES = "pressure";
const std::string ITEM_HUMI = "humidity";
const std::string ITEM_WSPD = "speed";
const std::string ITEM_WDIR = "deg";
const std::string ITEM_CLDN = "all";
const std::string ITEM_STAT = "name";

//Descriptions for weather items
const std::string DESC_DESC = "";
const std::string DESC_TEMP = "TEMPERATUR:";
const std::string DESC_TMIN = "TEMP. MIN:";
const std::string DESC_TMAX = "TEMP. MAX:";
const std::string DESC_PRES = "LUFTDRUCK:";
const std::string DESC_HUMI = "FEUCHTE:";
const std::string DESC_WSPD = "WINDSTAERKE:";
const std::string DESC_WDIR = "WINDRICHTUNG:";
const std::string DESC_CLDN = "WOLKENDECKE:";
const std::string DESC_STAT = "STATION:";

//Units for weather measurements
const std::string UNIT_DESC = "";
const std::string UNIT_TEMP = " degC";
const std::string UNIT_TMIN = " degC";
const std::string UNIT_TMAX = " degC";
const std::string UNIT_PRES = " hPa";
const std::string UNIT_HUMI = " %";
const std::string UNIT_WSPD = " m/s";
const std::string UNIT_WDIR = " deg";
const std::string UNIT_CLDN = " %";
const std::string UNIT_STAT = "";

//Weater item identifiers
enum WeatherItem_e
{
	ItemDescription,
	ItemTemperature,
	ItemTemperatureMin,
	ItemTemperatureMax,
	ItemPressure,
	ItemHumidity,
	ItemWindSpeed,
	ItemWindDirection,
	ItemCloudiness,
	ItemStation,
	NUM_WEATHER_ITEMS
};

//Umlaut start code
const char uml = 195;
//German umlaut codes - probably platform dependent
const char AE = 132; const char OE = 150; const char UE = 156;
const char ae = 164; const char oe = 182; const char ue = 188;

#endif
