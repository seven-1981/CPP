#ifndef _BPMVALUE
#define _BPMVALUE

#include <cmath>

//Defines for display bpm value behaviour
#define BPMVALUE_ARRAY_SIZE 10
#define THRES_CHANGE_LOW 3.0
#define THRES_CHANGE_HIGH 20.0
#define MAX_BAD_VALUES 3

//Class for handling the BPM value
//Determines the behaviour, if the BPM value changes
class BPMValue
{
public:
	BPMValue()
	{
		init_array(0.0);
		this->first_value = true;
		this->bad_values = 0;
	}

	~BPMValue() { }

	//Method that puts in a new value, makes some calculation
	//and returns the improved bpm value
	double process_value(double value)
	{
		//First, we check if it is the first measurement
		if (this->first_value == true)
		{
			//Yes, it is the first measurement, fill the array
			init_array(value);
			this->first_value = false;
			return value;
		}
		else
		{
			//There were some measurements before
			//We decide upon the new value:
			//1. Value is close to the mean of the other values
			//   -> Just put the new value into the array and return it
			//2. The value has changed slightly
			//   -> Re-init the values to the new value and return it
			//3. The value has changed drastically. Was it the first time? 
			//   -> a. If yes, memorize it and ignore the value
			//   -> b. If no (2-3 values bad), execute 2.

			//Get mean and check against new value
			double average = mean();
			double diff = fabs(value - average);

			//Case 3.
			if (diff >= THRES_CHANGE_HIGH)
			{
				if (this->bad_values < MAX_BAD_VALUES)
				{
					//First time, 
					this->bad_values++;
					//return this->values[0];
					return mean();
				}
				else
				{
					//Max. bad values reached
					init_array(value);
					this->bad_values = 0;
					return value;
				}
			}
			//Case 2.
			else if (diff >= THRES_CHANGE_LOW)
			{
				init_array(value);
				this->bad_values = 0;
				return value;
			}
			//Case 1.
			else
			{
				shift();
				this->values[0] = value;
				this->bad_values = 0;
				//return value;
				return mean();
			}
		}
	}

private:
	//Array for holding the last measured values
	double values[BPMVALUE_ARRAY_SIZE];

	//First measurement?
	bool first_value;

	//Number of 'bad' values i a row
	unsigned int bad_values;

	//Initialize array with a value
	void init_array(double value)
	{
		for (int i = 0; i < BPMVALUE_ARRAY_SIZE; i++)
		{
			this->values[i] = value;
		}
	}

	//Shift all the values
	void shift()
	{
		for (int i = BPMVALUE_ARRAY_SIZE - 1; i > 0; i--)
		{
			this->values[i] = this->values[i-1];
		}
	}

	//Get mean value
	double mean()
	{
		double average = 0.0;
		for (int i = 0; i < BPMVALUE_ARRAY_SIZE; i++)
		{
			average += this->values[i] / BPMVALUE_ARRAY_SIZE;
		}
		return average;
	}
};

#endif
