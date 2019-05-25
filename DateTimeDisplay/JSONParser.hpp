#ifndef _JSONPARSER_H
#define _JSONPARSER_H

#include <string>

#include "WeatherItems.hpp"

class JSONParser
{
public:
	static double get_value(std::string& data, const std::string& identifier)
	{
		double result = 0.0;
		std::string temp_string = data;
		if (JSONParser::parse(temp_string, identifier) == false)
			return 0.0;
		else
		{
			if (JSONParser::valid_number(temp_string) == true)
			{
				std::size_t temp_len = ITEM_TEMP.length();
				if (identifier.substr(0, temp_len) == ITEM_TEMP)
					return JSONParser::to_celsius(std::stod(temp_string));
				else
					return std::stod(temp_string);
			}
			else
				return 0.0;
		}
	}
	
	static std::string get_string(std::string& data, const std::string& identifier)
	{
		std::string temp_string = data;
		if (JSONParser::parse(temp_string, identifier) == false)
			return temp_string;
		if (JSONParser::remove_quotes(temp_string) == false)
			return data;
		JSONParser::replace_umlaute(temp_string);
		return temp_string;
	}
	
	static double to_celsius(double kelvin)
	{
		return kelvin - 273.15;
	}
	
	static std::string round_value(double value, std::size_t num_dec)
	{
		std::string num_string = std::to_string(value);
		std::size_t pos_dot = num_string.find(".");
		if (pos_dot == std::string::npos)
			return num_string;
		std::size_t len = num_string.length();
		std::string num1 = num_string.substr(0, pos_dot + 1);
		std::size_t decimals = num_string.substr(pos_dot + 1, len - pos_dot - 1).length();
		std::size_t decimals_to_apply = std::min(decimals, num_dec);
		return num1 + num_string.substr(pos_dot + 1, decimals_to_apply);
	}
	
private:
	static bool parse(std::string& data, const std::string& identifier)
	{
		std::size_t offset = identifier.length() + 2;
		std::size_t found_id = data.find(identifier);
		if (found_id == std::string::npos)
			return false;
		std::size_t found_comma = data.find(",", found_id + 1);
		std::size_t found_bracket = data.find("}", found_id + 1);
		if (found_comma == std::string::npos && found_bracket == std::string::npos)
			return false;
		std::size_t found_min = std::min(found_comma, found_bracket);
		data = data.substr(found_id + offset, found_min - found_id - offset);
		return true;
	}
	
	static bool valid_number(std::string& number_str)
	{
		if (number_str.length() == 0)
			return false;
		bool valid = true;
		for (char c : number_str)
		{
			if ((c < '0' || c > '9') && c != '.')
				valid = false;
		}
		return valid;
	}
	
	static bool remove_quotes(std::string& data)
	{
		std::string temp_string = data;
		std::size_t pos_quote1 = temp_string.find("\"");
		if (pos_quote1 != 0)
			return false;
		temp_string = temp_string.substr(1, temp_string.length() - 1);
		if (temp_string.substr(temp_string.length() - 1, 1) != "\"")
			return false;
		data = temp_string.substr(0, temp_string.length() - 1);
		return true;
	}
	
	static void replace_umlaute(std::string& data)
	{
		std::string result { };
		bool umlaut = false;
		for (char c : data)
		{
			if (c == uml)
			{
				//Umlaut detected, set flag
				umlaut = true;
				continue;
			}
			if (umlaut == true)
			{
				//Previous character was umlaut detection
				//now detect type and replace it
				if (c == AE)
					result.push_back('A');
				else if (c == OE)
					result.push_back('O');
				else if (c == UE)
					result.push_back('U');
				else if (c == ae)
					result.push_back('a');
				else if (c == oe)
					result.push_back('o');
				else if (c == ue)
					result.push_back('u');
				else { //Discard character
				}
				result.push_back('e');
				umlaut = false;
			}
			else
				result.push_back(c);
		}
		data = result;
	}
};

#endif
