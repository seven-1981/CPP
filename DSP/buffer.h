#ifndef _BUFFER_H
#define _BUFFER_H

#include <cstdlib>
#include <cassert>
#include <string>
#include <fstream>

template <typename T>
class buffer
{
public:
	buffer<T>()
	{
		this->values = nullptr;
		this->size = 0;
		this->sample_rate = 0;
		this->initialized = false;
	}

	buffer<T>(long size, long sample_rate)
	{
		init_buffer(size, sample_rate);
		this->initialized = true;
	}

	~buffer<T>()
	{
		free(this->values);
	}

	buffer<T>(const buffer<T>& rhs)
	{
		init_buffer(rhs.size, rhs.sample_rate);
		this->initialized = true;
		for (long i = 0; i < rhs.size; i++)
			this->values[i] = rhs.values[i];
	}

	buffer<T>& operator=(const buffer<T>& rhs)
	{
		init_buffer(rhs.size, rhs.sample_rate);
		this->initialized = true;
		return *this;
	}

	const T& operator[](long index) const
	{
		assert(this->initialized == true && index < this->size);
		return this->values[index];
	}

	T& operator[](long index)
	{
		assert(this->initialized == true && index < this->size);
		return this->values[index];
	}

	long get_size() const { return this->size; }
	long get_sample_rate() const { return this->sample_rate; }
	bool is_initialized() const { return this->initialized; }

	void init_buffer(long size, long sample_rate)
	{
		if (this->initialized == false)
		{
			this->values = (T*)malloc(size * sizeof(T));
			this->initialized = true;
			this->size = size;
			this->sample_rate = sample_rate;
		}
	}

	void print(std::string filename)
	{
		std::ofstream file(filename, std::ios_base::out);
		for (long i = 0; i < this->size; i++)
			file << i << "; " << this->values[i] << "\n";
		file.close();
	}

private:
	T* values;
	long size;
	long sample_rate;

	bool initialized;
};

#endif
