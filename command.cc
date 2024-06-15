#include "command.h"

WaitTime::WaitTime(int n){
	this->n = n;
	Te = new int64_t[n];
	Tc = new double[n];
	for(int i = 0; i < n; ++i){
		Te[i] = 0;
		Tc[i] = 0;
	}
	head = 0;
	total = 0;
}

WaitTime::~WaitTime(){
	delete []Tc;
	delete []Te;
}

int64_t  WaitTime::GetWaitTime(){
	int tail = (head + n - 1) % n;
	return total;
}

void WaitTime::PutWaitTime(int64_t wt, double time){
	total -= Te[head];
	total += wt;
	Te[head] = wt;
	Tc[head] = time;
	head = (head + 1) % n;
}

void WaitTime::Reset(){
	head = 0;
	total = 0;
	for(int i = 0; i < n; ++i){
		Te[i] = 0;
		Tc[i] = 0;
	}
}

void WaitTime::show(){
	for(int i = 0; i < n; ++i){
		std::cout << Te[(head + i) % n] << " ";
	}
	std::cout << "\n";
}
