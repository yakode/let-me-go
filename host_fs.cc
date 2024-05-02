#include "host_fs.h"

ZoneExtent::~ZoneExtent(){
	if(zone_ != nullptr)
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

Zone* ZoneExtent::GetZone(){
	return zone_;
}

void ZoneExtent::SetLength(int length){
	length_ = length;
}

void ZoneExtent::SetZone(Zone *zone){
	zone_ = zone;
}

int ZoneExtentList::Push(Zone *zone, int addr, int length){
	if(SHOW_ZONEFILE){
		if(zone)
			std::cout << "Push New Extent("<< addr << ", " << length << ") to Zone[" << zone->GetId() << "]\n";
		else
			std::cout << "Push New Extent(" << addr << ", " << length << ")  to Buffer\n";
	}

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
				int len2 = B - D;
				ptr->length_ = C - A;
				this->Push(ptr->zone_, D, len2);
				if(ptr->zone_ != nullptr)
					ptr->zone_->Delete(length);
			}else{
				// ------
				//    ++++++
				// -------
				//     +++
				ptr->length_ -= (B - C);
				if(ptr->zone_ != nullptr)
					ptr->zone_->Delete(B - C);
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
					ptr->zone_->Delete(D - A);
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
	buffered = 0;
}

int ZoneFile::AllocateNewZone(){
	zbd_->AllocateIOZone(-1, &active_zone_);

	return -1;
}

int ZoneFile::Write(int addr, int data_size){
	int left = data_size;
	int offset = 0;
	int wr_size;
	int s, deleted = 0;
	// -1: failed; 0: ok; 1: deletion
	
	while(left){
		if(buffered ==  SZBUF){
			s = FlushBuffer();
		}else{
			wr_size = std::min(left, SZBUF - buffered);
			s = extents->Push(nullptr, addr + offset, wr_size);
			buffered += wr_size;
			left -= wr_size;
			offset += wr_size;
		}

		if(s == 1){
			deleted = 1;
		}else if(s == -1){
			if(SHOW_ERR)
				std::cout << "ZoneFile::Write Push Failed\n";
			return -1;
		}
	}

	return deleted;
}

int ZoneFile::FlushBuffer(){
	if(SHOW_ZONEFILE)
		std::cout << "Flush Buffer\n";
	ZoneExtent *extent = extents->GetHead();
	std::vector<ZoneExtent*> flush;
	int s = 0, deleted = 0;

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
		
		int wr_size = 0;
		int capacity = active_zone_->GetCapacity();
		for(int i = flush.size() - 1; i >= 0; --i){
			if(wr_size == capacity)
				break;

			if(wr_size + flush[i]->GetLength() <= capacity){
				wr_size += flush[i]->GetLength();
				flush[i]->SetZone(active_zone_);
				if(SHOW_ZONEFILE)
					std::cout << "Push New Extent("<< flush[i]->GetSector() << ", " << flush[i]->GetLength() << ") to Zone[" << active_zone_->GetId() << "]\n";
				flush.pop_back();
			}else{
				// |--------|++++++++|
				// len1     len2
				int len1 = flush[i]->GetLength() - (capacity - wr_size);
				int len2 = capacity - wr_size;
				flush[i]->SetLength(len1);
				s = extents->Push(active_zone_, flush[i]->GetSector() + len1, len2);
				wr_size = active_zone_->GetCapacity();
				break;
			}

			if(s == 1){
				deleted = 1;
			}else if(s == -1){
				if(SHOW_ERR)
					std::cout << "ZoneFile::Write Failed\n";
				return -1;
			}
		}
		active_zone_->Write(wr_size);
		buffered -= wr_size;
	}

	return deleted;
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
		if(idx > idx_last && SHOW_ERR){
			std::cout << "ZoneFile[" << idx << "] is wrong\n";
		}

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
		std::cout << "free:" << free << "\n";
		std::cout << "non_free:" << non_free << "\n";
		std::cout << "free + non_free:" << free + non_free << "\n";
		std::cout << "free_percent:" << free_percent << "\n";
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
			if(extent->GetZone() && victim_zones.count(extent->GetZone()->GetId()) != 0){
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
			if(!flag)
				std::cout << "-----------------------------------------------------ResetBeforeWP\n";
			zbd->GetZone(i)->Reset();
			flag = true;
		}
	}

	if(flag && SHOW_SIMPLEFS){
		std::cout << "-----------------------------------------------------END-ResetBeforeWP\n";
		// show();
	}
}
