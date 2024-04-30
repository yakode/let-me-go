// ZonedBlockDevice::ResetUnusedIOZones
// wp 前沒資料就 reset
// trigger: deletion
// ZenFS::GCWorker
// trigger: every 10 sec

#include <vector>

#include "dev_blkmgr.h"
#include "parameter.h"

class Zone;
class ZoneBackend;

class Zone{
private:
	int id_;
	int capacity_; // remaining capacity
	int max_capacity_;
	int wp_;
	int used_capacity_; // sz of valid data
	BlockManager *blkmgr_;
public:
	Zone();
	Zone(int id, BlockManager *blkmgr);
	int Write(int data_size);
	int Read();
	int Reset();
	int Delete(int data_size);
	bool IsFull();
	int GetCapacity();
	int GetId(){return id_;}
	int GetUsedCapacity();
	int GetMaxCapacity();
	int GetWP();
};

class ZoneBackend{
private:
	std::vector<Zone*> zones;
	BlockManager *blkmgr;
public:
	ZoneBackend();
	int AllocateIOZone(int lifetimehint, Zone **zone);
	Zone* GetZone(int zoneid){return zones[zoneid];}
	int GetFreeSpace();
	int GetUsedSpace();
	int GetReclaimableSpace();
	void show(){
		blkmgr->show();
		std::cout << "Zone Garbage\\Valid Data\\Capacity:\n";
		for(int i = 0; i < NRZONE; ++i){
			std::cout << zones[i]->GetWP() - zones[i]->GetUsedCapacity() << "\\"
			<< zones[i]->GetUsedCapacity() << "\\" 
			<< zones[i]->GetCapacity() << "\n";
		}
	}
};
