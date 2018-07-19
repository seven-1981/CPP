#ifndef _BUFFER_H
#define _BUFFER_H

template <typename T>
class buffer
{
public:
	buffer<T>(long size, long sample_rate)
	{
		this->values = (T*)malloc(size * sizeof(T));
		this->size = size;
		this->sample_rate = sample_rate;
	}

	~buffer<T>()
	{
		free(this->values);
	}

	buffer<T>(const buffer<T>& rhs)
	{
		this->values = (T*)malloc(rhs.size * sizeof(T));
		this->size = rhs.size;
		this->sample_rate = rhs.sample_rate;
		for (long i = 0; i < rhs.size; i++)
			this->values[i] = rhs.values[i];
	}

	buffer<T>& operator=(const buffer<T>& rhs)
	{
		this->size = rhs.size;
		this->sample_rate = rhs.sample_rate;
		for (long i = 0; i < rhs.size; i++)
			this->values[i] = rhs.values[i];
		return *this;
	}

	T* values;
	long size;
	long sample_rate;
};

#endif
