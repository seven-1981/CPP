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
	WeatherCollector(const std::string& url) : m_url(url)
	{
		m_curl = curl_easy_init();
		if (m_curl)
		{
			init();
			m_initialized = true;
		}
		else
			m_initialized = false;
	}
	
	~WeatherCollector() { curl_easy_cleanup(m_curl); }
	
	bool request(std::string& data)
	{
		if (m_initialized == false)
			return false;
		
		init();
		if (curl_easy_perform(m_curl) == CURLE_OK)
		{
			data = m_readBuffer;
			return true;
		}
		else
			return false;
	}
	
private:
	std::string m_url;
	CURL* m_curl;
	std::string m_readBuffer;
	bool m_initialized;
	
	void init()
	{
		m_readBuffer.clear();
		curl_easy_reset(m_curl);
		curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_readBuffer);
	}	
};

#endif
