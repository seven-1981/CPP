#ifndef _WEATHERCOLLECTOR_H
#define _WEATHERCOLLECTOR_H

#include <curl/curl.h>
#include <string>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

class WeatherCollector
{
public:
	WeatherCollector() : 
	  m_url(""), m_initialized(false)
	{
		//Default constructor
	}

	WeatherCollector(const std::string& url) : 
	  m_url(url), m_initialized(false)
	{
		init();
	}
	
	~WeatherCollector() { curl_easy_cleanup(m_curl); }
	
	bool request(std::string& data)
	{
		if (m_initialized == false)
			return false;
		
		reset();
		if (curl_easy_perform(m_curl) == CURLE_OK)
		{
			data = m_readBuffer;
			return true;
		}
		else
			return false;
	}
	
	void update_url(std::string& url)
	{
		m_url = url;
		init();
	}
	
private:
	std::string m_url;
	CURL* m_curl;
	std::string m_readBuffer;
	bool m_initialized;
	
	void reset()
	{
		m_readBuffer.clear();
		curl_easy_reset(m_curl);
		curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_readBuffer);
	}	
	
	void init()
	{
		if (m_initialized == false)
		{
			m_curl = curl_easy_init();
		}
		if (m_curl)
		{
			reset();
			m_initialized = true;
		}
		else
			m_initialized = false;		
	}
};

#endif
