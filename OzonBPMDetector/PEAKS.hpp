#ifndef _PEAKS_H
#define _PEAKS_H

#include "buffer.hpp"
#include "DSP.hpp"
#include <vector>
#include <fstream>
#include <functional>

namespace PEAKS
{
	//Debug stream
	std::ofstream dbgOut;
	//Debug flag - can be set to output data
	static bool debug_active = false;
	static bool debug_file_open = false;
	
	void activate_debug()
	{
		debug_active = true;
		if (debug_file_open == false)
		{
			dbgOut.open("peak_data.txt", std::ios_base::ate | std::ios_base::app);
			debug_file_open = true;
		}
	}

	void deactivate_debug()
	{
		debug_active = false;
		debug_file_open = false;
		dbgOut.close();
	}
	
	struct peak
	{
		unsigned int i; //Array index (autocorr_array 1...n)
		unsigned int j; //Peak nbr inside autocorr_array
		explicit peak(unsigned int i, unsigned int j) //We allow only construction like this
		{
			this->i = i;
			this->j = j;
		};
		bool operator==(const peak& rhs) const //Comparison of two peaks
		{
			return ((this->i == rhs.i) && (this->j) == rhs.j);
		};
	};

	struct params
	{
		double bpm_min;
		double bpm_max;
		std::vector<double> widths;
		std::vector<double> thresholds;
		std::function<double(double, double)> weight;
		unsigned int adjacence;
		params() { }
		params(double bpm_min, double bpm_max,
		       std::vector<double>& widths, 
		       std::vector<double>& thresholds, 
		       std::function<double(double, double)>&& weight, 
		       unsigned int adjacence)
		{
			this->bpm_min = bpm_min;
			this->bpm_max = bpm_max;
			this->widths = widths;
			this->thresholds = thresholds;
			this->weight = weight;
			this->adjacence = adjacence;
		}
	};

	//Assume having a vector of indices with peaks already found -> vec
	//We want to know, if a certain new peak (peak) is already close to one of the found peaks in vec
	//So we check the proximity of the indices in vec for the indices defined by peak
	int find_index(std::vector<long>& vec, peak& peak, std::vector<std::vector<long>>& peak_indices, long width)
	{
		//Check size
		int index = -1;
		if (vec.size() == 0)
			return index;

		//Check index
		if (peak.i >= peak_indices.size())
			return index;
		if (peak.j >= peak_indices.at(peak.i).size())
			return index;

		//Go through already found peaks
		for (unsigned int i = 0; i < vec.size(); i++)
		{
			//If the index defined by 'peak' is close to an already found peak -> save the index and stop
			if (abs((peak_indices.at(peak.i).at(peak.j)) - (vec.at(i))) <= width)
			{
				index = (int)i;
			}
		}
		return index;
	}

	//Check if two peak's indices (peak1 and peak2) stored in peak_indices are close together
	//Proximity is defined by width
	bool close_together(peak& peak1, peak& peak2, std::vector<std::vector<long>>& peak_indices, long width)
	{
		//Check index
		if (peak1.i >= peak_indices.size() || peak2.i >= peak_indices.size())
			return false;
		if (peak1.j >= peak_indices.at(peak1.i).size() || peak2.j >= peak_indices.at(peak2.i).size())
			return false;

		if (abs((peak_indices.at(peak1.i).at(peak1.j)) - (peak_indices.at(peak2.i).at(peak2.j))) <= width)
			return true;
		else
			return false;
	}

	//Get peaks from a single input buffer
	template <typename T>
	unsigned int get_peaks(const buffer<T>& inbuffer, std::vector<long>& indices, std::vector<T>& values, unsigned int width, double threshold)
	{
		//Get size of input buffer
		long size = inbuffer.get_size();

		//Add width to end of buffer
		buffer<double> temp_buffer;
		temp_buffer.init_buffer(size + width, inbuffer.get_sample_rate());
		//Append zeros to end of buffer
		for (long i = 0; i < size + width; i++)
		{
			if (i < size)
				temp_buffer[i] = inbuffer[i];
			else
				temp_buffer[i] = 0;
		}

		std::vector<long> temp_ind;
		std::vector<T> temp_val;

		//Store local maximum
		T local_max = std::numeric_limits<T>::min();
		T global_max = std::numeric_limits<T>::min();
		long max_ind = 0;
		long counter = 0;

		//Scan through values
		for (long i = 0; i < size + width; i++)
		{
			//Check for new global maximum
			if (temp_buffer[i] > global_max)
			{
				global_max = temp_buffer[i];
			}

			//Check for new local maximum
			if (temp_buffer[i] > local_max)
			{
				//Store new local maximum
				local_max = temp_buffer[i];
				//Store index
				max_ind = i;
				//Reset counter
				counter = 0;
			}
			else
			{
				//Increase counter
				counter++;
				//Check if counter has reached width
				if (counter >= width)
				{
					//Reset counter
					counter = 0;
					//Store local maximum
					temp_val.push_back(local_max);
					//Store index
					temp_ind.push_back(max_ind);
					//Reset values
					local_max = std::numeric_limits<T>::min();
					max_ind = 0;
				}
			}
		}

		//Debug output
		//for (unsigned int i = 0; i < temp_val.size(); i++)
		//{
		//	std::cout << "Local Max: " << i << " - value " << temp_val.at(i) << " at pos " << temp_ind.at(i) << std::endl;
		//}

		//Check values - if below threshold, skip them
		for (unsigned int i = 0; i < temp_val.size(); i++)
		{
			if (temp_val.at(i) / (double)global_max >= threshold)
			{
				//Add value to result vector
				indices.push_back(temp_ind.at(i));
				values.push_back(temp_val.at(i));
			}
		}

		//Debug output
		if (debug_active == true)
		{
			for (unsigned int i = 0; i < values.size(); i++)
				dbgOut << "Local Max: " << i << " - value " << values.at(i) << " at pos " << indices.at(i) << std::endl;
		}

		//Return number of found peaks
		return values.size();
	}

	//Helper debug function - print vector contents
	template <typename T>
	void print_vec(std::string s, std::vector<T>& v)
	{
		dbgOut << s << " content: " << std::endl;
		for (unsigned int i = 0; i < v.size(); i++)
			dbgOut << i << ": " << v.at(i) << std::endl;
	}

	//For debug purposes, disable optimization
	//#pragma optimize("", off)  

	//Advanced 'PEAK' bpm extraction function
	double extract_bpm_value_advanced(std::vector<buffer<double>*>& autocorr_arrays, params& params)
	{
		//Array index for calculation of return value
		long array_index = 0;

		//Get size of autocorr array
		long size = autocorr_arrays.at(0)->get_size();

		//Number of arrays
		unsigned int nArrays = autocorr_arrays.size();

		//Perform weighting
		//Faster bpm values are more likely than slower ones
		for (unsigned int i = 0; i < nArrays; i++)
			DSP::apply_window(*autocorr_arrays.at(i), params.weight);

		//Peak index values - x axis, indices
		std::vector<std::vector<long>> peak_indices;
		//Maximum values - y axis
		std::vector<std::vector<double>> max_values;
		std::vector<double> global_max_values;
		//Number of peaks
		std::vector<unsigned int> num_peaks;

		//Calculate peaks and max values
		for (unsigned int i = 0; i < nArrays; i++)
		{
			//We have to push something in the vectors before use
			std::vector<long> init1; std::vector<double> init2;
			peak_indices.push_back(init1); max_values.push_back(init2);
			num_peaks.push_back(PEAKS::get_peaks(*autocorr_arrays.at(i), peak_indices.at(i), max_values.at(i), params.widths.at(i), params.thresholds.at(i)));
			//Find max conveniently using iterator - returning pointer to double
			global_max_values.push_back(*std::max_element(max_values.at(i).begin(), max_values.at(i).end()));
		}
		
		if (debug_active == true)
			print_vec("global_max_values", global_max_values);

		//Get confidence for each peak - the higher the better
		//Confidence is based on ratio max y value - peak value but it's decreased by number of peaks
		//The more peaks found in one array, the less weight has one peak
		//We also check how close indices are to their counterparts from other passbands
		//Adjacence is the amount of spreading along the x axis
		//The higher the better
		std::vector<double> confidence;
		std::vector<unsigned int> adjacence;
		std::vector<long> peaks;
		std::vector<double> average;

		//Create peak using outer for-loop (i,j)
		//Then forward compare with created inner peak (n, k)
		for (unsigned int i = 0; i < nArrays; i++)
		{
			for (unsigned int j = 0; j < num_peaks.at(i); j++)
			{
				peak peak_outer(i, j);
				//Check if (i,j) is in already in vector
				if (PEAKS::find_index(peaks, peak_outer, peak_indices, params.adjacence) == -1)
				{
					peaks.push_back(peak_indices.at(i).at(j));
					average.push_back(peak_indices.at(i).at(j));
					adjacence.push_back(0);
					confidence.push_back(max_values.at(i).at(j) / global_max_values.at(i) / num_peaks.at(i));
				}

				for (unsigned int n = i + 1; n < nArrays; n++)
				{
					for (unsigned int k = 0; k < num_peaks.at(n); k++)
					{
						peak peak_inner(n, k);
						if (PEAKS::close_together(peak_outer, peak_inner, peak_indices, params.adjacence) == true)
						{
							int found_index = PEAKS::find_index(peaks, peak_outer, peak_indices, params.adjacence);
							if (found_index != -1)
							{
								adjacence.at(found_index)++;
								average.at(found_index) *= adjacence.at(found_index);
								average.at(found_index) += peak_indices.at(n).at(k);
								average.at(found_index) /= (adjacence.at(found_index) + 1);
							}
						}
						//std::cout << "i" << i << " j" << j << " n" << n << " k" << k << std::endl;
					}
				}
			}
		}

		if (debug_active == true)
		{
			dbgOut << "Results:" << std::endl;
			dbgOut << "Num = " << peaks.size() << std::endl;
			print_vec("peaks", peaks);
			print_vec("confidence", confidence);
			print_vec("adjacence", adjacence);
			print_vec("average", average);
		}

		//Now what we have to do basically is to merge the 2 vectors
		//and get the max value adjacence * confidence
		double max_conf_adj = 0;
		unsigned int ind = 0;
		for (unsigned int i = 0; i < confidence.size(); i++)
		{
			double conf_adj = confidence.at(i) * ((double)adjacence.at(i));
			if (conf_adj > max_conf_adj)
			{
				max_conf_adj = conf_adj;
				ind = i;
			}
		}

		//Extract the index
		array_index = average.at(ind);
		if (debug_active == true)
			dbgOut << "Selected index: " << average.at(ind) << "\n";

		double min_lag = (double)60 / params.bpm_max;
		double max_lag = (double)60 / params.bpm_min;

		//Return the calculated bpm value from the max index
		double bpm_lag = min_lag + (max_lag - min_lag) / size * array_index;

		if (debug_active == true)
			dbgOut << "CALCULATED BPM VALUE: " << 60.0 / bpm_lag << "\n\n";

		return 60.0 / bpm_lag;
	}

	//#pragma optimize("", off)  

	//More pragmatic algorithm, this one just selects the array with the least peaks
	//and determines the maximum
	double extract_bpm_value(std::vector<buffer<double>*>& autocorr_arrays, params& params)
	{
		//Debug information
		static int count = 0;
		if (debug_active == true)
			dbgOut << "Measurement: " << count++ << "\n";

		//Array index for calculation of return value
		long array_index = 0;

		//Get size of autocorr array
		long size = autocorr_arrays.at(0)->get_size();

		//Number of arrays
		unsigned int nArrays = autocorr_arrays.size();

		//Perform weighting
		//Faster bpm values are more likely than slower ones
		for (unsigned int i = 0; i < nArrays; i++)
			DSP::apply_window(*autocorr_arrays.at(i), params.weight);

		//Peak index values - x axis, indices
		std::vector<std::vector<long>> peak_indices;
		//Maximum values - y axis
		std::vector<std::vector<double>> max_values;
		std::vector<double> global_max_values;
		//Number of peaks
		std::vector<unsigned int> num_peaks;

		//Calculate peaks and max values
		for (unsigned int i = 0; i < nArrays; i++)
		{
			//We have to push something in the vectors before use
			std::vector<long> init1; std::vector<double> init2;
			peak_indices.push_back(init1); max_values.push_back(init2);
			num_peaks.push_back(PEAKS::get_peaks(*autocorr_arrays.at(i), peak_indices.at(i), max_values.at(i), params.widths.at(i), params.thresholds.at(i)));
			//Find max conveniently using iterator - returning pointer to double
			global_max_values.push_back(*std::max_element(max_values.at(i).begin(), max_values.at(i).end()));
		}

		if (debug_active == true)
			print_vec("global_max_values", global_max_values);

		//Get array with least peaks
		unsigned int selection = 0;
		unsigned int value = std::numeric_limits<unsigned int>::max();
		for (unsigned int i = 0; i < nArrays; i++)
		{
			if (num_peaks.at(i) < value)
			{
				value = num_peaks.at(i);
				selection = i;
			}
		}
		
		if (debug_active == true)
			dbgOut << "Selection: " << selection << "\n";

		//Find index with global maximum
		for (unsigned int i = 0; i < num_peaks.at(selection); i ++)
		{
			if (max_values.at(selection).at(i) == global_max_values.at(selection))
				array_index = peak_indices.at(selection).at(i);
		}

		if (debug_active == true)
			dbgOut << "Selected array index: " << array_index << "\n";

		double min_lag = (double)60 / params.bpm_max;
		double max_lag = (double)60 / params.bpm_min;

		//Return the calculated bpm value from the max index
		double bpm_lag = min_lag + (max_lag - min_lag) / size * array_index;

		if (debug_active == true)
			dbgOut << "CALCULATED BPM VALUE: " << 60.0 / bpm_lag << "\n\n";

		return 60.0 / bpm_lag;
	}

}

#endif