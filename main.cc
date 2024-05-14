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
	int64_t latency_w = 0, latency_r = 0;

	while(ifs >> time >> cmd >> addr >> data_size){
		if(SHOW_CMD){
			std::cout << std::setprecision(9) << "-----Time:" << time; 
			std::cout << ", Command:" << cmd 
				<< ", Address:" << addr_ 
				<< ", Size:" << data_size_ << "-----\n";
		}

		int addr_ = addr % SZFS;
		int data_size_ = data_size;
		if(addr_ + data_size_ > SZFS)
			data_size_ = SZFS - addr_;

		if(gc_counter == 0){
			s = 1;
			if(GC_ENABLE){
				s = test.GarbageCollection();
				if(s == -1)
					break;
			}
			if(s == 1 && ENABLE_FBL_REFRESH){
				s = test.FBLRefreshment();
				if(s == -1)
					break;
			}
		}
		gc_counter = (gc_counter + 1) % GC_INTERVAL;

		if(cmd == "WS"){
			s = test.Write(addr_, data_size_);
			latency_w += (int64_t)s;
		}else if(cmd == "RS"){
			s = test.Read(addr_, data_size_);
			latency_r += (int64_t)s;
		}
		if(s == -1)
			break;
	}

	test.show();
	test.check();
	return 0;
}
