#include <iostream>

//Interface which describes any kind of data.
struct IData
{
	virtual ~IData()
	{
	}
};

template <typename T>
struct Data : public IData
{
	T x;
	Data(T _x)
	{
		std::cout << "Creating Data, x = " << _x << std::endl;
		x = _x;
	}
};

template <typename RET, typename PAR>
struct FPTR
{
	typedef RET (*fptype)(PAR);
};

template <typename RET, typename PAR>
struct FPDATA
{
	typedef typename FPTR<Data<RET>*, Data<PAR>*>::fptype fptype;
};

//Interface which desribes any kind of operation.
template <typename RET, typename PAR>
struct IOperation
{
	typename FPDATA<RET, PAR>::fptype Fptr;

	//actual operation which will be performed
	virtual IData* Execute(IData *_pData) = 0;

	virtual ~IOperation()
	{
	}
};

template <typename RET, typename PAR>
struct Event : public IOperation<RET, PAR>
{
	Event(typename FPDATA<RET, PAR>::fptype _fp)
	{
		std::cout << "Creating Event..." << std::endl;
		IOperation<RET,PAR>::Fptr = reinterpret_cast<typename FPDATA<RET, PAR>::fptype>(_fp);
	}

	IData* Execute(IData *_pData)
	{
		Data<PAR>* pData = dynamic_cast<Data<PAR>*>(_pData);
		std::cout << "Executing Base EventInt." << std::endl;
		return IOperation<RET,PAR>::Fptr(pData);
	}

	/*Intdata* Execute(Intdata* _pData)
	{
		IData* pData = dynamic_cast<IData*>(_pData);
		std::cout << "Executing Derived EventInt." << std::endl;
		return dynamic_cast<Intdata*>(Execute(pData));
	}*/
};

Data<int>* intfunc(Data<int>* _x)
{
	_x->x = _x->x + _x->x;
	return _x;
}

Data<double>* doublefunc(Data<double>* _x)
{
	_x->x = _x->x + _x->x;
	return _x;
}

int main()
{
	std::cout << "Start Interface Test..." << std::endl;

	Data<int>* pa = new Data<int>(5);
	Data<int>* pr = new Data<int>(0);
	FPDATA<int, int>::fptype fp = intfunc;
	Event<int, int> x(fp);
	pr = dynamic_cast<Data<int>*>(x.Execute(pa));
	std::cout << "Result: " << pr->x << std::endl;

	IData* pi = new Data<double>(77);
	IData* pj = new Data<double>(0);
	FPDATA<double, double>::fptype fp2 = doublefunc;
	Event<double, double> y(fp2);
	pj = y.Execute(pi);
	std::cout << "Result: " << dynamic_cast<Data<double>*>(pj)->x << std::endl;
	
	std::cout << "End Interface Test." << std::endl;
	
	getchar();
	return 0;
}
