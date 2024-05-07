#include "host_fs.h"
#include <iostream>
#include <string>
#include <fstream>

int main(int argc, char *argv[]){
	std::string input = argv[1];
	std::ifstream ifs(input, std::ios::in);
	if(!ifs.is_open()){
		std::cout << "failed to open " << input << "\n";
		return -1;
	}

	SimpleFS test;
	test.show();
	double time;
	std::string cmd;
	int addr, data_size;
	int gc_counter = 1;
	int s = 0;

	while(ifs >> time >> cmd >> addr >> data_size){
		int addr_ = addr % SZFS;
		int data_size_ = data_size;
		if(addr_ + data_size_ > SZFS)
			data_size_ = SZFS - addr_;

		std::cout << "-----Time:" << time << ", Command:" << cmd << ", Address:" << addr_ << ", Size:" << data_size_ << "-----\n";
		if(GC_ENABLE && gc_counter == 0){
			s = test.GarbageCollection();
			if(s == -1)
				break;
		}

		s = test.Write(addr_, data_size_);
		if(s == -1)
			break;

		gc_counter = (gc_counter + 1) % GC_INTERVAL;
	}
	test.show();
	test.check();
	return 0;
}
