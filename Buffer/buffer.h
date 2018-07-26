#ifndef _BUFFER_H
#define _BUFFER_H

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
		if (this->initialized == true)
			return this->values[index];
		else
			return this->cnull;
	}

	T& operator[](long index)
	{
		if (this->initialized == true && index < this->size)
			return this->values[index];
		else
			return this->null;
	}

	long get_size() const { return this->size; }
	long get_sample_rate() const { return this->sample_rate; }

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

private:
	T* values;
	long size;
	long sample_rate;

	bool initialized;
	T null = 0;
	const T cnull = 0;
};

#endif
