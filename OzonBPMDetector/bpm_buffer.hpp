#ifndef _BPM_BUFFER
#define _BPM_BUFFER

#include <iostream>
#include "bpm_globals.hpp"
#include "SplitConsole.hpp"

//Buffer class - holds a number of buffers of any type
//Used to buffer recorded samples from the bpm_audio class
//Provides buffers for sample storage, each buffer has the same size N
template <typename T>
class BPMBuffer
{
public:
	//Constructor and destructor
	BPMBuffer(int size) 
	{ 
		this->size = size;
		for (int i = 0; i < NUM_OF_BUFFERS; i++)
		{
			this->buffers[i] = (T*)malloc(this->size * sizeof(T));
			this->lock = false;
			this->read = 0;
			this->read_temp = 0;
			this->write = 0;
			this->full = false;
			this->empty = true;
		}
	}

	~BPMBuffer()
	{
		for (int i = 0; i < NUM_OF_BUFFERS; i++)
		{
			free(this->buffers[i]);
		}
	}
	
	//Methods for buffer access
	eError put_data(T* data);
	T* get_data();

	//Check proper initialisation
	eError check_init();

	//Check if buffer is full - returns eSuccess if not full
	eError check_full();

	//Check if buffer is ready (e.g. there is some data to extract)
	//Returns eSuccess if ready
	eError check_ready();

	//Getter methods
	int read_index() { return this->read; }
	int write_index() { return this->write; }

private:
	//Default constructor private, must pass size if instantiated
	BPMBuffer();

	//Size of each buffer
	long size;

	//Buffer flags
	int read;
	int read_temp;
	int write;
	bool full;
	bool empty;

	//Buffers
	T* buffers[NUM_OF_BUFFERS];

	//Buffer protection with bool
	bool lock;
};

template <typename T>
eError BPMBuffer<T>::put_data(T* data)
{
	//We check if buffer was initialized properly
	eError init = check_init();
	if (init != eSuccess)
		return init;
	
	//We check if the buffer is full
	if (this->full == true)
		return eBuffer_Overflow;

	//We don't check if the buffer is locked. The buffer is not full, so it must
	//be either empty or partially full.

	//Determine if we have to lock the buffer.
	//We only lock the buffer, if read and write index are the same.
	//It should not be possible to read and write the same buffer at the same time.
	//However, it should be possible to read and write different buffers at the same time.
	if (this->read == this->write)
		this->lock = true;
	
	//Put the data into the next buffer
	for (long i = 0; i < this->size; i++)
		this->buffers[this->write][i] = data[i];

	//As soon as we have put something inside, the buffer is not empty any more
	this->empty = false;

	//Increase index and wrap
	this->write++;
	if (this->write >= NUM_OF_BUFFERS)
		this->write = 0;

	//Check if buffer is full
	if (this->read == this->write)
		this->full = true;

	//After we have finished, update the flags
	if (this->lock == true)
		this->lock = false;
	
	return eSuccess;		
}

template <typename T>
T* BPMBuffer<T>::get_data()
{	
	//We check if buffer was initialized properly
	eError init = check_init();
	if (init != eSuccess)
		return nullptr;

	//We check if the buffer is empty
	if (this->empty == true)
		return nullptr;

	//Determine if the buffer is already locked
	//If yes - we have to wait until it gets unlocked
	while (this->lock == true)
		//Wait

	//This time, it's not necessary to lock the buffer.
	//If the read and write index were matching, the buffer would be full.
	//and the put_data method can't be executed. And it can't be empty either.
	//Therefore it's safe to read the data.

	//Get actual index
	this->read_temp = this->read;

	//Increase index and wrap
	this->read++;
	if (this->read >= NUM_OF_BUFFERS)
		this->read = 0;

	//If we have flushed once, the buffer is not full any more
	this->full = false;

	//Check if buffer is empty
	if (this->read == this->write)
		this->empty = true;
	
	//Return buffer
	return this->buffers[this->read_temp];
}

template <typename T>
eError BPMBuffer<T>::check_init()
{
	//Check if buffers were properly initialized
	eError retval = eSuccess;
	
	for (int i = 0; i < NUM_OF_BUFFERS; i++)
	{
		if (this->buffers[i] == nullptr)
			retval = eBuffer_NotInitialized;
	}

	return retval;
}
	
template <typename T>
eError BPMBuffer<T>::check_full()
{
	if (this->full == true)
		return eBuffer_IsFull;
	else
		return eSuccess;
}

template <typename T>
eError BPMBuffer<T>::check_ready()
{
	if (this->write == this->read && this->full == false)
		return eBuffer_NotReady;
	else
		return eSuccess;
}

#endif
