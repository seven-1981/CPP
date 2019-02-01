#ifndef _BPM_PARAM_H
#define _BPM_PARAM_H

#include <string>
#include <vector>
#include <limits>
#include <typeinfo>

//Basic parameter data type
//All parameters are inherited from this type
typedef enum { TypeUNKNOWN, TypeBOOL, TypeINT, TypeDOUBLE } Type;

class Param
{
public:
	Param(const std::string& name) : name(name) { }
	virtual ~Param() { }
	std::string get_name() { return this->name; }
	Type get_type() { return this->type; }
private:
	std::string name;
protected:
	Type type;
};

template <typename T>
class TypedParam : public Param
{
public:
	TypedParam(const std::string& name, const T& data, const T& min = std::numeric_limits<T>::min(), const T& max = std::numeric_limits<T>::max()) 
	: Param(name), data(data), max(max), min(min) { check(); }
	T get() { return this->data; }
	T get_min() { return this->min; }
	T get_max() { return this->max; }
	void set(T val) { this->data = val; check(); }
	void set_min(T val) { this->min = val; check(); }
	void set_max(T val) { this->max = val; check(); }

private:
	T data;
	T min, max;
	void check()
	{
		if (this->data < this->min)
			this->data = this->min;
		if (this->data > this->max)
			this->data = this->max;

		this->type = TypeUNKNOWN;
		if (typeid(this->data) == typeid(bool))
			this->type = TypeBOOL;
		if (typeid(this->data) == typeid(int))
			this->type = TypeINT;
		if (typeid(this->data) == typeid(double))
			this->type = TypeDOUBLE;
	}
};

//Parameter list handler
//This is basically a simple container for various data types
//Usage: add() adds elements to the parameter list. Lookup is done by
//the name property. Note that data type must be supplied by user.
//get() retrieves a parameter value indexed by its name
//set() sets a parameter value indexed by its name
//If name is not found or data type is wrong, an empty value is returned.

//Forward declaration of init function
void init_param();

class ParamList
{
public:
	ParamList() 
	{
		//Create parameters
		//Debug output
		add(new TypedParam<bool>("debug main", true));
		add(new TypedParam<bool>("debug gpio", false));
		add(new TypedParam<bool>("debug writer", false));
		add(new TypedParam<bool>("debug listener", false));
		add(new TypedParam<bool>("debug button", true));
		add(new TypedParam<bool>("debug queue", false));
		add(new TypedParam<bool>("debug state", false));
		add(new TypedParam<bool>("debug machine", false));
		add(new TypedParam<bool>("debug functions", true));
		add(new TypedParam<bool>("debug audio", true));
		add(new TypedParam<bool>("debug wavfile", false));
		add(new TypedParam<bool>("debug analyze", true));
		add(new TypedParam<bool>("debug biquad", false));
		//FC Queue
		add(new TypedParam<int>("queue size", 32, 16, 256));
		add(new TypedParam<int>("queue wait time", 100, 0, 1000));
		//Split Console
		add(new TypedParam<bool>("split logging", 0, 0, 1));
		add(new TypedParam<int>("split errors", 4, 0, 4));
		add(new TypedParam<int>("split fc", 3, 0, 4));
		add(new TypedParam<int>("split audio", 2, 0, 4));
		add(new TypedParam<int>("split main", 1, 0, 4));
		add(new TypedParam<int>("split input", 0, 0, 4));
		//Timing of segment display
		add(new TypedParam<int>("hold addr seg", 400, 100, 5000));
		add(new TypedParam<int>("pause addr seg", 50, 10, 5000));
		add(new TypedParam<int>("hold seg addr", 400, 100, 5000));
		add(new TypedParam<int>("pause seg addr", 50, 10, 5000));
		//Debug output file creation
		add(new TypedParam<bool>("create wavfiles", false));
		add(new TypedParam<bool>("create autocorr files", false));
		add(new TypedParam<bool>("create peak data", false));
		//Audio analysis
		add(new TypedParam<int>("algorithm", 1, 0, 1));
		add(new TypedParam<double>("lo freq", 20.0, 20.0, 200.0));
		add(new TypedParam<double>("hi freq", 150.0, 100.0, 300.0));
		add(new TypedParam<double>("bpm min", 100.0, 100.0, 120.0));
		add(new TypedParam<double>("bpm max", 200.0, 160.0, 240.0));
		add(new TypedParam<double>("env filt rec", 0.005, 0.001, 0.05));
		add(new TypedParam<double>("peak width", 200.0, 20.0, 1000.0));
		add(new TypedParam<double>("peak threshold", 0.3, 0.01, 0.9));
		add(new TypedParam<double>("peak adjacence", 60.0, 5.0, 200.0));
		add(new TypedParam<double>("rms threshold", 1200.0, 0.0, 32767.0));
		//Functions
		add(new TypedParam<int>("man cycle", 500, 100, 2000));
	}

	~ParamList()
	{
		std::vector<Param*>::iterator iter;
		for (iter = this->list.begin(); iter < this->list.end(); ++iter)
			delete (*iter);
		this->list.clear();
	}

	void add(Param* p) { this->list.push_back(p); }
	
	Type get_type(const std::string& name)
	{
		Type retval = TypeUNKNOWN;
		std::vector<Param*>::iterator iter, end;
		for (iter = this->list.begin(), end = this->list.end(); iter < end; ++iter)
		{
			if (name == (*iter)->get_name())
			{
				retval = (*iter)->get_type();
			}
		}
		return retval;
	}

	bool valid(const std::string& name)
	{
		bool success = false;
		std::vector<Param*>::iterator iter, end;
		for (iter = this->list.begin(), end = this->list.end(); iter < end; ++iter)
		{
			if (name == (*iter)->get_name())
			{
				success = true;
			}
		}
		return success;
	}

	template <typename T>
	T get(const std::string& name)
	{
		T retval { };
		std::vector<Param*>::iterator iter, end;
		for (iter = this->list.begin(), end = this->list.end(); iter < end; ++iter)
		{
			if (name == (*iter)->get_name())
			{
				retval = ((TypedParam<T>*)(*iter))->get();
			}
		}
		return retval;
	}

	template <typename T>
	bool set(const std::string& name, T val)
	{
		bool success = false;
		std::vector<Param*>::iterator iter, end;
		for (iter = this->list.begin(), end = this->list.end(); iter < end; ++iter)
		{
			if (name == (*iter)->get_name())
			{
				((TypedParam<T>*)(*iter))->set(val);
				success = true;
			}
		}
		return success;
	}

	//Some explanation for type cast:
	//To access iterator, it must be dereferenced -> *iter (access underlying pointer)
	//This underlying pointer is of type Param* and must be cast 
	//from base to derived type -> (TypedParam<T>*)
	
private:
	std::vector<Param*> list;
};

#endif