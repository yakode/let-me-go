#include <vector>
#include <iomanip>

#include "dev_blkmgr.h"
#include "parameter.h"

class Zone;
class ZoneBackend;

class Zone{
private:
	int id_;
	int64_t capacity_; // remaining capacity
	int64_t max_capacity_;
	int64_t wp_;
	int64_t used_capacity_; // sz of valid data
	BlockManager *blkmgr_;
public:
	Zone();
	Zone(int id, BlockManager *blkmgr);
	int Write(int data_size);
	int Read(); // I have not implenmented it yet
	int Reset();
	int Delete(long long data_size);
	bool IsFull();
	int GetId();
	int64_t GetCapacity();
	int64_t GetMaxCapacity();
	int64_t GetUsedCapacity();
	int64_t GetWP();
};

class ZoneBackend{
private:
	std::vector<Zone*> zones;
	BlockManager *blkmgr;
public:
	ZoneBackend();
	int AllocateIOZone(int lifetimehint, Zone **zone);
	Zone* GetZone(int zoneid){return zones[zoneid];}
	int64_t GetFreeSpace();
	int64_t GetUsedSpace();
	int64_t GetReclaimableSpace();
	int64_t GetGarbage();
	int GetECMin();
	int GetECMax();
	int GetECMinFree();
	int GetResetHint(int zoneid);
	void show(){
		blkmgr->show();
		std::cout << "Valid Data: " << GetUsedSpace() << " Bytes\n";
		std::cout << "Garbage Data: " << GetGarbage() << " Bytes\n";
	/*	
		std::cout << "    |Zone Garbage |" << "Valid Data   |" << "Capacity\n";
		std::cout << "----|-------------|" << "-------------|" << "-------------\n";
		for(int i = 0; i < NRZONE; ++i){
			std::cout << std::setw(4) << i << "|"
				<< std::setw(13) << zones[i]->GetWP() - zones[i]->GetUsedCapacity() << "|"
				<< std::setw(13) << zones[i]->GetUsedCapacity() << "|" 
				<< std::setw(13) << zones[i]->GetCapacity() << "\n";
		}
	*/	
	}
};
