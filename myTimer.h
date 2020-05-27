#ifndef _myTimer_H_
#define _myTimer_H

#include <chrono>
using namespace std::chrono;

class myTimer{
public:
	myTimer() {
		update();
	}
	~myTimer() {}
	void update()
	{
		_begin = high_resolution_clock::now();
	}
	double getSecond()
	{
		return this->getMicrosecond()*0.000001;
	}
	double getMillisecond()
	{
		return this->getMicrosecond()*0.001;
	}
	long long getMicrosecond()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
protected:
	time_point<high_resolution_clock> _begin;
};

#endif // !_myTimer_H_

