#include "host_fs.h"

ZoneExtent::~ZoneExtent(){
	zone_->Delete(length_);
}

ZoneExtent* ZoneExtent::GetNext(){
	return next_;
}

int ZoneExtent::GetSector(){
	return sector_;
}

int ZoneExtent::GetLength(){
	return length_;
}

int ZoneExtent::GetZone(){
	return zone_->GetId();
}

int ZoneExtentList::Push(Zone *zone, int addr, int length){
	if(SHOW_ZONEFILE)
		std::cout << "Push New Extent to Zone[" << zone->GetId() << "]\n";

	int flag = 0;
	// 0: ok; -1: failed; 1: deletion
	if(head_ == nullptr){
		head_ = new ZoneExtent(zone, addr, length, nullptr);
		return 0;
	}
	
	ZoneExtent *pre = nullptr;
	ZoneExtent *ptr = head_;
	// delete overlapping
	while(ptr != nullptr){
		// 這裡的寫法有問題，要修
		// 把重疊的舊資料刪除
		// 如果寫好 Push Buffer 就不用改這裡
		int A = ptr->sector_;
		int B = ptr->sector_ + ptr->length_;
		int C = addr;
		int D = addr + length;
		// A------B
		//    C++++++D
		// [A, B) [C, D)
		if(C >= B)
			break;
		if(A < C and B > C){
			// -------
			//   +++
			// -------
			//     +++
			// -------
			//      ++++++
			flag = 1;
			if(B > D){
				// -------
				//   +++
				int len2 = B - D;
				ptr->length_ = C - A;
				this->Push(ptr->zone_, D, len2);
				ptr->zone_->Delete(length);
			}else{
				// ------
				//    ++++++
				// -------
				//     +++
				ptr->length_ -= (B - C);
				ptr->zone_->Delete(B - C);
			}
		}else if(A > C and D > A){
			//    ---
			// ++++++++
			//    --------
			// +++++
			//    ----
			// +++++++
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
				ptr->zone_->Delete(D - A);
			}
		}else if(A == C){
			// -----
			// +++++
			// -----
			// ++++++++
			// -----
			// ++
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
				ptr->zone_->Delete(length);
			}
		}

		pre = ptr;
		ptr = ptr->next_;
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
}

int ZoneFile::AllocateNewZone(){
	zbd_->AllocateIOZone(-1, &active_zone_);

	return -1;
}

int ZoneFile::Write(int addr, int data_size){
	int left = data_size;
	int offset = 0;
	int wr_size;
	int s, ret = 0;
	// -1: failed; 0: ok; 1: deletion

	while(left){
		if(active_zone_ == nullptr || active_zone_->IsFull()){
			AllocateNewZone();
			if(active_zone_ == nullptr){
				if(SHOW_ERR)
					std::cout << "No Space\n";
				return -1;
			}
		}

		wr_size = std::min(left, active_zone_->GetCapacity());
		s = extents->Push(active_zone_, addr + offset, wr_size);
		active_zone_->Write(wr_size);
		if(s == 1){
			ret = 1;
		}
		if(s == -1){
			if(SHOW_ERR)
				std::cout << "ZoneFile::Write Failed\n";
			return -1;
		}

		left -= wr_size;
		offset += wr_size;
	}

	return ret;
}

int ZoneFile::Read(){return -1;}

ZoneExtentList* ZoneFile::GetZoneExtentList(){
	return extents;
}

SimpleFS::SimpleFS(){
	zbd = new ZoneBackend();
	files.resize(NRFILE, nullptr);
}

int SimpleFS::Write(int addr, int data_size){
	int idx = addr / SZFILE;
	int idx_last = (addr + data_size - 1) / SZFILE;
	int left = data_size;
	int offset = 0;
	int flag = 0;

	while(left){
		int wr_size = left;
		idx = (addr + offset) / SZFILE;
		if(idx_last != idx){
			wr_size = (idx + 1) * SZFILE - (addr + offset);
		}

		if(files[idx] == nullptr)
			files[idx] = new ZoneFile(zbd);

		if(SHOW_ZONEFILE)
			std::cout << "Write [" << addr+offset << "](" << wr_size << ") to ZoneFile[" << idx << "]\n"; 
		int s = files[idx]->Write(addr + offset, wr_size);
		if(s == -1)
			return -1;
		else if(s == 1)
			flag = 1;

		offset += wr_size;
		left -= wr_size;
	}

	if(flag)
		ResetBeforeWP();

	return 0;
}

void SimpleFS::GarbageCollection(){
	int free = zbd->GetFreeSpace();
	int non_free = zbd->GetUsedSpace() + zbd->GetReclaimableSpace();
	int free_percent = (100 * free) / (free + non_free);
	
	if(free_percent > GC_START_LEVEL)
		return;

	if(SHOW_SIMPLEFS){
		std::cout << "-----------------------------------------------------GarbageCollection\n";
		// show();
	}

	int threshold = (100 - GC_SLOPE * (GC_START_LEVEL - free_percent));
	std::set<int> victim_zones;
	int migrate = 0;

	for(int i = 0; i < NRZONE; ++i){
		Zone *zone =  zbd->GetZone(i);
		if(zone->IsFull()){
			int garbage_percent_approx = 100 - 100 * zone->GetUsedCapacity() / zone->GetMaxCapacity();
        	if (garbage_percent_approx > threshold && garbage_percent_approx < 100) {
				if(migrate + zone->GetUsedCapacity() <= free){
          			victim_zones.insert(i);
					migrate += zone->GetUsedCapacity();
				}
        	}
		}
	}

	for(int i = 0; i < NRFILE; ++i){
		if(files[i] == nullptr)
			continue;
		ZoneExtent *extent = files[i]->GetZoneExtentList()->GetHead();
		while(extent){
			if(victim_zones.count(extent->GetZone()) != 0){
				files[i]->Write(extent->GetSector(), extent->GetLength());
			}
			extent = extent->GetNext();	
		}
	}

	for (std::set<int>::iterator it = victim_zones.begin(); it != victim_zones.end(); ++it){
		zbd->GetZone(*it)->Reset();
	}

	if(SHOW_SIMPLEFS){
		// show();
		if(victim_zones.empty())
			std::cout << "Garbage Collection Failed\n";
		std::cout << "----------------------------------------------------END-GarbageCollection\n";
	}
}

void FBLRefreshment(){}

void SimpleFS::ResetBeforeWP(){
	bool flag = false;
	for(int i = 0; i < NRZONE; ++i){
		if(zbd->GetZone(i)->GetWP() != 0 && zbd->GetZone(i)->GetUsedCapacity() == 0){
			zbd->GetZone(i)->Reset();
			flag = true;
		}
	}

	if(flag && SHOW_SIMPLEFS){
		std::cout << "-----------------------------------------------------ResetBeforeWP\n";
		// show();
	}
}
