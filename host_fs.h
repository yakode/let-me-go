#include <vector>
#include <set>
#include <algorithm>
#include <iostream>

#include "host_zbd.h"
#include "parameter.h"

class ZoneExtent;
class ZoneExtentList;
class ZoneFile;
class SimpleFS;

class ZoneExtent{
	friend class ZoneExtentList;
private:
	Zone *zone_;
	int sector_;
	int length_;
	ZoneExtent *next_;
public:
	ZoneExtent(Zone *z, int s, int l, ZoneExtent *n): zone_(z), sector_(s), length_(l), next_(n){}
	ZoneExtent(Zone *z, int s, int l): zone_(z), sector_(s), length_(l), next_(nullptr){}
	~ZoneExtent();
	Zone* GetZone();
	int GetSector();
	int GetLength();
	ZoneExtent *GetNext();
	void SetLength(int length);
	void SetZone(Zone* zone);
};

class ZoneExtentList{
private:
	ZoneExtent *head_;
public:
	ZoneExtentList(): head_(nullptr){}
	int Push(Zone *zone, int addr, int length);
	ZoneExtent* GetHead();
	/*void show(){
		ZoneExtent *ptr = head_;
		while(ptr != nullptr){
			if(ptr->zone_ != nullptr)
				std::cout << "zone: " << ptr->zone_->GetId() << ", ";
			std::cout << "addr: " << ptr->sector_ << ", length: " << ptr->length_ << "\n";
			ptr = ptr->next_;
		}
	}*/
};

class ZoneFile{
private:
	ZoneExtentList *extents;
	Zone *active_zone_;
	ZoneBackend *zbd_;
	int buffered;

	int AllocateNewZone();
public:
	ZoneFile(ZoneBackend *zbd);
	int Write(int addr, int data_size, bool *sthDeleted);
	int FlushBuffer(bool *sthDeleted);
	int Read();
	ZoneExtentList* GetZoneExtentList();
	// void show(){extents->show();}
};

class SimpleFS{
private:
	std::vector<ZoneFile*> files;
	ZoneBackend *zbd;
public:
	SimpleFS();
	int GarbageCollection();
	int FBLRefreshment();
	void ResetBeforeWP();
	int Write(int addr, int data_size);
	int Read(int addr, int data_size){return -1;}
	void check();
	void show(){
		std::cout << "----------------------Show Zone Information\n";
		zbd->show();
		std::cout << "-----------------------End Zone Information\n";
	}
};
