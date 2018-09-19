#include <iostream>

//Interface which describes any kind of data.
struct IData
{
	virtual ~IData()
	{
	}
};

struct Intdata : public IData
{
	int x;
	Intdata(int _x)
	{
		std::cout << "Creating Intdata, x = " << _x << std::endl;
		x = _x;
	}
};

struct Doubledata : public IData
{
	double x;
	Doubledata(double _x)
	{
		std::cout << "Creating Doubledata, x = " << _x << std::endl;
		x = _x;
	}
};

template <typename RET, typename PAR>
struct FPTR
{
	typedef RET (*fptype)(PAR);
};

typedef FPTR<IData*, IData*>::fptype FPIDATA;
typedef FPTR<Intdata*, Intdata*>::fptype FPINT;

//Interface which desribes any kind of operation.
struct IOperation
{
	FPIDATA Fptr;

	//actual operation which will be performed
	virtual IData* Execute(IData *_pData) = 0;

	virtual ~IOperation()
	{
	}
};

struct EventInt : public IOperation
{
	EventInt(FPINT _fp)
	{
		std::cout << "Creating Event..." << std::endl;
		Fptr = reinterpret_cast<FPIDATA>(_fp);
	}

	IData* Execute(IData *_pData)
	{
		Intdata* pData = dynamic_cast<Intdata*>(_pData);
		std::cout << "Executing Base EventInt." << std::endl;
		return Fptr(pData);
	}

	/*Intdata* Execute(Intdata* _pData)
	{
		IData* pData = dynamic_cast<IData*>(_pData);
		std::cout << "Executing Derived EventInt." << std::endl;
		return dynamic_cast<Intdata*>(Execute(pData));
	}*/
};

Intdata* intfunc(Intdata* _x)
{
	_x->x = _x->x + _x->x;
	return _x;
}

Doubledata* doublefunc(Doubledata* _x)
{
	_x->x = _x->x + _x->x;
	return _x;
}

int main()
{
	std::cout << "Start Interface Test..." << std::endl;

	Intdata* pa = new Intdata(5);
	Intdata* pr = new Intdata(0);
	FPINT fp = intfunc;
	EventInt x(fp);
	pr = dynamic_cast<Intdata*>(x.Execute(pa));
	std::cout << "Result: " << pr->x << std::endl;

	IData* pi = new Intdata(77);
	IData* pj = new Intdata(0);
	EventInt y(fp);
	pj = x.Execute(pi);
	std::cout << "Result: " << dynamic_cast<Intdata*>(pj)->x << std::endl;

	Doubledata* pk = new Doubledata(55.5);
	Doubledata* pu = new Doubledata(11.2);
	

	std::cout << "End Interface Test." << std::endl;
	
	getchar();
	return 0;
}
