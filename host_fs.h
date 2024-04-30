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
	ZoneExtent(Zone *z, int s, int l): zone_(z), sector_(s), length_(l){}
	~ZoneExtent();
	ZoneExtent *GetNext();
	int GetSector();
	int GetLength();
	int GetZone();
};

class ZoneExtentList{
private:
	ZoneExtent *head_;
public:
	ZoneExtentList(): head_(nullptr){}
	int Push(Zone *zone, int addr, int length);
	ZoneExtent* GetHead();
	void show(){
		ZoneExtent *ptr = head_;
		while(ptr != nullptr){
			std::cout << ptr->sector_ << " " << ptr->length_ << "\n";
			ptr = ptr->next_;
		}
	}
};

class ZoneFile{
private:
	ZoneExtentList *extents;
	Zone *active_zone_;
	int AllocateNewZone();

	ZoneBackend *zbd_;

public:
	ZoneFile(ZoneBackend *zbd);
	int Write(int addr, int data_size);
	int Read();
	ZoneExtentList* GetZoneExtentList();
	void show(){extents->show();}
};

class SimpleFS{
private:
	std::vector<ZoneFile*> files;
	ZoneBackend *zbd;
public:
	SimpleFS();
	void GarbageCollection();
	void FBLRefreshment();
	void ResetBeforeWP();
	// 找到要寫的 ZoneFile call ZoneFile::Write
	int Write(int addr, int data_size);
	void show(){
		std::cout << "----------------------Show Zone Information\n";
		zbd->show();
		std::cout << "-----------------------End Zone Information\n";
	}
};