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

	while(ifs >> time >> cmd >> addr >> data_size){
		std::cout << "-----Time:" << time << ", Command:" << cmd << ", Address:" << addr%SZFS << ", Size:" << data_size/2 << "-----\n";
		if(GC_ENABLE && gc_counter == 0)
			test.GarbageCollection();

		int  ret = test.Write(addr%SZFS, data_size/2);
		if(ret == -1)
			break;

		gc_counter = (gc_counter + 1) % GC_INTERVAL;
	}
	test.show();
	return 0;
}
