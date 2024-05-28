#include "host_fs.h"
#include "command.h"
#include <iostream>
#include <string>
#include <fstream>

int main(int argc, char *argv[]){
	SimpleFS test;
	WaitTime wt(16);

	double time;
	std::string cmd;
	int addr, data_size;
	
	int s = 0;
	int64_t latency_w_us = 0, latency_r_us = 0; // micro second
	int64_t latency_w_s = 0, latency_r_s = 0; // second

	int64_t nr_w = 0, nr_r = 0; // amount of commands

	std::cout << "Input Files:\n";
	for(int i = 1; i < argc; ++i){
		std::string input = argv[i];
		std::cout << "\t" << input << "\n";
		std::ifstream ifs(input, std::ios::in);
		if(!ifs.is_open()){
			std::cout << "failed to open " << input << "\n";
			return -1;
		}

		while(ifs >> time >> cmd >> addr >> data_size){
			int addr_ = addr % SZFS;
			int data_size_ = data_size;
			if(addr_ + data_size_ > SZFS)
				data_size_ = SZFS - addr_;
	
			if(SHOW_CMD){
				std::cout << std::setprecision(9) << "-----Time:" << std::setw(10) << time; 
				std::cout << ", Command:" << cmd 
					<< ", Address:" << std::setw(10) << addr_ 
					<< ", Size:" << std::setw(5) << data_size_ << "-----\n";
			}
	
			// Garbage Collection
			/*
			if(current - time_last_gc >= 10){
				time_last_gc = current;
				s = 0;
				if(GC_ENABLE){
					s = test.GarbageCollection();
					if(s == -1)
						break;
				}
				if(s == 0 && ENABLE_FBL_REFRESH){
					s = test.FBLRefreshment();
					if(s == -1)
						break;
				}
				current += (double)((double)s / (double)US);
				if(SHOW_CMD) std::cout << "GC " << (double)((double)s / (double)US) << " sec\n";
			}
			*/
	
			if(cmd == "WS"){
				s = test.Write(addr_, data_size_);

				wt.PutWaitTime(s, time);
				latency_w_us += wt.GetWaitTime();
				latency_w_s += latency_w_us / 1000000;
				latency_w_us %= 1000000;

				nr_w++;

				if(SHOW_CMD) std::cout << "W: " << (double)((double)s / (double)US) << " sec";
			}else if(cmd == "RS"){
				s = test.Read(addr_, data_size_);

				if(s == -1){
					s = 0;
					continue;
				}

				wt.PutWaitTime(s, time);
				latency_r_us += wt.GetWaitTime();
				latency_r_s += latency_r_us / 1000000;
				latency_r_us %= 1000000;

				nr_r++;

				if(SHOW_CMD) std::cout << "R: " << (double)((double)s / (double)US) << " sec";
			}
			if(s == -1)
				break;
		}
		ifs.close();
	}

	test.show();
	std::cout << "Average Write Latency: " << (double)latency_w_s/nr_w << "s\n"; // and " << latency_w << "us\n";
	std::cout << "Average Read Latency:  " << (double)latency_r_s/nr_r << "s\n"; // and " << latency_r << "us\n";
	std::cout << nr_w << " write commands, and " << nr_r << " read commands\n";
	// test.check();
	return 0;
}
