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
	int64_t sector_;
	int64_t length_;
	ZoneExtent *next_;
public:
	ZoneExtent(Zone *z, int64_t s, int64_t l, ZoneExtent *n): zone_(z), sector_(s), length_(l), next_(n){}
	ZoneExtent(Zone *z, int64_t s, int64_t l): zone_(z), sector_(s), length_(l), next_(nullptr){}
	~ZoneExtent();
	Zone* GetZone();
	int64_t GetSector();
	int64_t GetLength();
	ZoneExtent *GetNext();
	void SetLength(int64_t length);
	void SetZone(Zone* zone);
};

class ZoneExtentList{
private:
	ZoneExtent *head_;
public:
	ZoneExtentList(): head_(nullptr){}
	int Push(Zone *zone, int64_t addr, int64_t length);
	ZoneExtent* GetHead();
};

class ZoneFile{
private:
	ZoneExtentList *extents;
	Zone *active_zone_;
	ZoneBackend *zbd_;
	int64_t buffered;

	int AllocateNewZone();
public:
	ZoneFile(ZoneBackend *zbd);
	type_latency Write(int64_t addr, int64_t data_size, bool *sthDeleted);
	type_latency FlushBuffer(bool *sthDeleted);
	type_latency Read(int64_t addr, int64_t data_size, std::set<int>*);
	ZoneExtentList* GetZoneExtentList();
	void DummyActiveZone();
};

class SimpleFS{
private:
	std::vector<ZoneFile*> files;
	ZoneBackend *zbd;
	int nrGC, nrRWP, nrRFBL;
	int64_t nrMigrate; // Sector
	bool flagGCWL;
public:
	SimpleFS();
	type_latency GarbageCollection();
	type_latency FBLRefreshment();
	type_latency ResetBeforeWP();
	type_latency Write(int64_t addr, int64_t data_size);
	type_latency Read(int64_t addr, int64_t data_size);
	void FillUp();
	void Flush();
	void check();
	void show(){
		std::cout << "----------------------Hey Zone Information----------------------\n";
		if(ENABLE_DYNAMIC_MAPPING)
			std::cout << "Mapping Method:                     Dynamic\n";
		else
			std::cout << "Mapping Method:                     Static\n";
		std::cout << "Enable Garbage Collection:          " << GC_ENABLE << "\n";
		std::cout << "Enable Garbage Collection with WL:  " << ENABLE_GC_WL << "\n";
		std::cout << "Enable Free Block List Refreshment: " << ENABLE_FBL_REFRESH << "\n";
		std::cout << "-\n";
		std::cout << "Storage Size:    " << std::setw(20) << (long long)NRBLK * (long long)SZBLK / 1024 << " K Sectors\n";
		std::cout << "File System Size:" << std::setw(20) << SZFS/1024 << " K Sectors\n";
		std::cout << "-\n";
		zbd->show();
		std::cout << "Garbage Collection:           " << std::setw(10) << nrGC << " times\n";
		std::cout << "Migrate:                      " << std::setw(10) << nrMigrate << " sectors\n";
		std::cout << "Reset If Invalid Before WP:   " << std::setw(10) << nrRWP << " times\n";
		std::cout << "Free Block List Refreshment:: " << std::setw(10) << nrRFBL << " times\n";
		std::cout << flagGCWL << "\n";
		std::cout << "-----------------------End Zone Information----------------------\n";
	}
};
