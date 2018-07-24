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
	}

	buffer<T>(long size, long sample_rate)
	{
		init_buffer(size, sample_rate);
	}

	~buffer<T>()
	{
		free(this->values);
	}

	buffer<T>(const buffer<T>& rhs)
	{
		init_buffer(rhs.size, rhs.sample_rate);
		for (long i = 0; i < rhs.size; i++)
			this->values[i] = rhs.values[i];
	}

	buffer<T>& operator=(const buffer<T>& rhs)
	{
		init_buffer(rhs.size, rhs.sample_rate);
		return *this;
	}

	long get_size() const { return this->size; }
	long get_sample_rate() const { return this->sample_rate; }

	void init_buffer(long size, long sample_rate)
	{
		this->values = (T*)malloc(size * sizeof(T));
		this->size = size;
		this->sample_rate = sample_rate;
	}

	T* values;

private:
	long size;
	long sample_rate;
};

#endif
