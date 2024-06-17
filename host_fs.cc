#include "host_fs.h"

ZoneExtent::~ZoneExtent(){
	if(zone_ != nullptr)
		zone_->Delete(length_);
}

Zone* ZoneExtent::GetZone(){
	return zone_;
}

int64_t ZoneExtent::GetSector(){
	return sector_;
}

int64_t ZoneExtent::GetLength(){
	return length_;
}

ZoneExtent* ZoneExtent::GetNext(){
	return next_;
}

void ZoneExtent::SetLength(int64_t length){
	length_ = length;
}

void ZoneExtent::SetZone(Zone *zone){
	zone_ = zone;
}

int ZoneExtentList::Push(Zone *zone, int64_t addr, int64_t length){
	if(SHOW_ZONEFILE){
		if(zone)
			std::cout << "Push New Extent(addr: "<< addr << ", size: " << length << ") to Zone[" << zone->GetId() << "]\n";
		else
			std::cout << "Push New Extent(addr: " << addr << ", size: " << length << ") to Buffer\n";
	}

	int flag = 0, s = 0;
	// 0: ok; -1: failed; 1: deletion
	if(head_ == nullptr){
		head_ = new ZoneExtent(zone, addr, length, nullptr);
		return 0;
	}
	
	ZoneExtent *pre = nullptr;
	ZoneExtent *ptr = head_;
	// delete overlapping
	while(ptr != nullptr){
		// 把重疊的舊資料刪除
		int64_t A = ptr->sector_;
		int64_t B = ptr->sector_ + ptr->length_;
		int64_t C = addr;
		int64_t D = addr + length;
		// A------B
		//    C++++++D
		// [A, B) [C, D)
		if(A >= D)
			break;
		if(A < C and B > C){
			// -------
			//   +++
			// -------
			//     +++
			// -------
			//      ++++++
			if(ptr->zone_ != nullptr)
				flag = 1;
			if(B > D){
				// -------
				//   +++
				int64_t len2 = B - D;
				ptr->length_ = C - A;
				this->Push(ptr->zone_, D, len2);
				if(ptr->zone_ != nullptr)
					s = ptr->zone_->Delete(length);
			}else{
				// ------
				//    ++++++
				// -------
				//     +++
				ptr->length_ -= (B - C);
				if(ptr->zone_ != nullptr)
					s = ptr->zone_->Delete(B - C);
			}
		}else if(A > C and D > A){
			//    ---
			// ++++++++
			//    --------
			// +++++
			//    ----
			// +++++++
			if(ptr->zone_ != nullptr)
				flag = 1;
			if(D > B || D == B){
				//   ----
				// ++++++++++
				//     ---
				// +++++++
				ZoneExtent* del = ptr;
				if(pre == nullptr){
					head_ = ptr->next_;
					ptr = head_;
				}else{
					pre->next_ = ptr->next_;
					ptr = ptr->next_;
				}
				delete del;
				continue;
			}else if(D < B){
				//    -----
				// ++++
				ptr->sector_ = D;
				ptr->length_ -= (D - A);
				if(ptr->zone_ != nullptr)
					s = ptr->zone_->Delete(D - A);
			}
		}else if(A == C){
			// -----
			// +++++
			// -----
			// ++++++++
			// -----
			// ++
			if(ptr->zone_ != nullptr)
				flag = 1;
			if(B <= D){
				ZoneExtent* del = ptr;
				if(pre == nullptr){
					head_ = ptr->next_;
					ptr = head_;
				}else{
					pre->next_ = ptr->next_;
					ptr = ptr->next_;
				}
				delete del;
				continue;
			}else{
				ptr->sector_ = D;
				ptr->length_ = B - D;
				if(ptr->zone_ != nullptr)
					s = ptr->zone_->Delete(length);
			}
		}

		pre = ptr;
		ptr = ptr->next_;
		if(s == -1)
			return -1;
	}

	// insert
	pre = nullptr;
	ptr = head_;
	while(ptr != nullptr){
		if(ptr->sector_ > addr){
			if(pre == nullptr){
				head_ = new ZoneExtent(zone, addr, length, ptr);
				return flag;
			}
			pre->next_ = new ZoneExtent(zone, addr, length, ptr);
			return flag;
		}
		pre = ptr;
		ptr = ptr->next_;
	}
	if(head_ == nullptr)
		head_ = new ZoneExtent(zone, addr, length, nullptr);
	else
		pre->next_ = new ZoneExtent(zone, addr, length, nullptr);
	return flag;
}

ZoneExtent* ZoneExtentList::GetHead(){
	return head_;
}

ZoneFile::ZoneFile(ZoneBackend *zbd){
	zbd_ = zbd;
	active_zone_ = nullptr;
	extents = new ZoneExtentList();
	buffered = 0;
}

int ZoneFile::AllocateNewZone(){
	zbd_->AllocateIOZone(-1, &active_zone_);
	if(active_zone_ == nullptr)
		return -1;
	return 0;
}

type_latency ZoneFile::Write(int64_t addr, int64_t data_size, bool *sthDeleted){
	int64_t left = data_size;
	int64_t offset = 0;
	int64_t wr_size = 0;
	type_latency s;
	type_latency latency = 0;
	
	while(left){
		if(buffered ==  SZBUF){
			s = FlushBuffer(sthDeleted);
			latency += s;
		}else{
			wr_size = std::min(left, SZBUF - buffered);
			s = extents->Push(nullptr, addr + offset, wr_size);
			buffered += wr_size;
			left -= wr_size;
			offset += wr_size;
		}

		if(s == 1){
			*sthDeleted = true;
		}else if(s == -1){
			if(SHOW_ERR)
				std::cout << "ZoneFile::Write Push Failed\n";
			return -1;
		}
	}
	if(buffered ==  SZBUF){
		s = FlushBuffer(sthDeleted);
		latency += s;
	}

	return latency;
}

type_latency ZoneFile::FlushBuffer(bool *sthDeleted){
	if(SHOW_ZONEFILE)
		std::cout << "-" << std::setw(69) << "Flush Buffer\n";

	type_latency latency = 0;
	ZoneExtent *extent = extents->GetHead();
	std::vector<ZoneExtent*> flush;
	type_latency s = 0;

	while(extent){
		if(extent->GetZone() == nullptr){
			flush.push_back(extent);
		}
		extent = extent->GetNext();	
	}

	while(!flush.empty()){
		if(active_zone_ == nullptr || active_zone_->IsFull()){
			AllocateNewZone();
			if(active_zone_ == nullptr){
				if(SHOW_ERR)
					std::cout << "No Space\n";
				return -1;
			}
		}
		
		int64_t wr_size = 0;
		int64_t capacity = active_zone_->GetCapacity();
		for(int i = flush.size() - 1; i >= 0; --i){
			if(wr_size == capacity)
				break;

			if(wr_size + flush[i]->GetLength() <= capacity){
				wr_size += flush[i]->GetLength();
				flush[i]->SetZone(active_zone_);
				if(SHOW_ZONEFILE)
					std::cout << "Push New Extent(addr: " << flush[i]->GetSector() << ", size: "
						<< flush[i]->GetLength() << ") to Zone[" << active_zone_->GetId() << "]\n";
				flush.pop_back();
			}else{
				// |--------|++++++++|
				// len1     len2
				int64_t len1 = flush[i]->GetLength() - (capacity - wr_size);
				int64_t len2 = capacity - wr_size;
				flush[i]->SetLength(len1);
				s = extents->Push(active_zone_, flush[i]->GetSector() + len1, len2);
				wr_size = active_zone_->GetCapacity();
				break;
			}

			if(s == 1){
				*sthDeleted = true;
			}else if(s == -1){
				if(SHOW_ERR)
					std::cout << "ZoneFile::Write Failed\n";
				return -1;
			}
		}
		s = active_zone_->Write(wr_size);
		if(s == -1){
			return -1;
		}
		latency += s;
	}
	buffered = 0;

	if(SHOW_ZONEFILE)
		std::cout << std::setw(70) << "END Flush Buffer\n";
	return latency;
}

type_latency ZoneFile::Read(int64_t addr, int64_t data_size, std::set<int> *readPages){
	int64_t offset = 0;
	int64_t left = data_size;
	int64_t rd_size = 0;
	type_latency latency = 0;
	type_latency s = 0;

	ZoneExtent *extent = extents->GetHead();
	while(extent && left > 0){
		int64_t start = extent->GetSector();
		int64_t len = extent->GetLength();
		// [,)
		if(!((start > addr + data_size) || (addr + offset > start + len))){
			if(addr + offset < start){
				return -1;
			}
			rd_size = std::min(len - (addr + offset - start), left);
			if(extent->GetZone() != nullptr)
				s = extent->GetZone()->Read(rd_size);
			else
				s = 0;
			if(s == -1){
				return -1;
			}
			latency += s;

			offset += rd_size;
			left -= rd_size;
		}
		extent = extent->GetNext();	
	}

	if(left > 0){
		return -1;
	}

	return latency;
}

ZoneExtentList* ZoneFile::GetZoneExtentList(){
	return extents;
}

void ZoneFile::DummyActiveZone(){
	active_zone_->Dummy();
}

SimpleFS::SimpleFS(){
	zbd = new ZoneBackend();
	files.resize(NRFILE, nullptr);
	nrGC = 0;
	nrRFBL = 0;
	nrRWP = 0;
	nrMigrate = 0;
	flagGCWL = false;
}

// 找到要寫的 ZoneFile call ZoneFile::Write
type_latency SimpleFS::Write(int64_t addr, int64_t data_size){
	int idx = addr / SZFILE;
	int idx_last = (addr + data_size - 1) / SZFILE;
	int64_t left = data_size;
	int64_t offset = 0;
	bool sthDeleted = false;
	type_latency latency = 0;
	type_latency s = 0;

	while(left){
		int64_t wr_size = left;
		idx = (addr + offset) / SZFILE;
		if(idx > idx_last){
			if(SHOW_ERR)
				std::cout << "ZoneFile[" << idx << "] is wrong\n";
			return -1;
		}

		if(idx_last != idx){
			wr_size = (idx + 1) * SZFILE - (addr + offset);
		}

		if(files[idx] == nullptr)
			files[idx] = new ZoneFile(zbd);

		if(SHOW_ZONEFILE)
			std::cout << "Write [" << addr+offset << "](" << wr_size << ") to ZoneFile[" << idx << "]\n"; 
		s = files[idx]->Write(addr + offset, wr_size, &sthDeleted);
		if(s == -1)
			return -1;
		latency += s;

		offset += wr_size;
		left -= wr_size;
	}

	if(sthDeleted == true){
		sthDeleted = false;
		s = ResetBeforeWP();
		latency += s;
	}

	return latency;
}

type_latency SimpleFS::Read(int64_t addr, int64_t data_size){
	int idx = addr / SZFILE;
	int idx_last = (addr + data_size - 1) / SZFILE;
	int64_t left = data_size;
	int64_t offset = 0;
	type_latency latency = 0;
	type_latency s = 0;
	std::set<int> readPages;

	while(left){
		int64_t rd_size = left;
		idx = (addr + offset) / SZFILE;
		if(idx > idx_last){
			if(SHOW_ERR)
				std::cout << "ZoneFile[" << idx << "] is wrong\n";
			return -1;
		}

		if(idx_last != idx){
			rd_size = (idx + 1) * SZFILE - (addr + offset);
		}

		if(files[idx] == nullptr){
			//if(SHOW_ERR)
				//std::cout << "ZoneFile[" << idx << "] has no data\n";
			return -1;
		}

		if(SHOW_ZONEFILE)
			std::cout << "Read [" << addr+offset << "](" << rd_size << ") from ZoneFile[" << idx << "]\n"; 
		s = files[idx]->Read(addr + offset, rd_size, &readPages);
		latency += s;
		if(s == -1)
			return -1;

		offset += rd_size;
		left -= rd_size;
	}

	// latency = readPages.size() * LATENCY_READ;
	return latency;
}

// Reference: ZenFS::GCWorker
// trigger: every 10 sec
type_latency SimpleFS::GarbageCollection(){
	int64_t free = zbd->GetFreeSpace();
	int64_t non_free = zbd->GetUsedSpace() + zbd->GetReclaimableSpace();
	int free_percent = (100 * free) / (free + non_free);
	bool sthDeleted = false;
	type_latency latency = 0;
	
	
	if(free_percent > GC_START_LEVEL)
		return 0;
	
	int ec_min = zbd->GetECMin();
	int ec_max = zbd->GetECMax();

	if(SHOW_SIMPLEFS){
		std::cout << "-----------------------------------------------------GarbageCollection\n";
		std::cout << "free:" << free << "\n";
		std::cout << "non_free:" << non_free << "\n";
		std::cout << "free_percent:" << free_percent << "\n\n";
		std::cout << "ec_max: " << ec_max << "\n";
		std::cout << "ec_min: " << ec_min << "\n\n";
	}

	int threshold = (100 - GC_SLOPE * (GC_START_LEVEL - free_percent));
	std::set<int> victim_zones;
	int64_t migrate = 0;

	if(ENABLE_GC_WL && ENABLE_DYNAMIC_MAPPING && (ec_max - ec_min) > 0.1 * (EC_LIMIT - ec_max)){
		// WL
		for(int i = 0; i < NRZONE; ++i){
			Zone *zone =  zbd->GetZone(i);
			if(zone->IsFull()){
				int reset_hint = zbd->GetResetHint(i);
	        	if (reset_hint < ec_max - 0.1 * (EC_LIMIT - ec_max)){
					if(migrate + zone->GetUsedCapacity() + SZBLK*2 <= free){
	          			victim_zones.insert(i);
						migrate += zone->GetUsedCapacity();
						flagGCWL = true;
					}
    	   	 	}
			}
		}
		if(SHOW_SIMPLEFS){
			if(victim_zones.empty())
				std::cout << "Wear Leveling Failed\n";
			else
				std::cout << "Garbage Collection with Wear Leveling\n";
		}
	}

	if(victim_zones.empty()){
		// Normal GC
		for(int i = 0; i < NRZONE; ++i){
			Zone *zone = zbd->GetZone(i);
			if(zone->IsFull()){
				int garbage_percent_approx = 100 - 100 * zone->GetUsedCapacity() / zone->GetMaxCapacity();
	        	if(garbage_percent_approx > threshold && garbage_percent_approx <= 100){
					// redundant SZBLK bytes for padding
					if(migrate + zone->GetUsedCapacity() + (SZBLK * 2) <= free){
	          			victim_zones.insert(i);
						migrate += zone->GetUsedCapacity();
					}
    	   	 	}
			}
		}
	}

	if(!victim_zones.empty()){
		++nrGC;
	}

	// migrate data
	type_latency s = 0;
	for(int i = 0; i < NRFILE; ++i){
		if(files[i] == nullptr)
			continue;
		ZoneExtent *extent = files[i]->GetZoneExtentList()->GetHead();
		while(extent){
			if(extent->GetZone() && victim_zones.count(extent->GetZone()->GetId()) != 0){
				s = files[i]->Write(extent->GetSector(), extent->GetLength(), &sthDeleted);
				if(s == -1){
					if(SHOW_ERR)
						std::cout << "No Space to Migrate:(\n";
					return -1;
				}
				latency += s;
			}
			extent = extent->GetNext();	
		}
	}

	for (std::set<int>::iterator it = victim_zones.begin(); it != victim_zones.end(); ++it){
		s = zbd->GetZone(*it)->Reset();
		latency += s;
	}

	if(SHOW_SIMPLEFS){
		// show();
		if(victim_zones.empty())
			std::cout << "Garbage Collection Failed\n";
		else
			std::cout << "Migrate " << migrate << "sectors\n";
		std::cout << "----------------------------------------------------END-GarbageCollection\n";
	}
	nrMigrate += migrate;

	return latency;
}

// 當 EC_max 跟 EC_min 差距過大且 Free Block List 內的 block 的 EC 偏高時，執行 WL
// trigger: every 10 sec if GC is not be triggerd
type_latency SimpleFS::FBLRefreshment(){
	if(!ENABLE_FBL_REFRESH || !ENABLE_DYNAMIC_MAPPING)
		return 0;
	int ec_min = zbd->GetECMin();
	int ec_max = zbd->GetECMax();
	if(!((ec_max - ec_min) > 0.1 * (EC_LIMIT - ec_max)))
		return 0;
	int ec_min_free = zbd->GetECMinFree();
	if(ec_min_free == -1 || !(ec_min_free > (ec_min + 0.9 * (ec_max - ec_min))))
		return 0;
	
	bool sthDeleted = false;

	if(SHOW_SIMPLEFS){
		std::cout << "-----------------------------------------------------FreeBlockListRefreshment\n";
		std::cout << "ec_max: " << ec_max << "\n";
		std::cout << "ec_min: " << ec_min << "\n";
		std::cout << "ec_min_free: " << ec_min_free << "\n\n";
	}

	std::set<int> victim_zones;
	int64_t free = zbd->GetFreeSpace();
	int64_t migrate = 0;

	for(int i = 0; i < NRZONE; ++i){
		Zone *zone =  zbd->GetZone(i);
		if(zone->IsFull()){
			int reset_hint = zbd->GetResetHint(i);
        	if (reset_hint < ec_max - 0.1 * (EC_LIMIT - ec_max)){
				if(migrate + zone->GetUsedCapacity() + SZBLK <= free){
          			victim_zones.insert(i);
					migrate += zone->GetUsedCapacity();
				}
	   	 	}
		}
	}

	int s = 0;
	for(int i = 0; i < NRFILE; ++i){
		if(files[i] == nullptr)
			continue;
		ZoneExtent *extent = files[i]->GetZoneExtentList()->GetHead();
		while(extent){
			if(extent->GetZone() && victim_zones.count(extent->GetZone()->GetId()) != 0){
				s = files[i]->Write(extent->GetSector(), extent->GetLength(), &sthDeleted);
				if(s == -1){
					if(SHOW_ERR)
						std::cout << "No Space to Migrate:(\n";
					return -1;
				}
			}
			extent = extent->GetNext();	
		}
	}

	if(!victim_zones.empty()){
		++nrRFBL;
	}

	for (std::set<int>::iterator it = victim_zones.begin(); it != victim_zones.end(); ++it){
		zbd->GetZone(*it)->Reset();
	}

	if(SHOW_SIMPLEFS){
		if(victim_zones.empty())
			std::cout << "Free Block List Refreshment Failed\n";
		else
			std::cout << "Migrate " << migrate << "bytes\n";
		std::cout << "----------------------------------------------------END-FreeBlockListRefreshment\n";
	}

	return 0;
}

// Reference:ZonedBlockDevice::ResetUnusedIOZones
// wp 前沒資料就 reset
// trigger: deletion
type_latency SimpleFS::ResetBeforeWP(){
	bool flag = false;
	type_latency latency = 0;
	for(int i = 0; i < NRZONE; ++i){
		if(zbd->GetZone(i)->GetWP() != 0 && zbd->GetZone(i)->GetUsedCapacity() == 0){
			if(SHOW_SIMPLEFS && !flag)
				std::cout << "-----------------------------------------------------ResetBeforeWP\n";
			latency += zbd->GetZone(i)->Reset();
			flag = true;
			++nrRWP;
		}
	}
	
	if(flag && SHOW_SIMPLEFS){
		std::cout << "-----------------------------------------------------END-ResetBeforeWP\n";
	}
	 return latency;
}

void SimpleFS::FillUp(){
	bool dummy;
	int s;
	for(int64_t i = 0; i < NRFILE; ++i){
		if(files[i] == nullptr)
			files[i] = new ZoneFile(zbd);
		files[i]->Write(i * SZFILE, SZFILE, &dummy);
		/*
		s = files[i]->Write(i * SZFILE, SZFILE / 2, &dummy);
		if(s == -1)
			return;
		files[i]->FlushBuffer(&dummy);
		files[i]->DummyActiveZone();
		s = files[i]->Write((i * SZFILE) + (SZFILE / 2), SZFILE / 2, &dummy);
		if(s == -1)
			return;
		files[i]->FlushBuffer(&dummy);
		files[i]->DummyActiveZone();
		*/
	}	
	//s = files[NRFILE - 1]->Write((int64_t)(NRFILE - 1) * SZFILE, SZFILE, &dummy);
	//files[NRFILE - 1]->FlushBuffer(&dummy);
	//files[NRFILE - 1]->DummyActiveZone();
	Flush();
}

void SimpleFS::Flush(){
	bool dummy;
	for(int i = 0; i < NRFILE; ++i){
		if(files[i] != nullptr)
			files[i]->FlushBuffer(&dummy);
	}
}

void SimpleFS::check(){
	int64_t used_capacity = zbd->GetUsedSpace();
	std::cout << used_capacity << " <= " << SZFS << "?\n";
}
