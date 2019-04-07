#ifndef _TIMING_H
#define _TIMING_H

#include <chrono>

using namespace std::chrono;

class Timing
{
public:
	Timing()
	{
		init();
	}
	~Timing() { }

	void init()
	{
		reference = high_resolution_clock::now();
	}

	long long get_total_time_us()
	{
		time_point<high_resolution_clock> yet;
		yet = high_resolution_clock::now();
		long long us = duration_cast<microseconds>(yet - this->reference).count();
		return us;	
	}

	long long get_time_us()
	{
		time_point<high_resolution_clock> yet;
		yet = high_resolution_clock::now();
		long long us = duration_cast<microseconds>(yet - this->reference).count();
		this->reference = yet;
		return us;
	}

private:
	time_point<high_resolution_clock> reference;
};

#endif
