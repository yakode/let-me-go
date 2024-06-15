#include "command.h"
#include "host_fs.h"
#include <iostream>
#include <string>
#include <fstream>

int main(int argc, char *argv[]){
	SimpleFS test;
	WaitTime wt(16);
	test.FillUp();
	test.show();

	double time;
	std::string cmd;
	int64_t addr, data_size;
	
	int64_t s = 0; // micro second
	int64_t latency_w_us = 0, latency_r_us = 0; // micro second
	type_latency latency_w_s = 0, latency_r_s = 0; // second
	type_latency latency_gc_s = 0;
	int64_t latency_gc_us = 0;
	double counter_gc = 10;

	int64_t nr_w = 0, nr_r = 0; // amount of commands

	std::cout << "Input Files:\n";
	for(int i = 1; i < argc; i += 2){
		std::string in_id = argv[i];
		std::cout << "\t" << "ssdtrace-" + in_id << "\n";
		int N = std::stoi(argv[i + 1]) + 1;
		std::string prefix = "./ssdtrace-"+ in_id + "/ssdtrace-" + in_id + "-";
		for(int j = 0; j < N; ++j){
			std::string input = prefix + std::string(5 - std::to_string(j).length(), '0') + std::to_string(j) + ".in";
			std::ifstream ifs(input, std::ios::in);
			if(!ifs.is_open()){
				std::cout << "failed to open " << input << "\n";
				return -1;
			}
			wt.Reset();
	
			while(ifs >> time >> cmd >> addr >> data_size){
				int64_t addr_ = addr  % SZFS;
				int64_t data_size_ = data_size;
				if(addr_ + data_size_ > SZFS)
					data_size_ = SZFS - addr_;
		
				if(SHOW_CMD){
					std::cout << std::setprecision(9) << "-----Time:" << std::setw(10) << time; 
					std::cout << ", Command:" << cmd 
						<< ", Address:" << std::setw(10) << addr_ 
						<< ", Size:" << std::setw(5) << data_size_ << "-----\n";
				}
		
				// Garbage Collection
				if(counter_gc <= 0){
					counter_gc = 10;
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
					latency_gc_us = s;
					if(SHOW_CMD) std::cout << "GC " << (double)((double)s / (double)1000000) << " sec\n";
				}
		
				// Command Handle
				if(cmd == "WS"){
					s = test.Write(addr_, data_size_);
					if(s <= -1)
						break;
	
					wt.PutWaitTime(s + latency_gc_us, time);
					latency_w_us += wt.GetWaitTime();
					latency_w_s += latency_w_us / 1000000;
					latency_w_us %= 1000000;
	
					nr_w++;
				}else if(cmd == "RS"){
					s = test.Read(addr_, data_size_);
	
					if(s == -1){
						s = 0;
						continue;
					}
	
					wt.PutWaitTime(s + latency_gc_us, time);
					latency_r_us += wt.GetWaitTime();
					latency_r_s += latency_r_us / 1000000;
					latency_r_us %= 1000000;
	
					nr_r++;
				}
				latency_gc_us = 0;
				counter_gc -= (double)((double)s / (double)1000000);
			}
			ifs.close();
		}
		test.Flush();
	}

	test.show();
	std::cout << "Average Write Latency: " << (double)latency_w_s/nr_w << "s\n"; // and " << latency_w << "us\n";
	std::cout << "Average Read Latency:  " << (double)latency_r_s/nr_r << "s\n"; // and " << latency_r << "us\n";
	std::cout << nr_w << " write commands, and " << nr_r << " read commands\n";
	return 0;
}
