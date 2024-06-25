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
	double counter_fbl = 3;

	int64_t nr_w = 0, nr_r = 0; // amount of commands

	std::string inputfiles[5] = {
		"../input/ssdtrace-00.in", "../input/ssdtrace-01.in", "../input/ssdtrace-02.in", "../input/ssdtrace-03.in"
	};
	std::string systorfiles[30] = {
		/*"../systor/2016022207-LUN0.in",*/ "../systor/2016022214-LUN0.in", "../systor/2016022307-LUN0.in", 
		"../systor/2016022314-LUN0.in", "../systor/2016022208-LUN0.in", "../systor/2016022215-LUN0.in",
		"../systor/2016022308-LUN0.in", "../systor/2016022315-LUN0.in", "../systor/2016022209-LUN0.in",
		"../systor/2016022216-LUN0.in", "../systor/2016022309-LUN0.in", "../systor/2016022316-LUN0.in",
		"../systor/2016022210-LUN0.in", "../systor/2016022217-LUN0.in", "../systor/2016022310-LUN0.in",
		"../systor/2016022317-LUN0.in", "../systor/2016022211-LUN0.in", "../systor/2016022218-LUN0.in",
	};/*	"../systor/2016022311-LUN0.in", "../systor/2016022318-LUN0.in", "../systor/2016022212-LUN0.in",
		"../systor/2016022219-LUN0.in", "../systor/2016022312-LUN0.in", "../systor/2016022319-LUN0.in",
		"../systor/2016022213-LUN0.in", "../systor/2016022220-LUN0.in", "../systor/2016022313-LUN0.in"
	};*/
	int N = 4;
	if(!TRACE) N = 6;
	int T = 4;
	if(!TRACE) T = 14;
	bool err = false;
	std::cout << "Input Files:\n";
	while(T--){
		if(err) break;
	//std::cout << T << "\n";
		for(int i = 0; i < N; ++i){
			if(err) break;
			//if(T == 0 && i == 1) break;
			std::string input = inputfiles[i];
			if(!TRACE) input = systorfiles[i];
			//std::cout << "\t" << input << "\n";
			std::ifstream ifs(input, std::ios::in);
			if(!ifs.is_open()){
				std::cout << "failed to open " << input << "\n";
				return -1;
			}
			wt.Reset();
	
			while(ifs >> time >> cmd >> addr >> data_size){
				int64_t addr_ = addr  % SZFS;
				int64_t data_size_ = data_size;
				if(!TRACE) data_size_ *= 16;
				if(addr_ + data_size_ > SZFS)
					data_size_ = SZFS - addr_;
		
				if(SHOW_CMD){
					std::cout << std::setprecision(9) << "-----Time:" << std::setw(10) << time; 
					std::cout << ", Command:" << cmd 
						<< ", Address:" << std::setw(10) << addr_ 
						<< ", Size:" << std::setw(5) << data_size_ << "-----\n";
				}
		
				if(counter_fbl <= 0){
					counter_fbl = 3;
					s = 0;
					if(ENABLE_FBL_REFRESH){
						s = test.FBLRefreshment();
						if(s == -1){
							err = true;
							break;
						}
					}
					latency_gc_us += s;
				}
				// Garbage Collection
				if(counter_gc <= 0){
					counter_gc = 10;
					s = 0;
					if(GC_ENABLE){
						s = test.GarbageCollection();
						if(s == -1){
							err = true;
							break;
						}
					}
					latency_gc_us = s;
				}
		
				// Command Handle
				if(cmd == "WS" || cmd == "W"){
					s = test.Write(addr_, data_size_);
					if(s <= -1){
						err = true;
						break;
					}
	
					wt.PutWaitTime(s + latency_gc_us, time);
					latency_w_us += wt.GetWaitTime();
					latency_w_s += latency_w_us / 1000000;
					latency_w_us %= 1000000;
	
					nr_w++;
				}else if(cmd == "RS" || cmd == "R"){
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
				counter_fbl -= (double)((double)s / (double)1000000);
			}
			// std::cout << nr_w << " write commands, and " << nr_r << " read commands\n";
			ifs.close();
			test.Flush();
		}
		std::cout << T << "\n";
	}

	test.show();
	std::cout << "Average Write Latency: " << (double)latency_w_s/nr_w << "s\n"; // and " << latency_w << "us\n";
	std::cout << "Average Read Latency:  " << (double)latency_r_s/nr_r << "s\n"; // and " << latency_r << "us\n";
	std::cout << nr_w << " write commands, and " << nr_r << " read commands\n";
	return 0;
}
