#include "host_zbd.h"

Zone::Zone(){
	max_capacity_ = SZZONE * SZBLK;	
	capacity_ = SZZONE * SZBLK;	
	wp_ = 0;
	used_capacity_ = 0;
}

Zone::Zone(int id, BlockManager *blkmgr){
	id_ = id;
	max_capacity_ = SZZONE * SZBLK;	
	capacity_ = SZZONE * SZBLK;	
	wp_ = 0;
	used_capacity_ = 0;
	blkmgr_ = blkmgr;
}

int Zone::Read(){return -1;}

int Zone::Write(int data_size){
	int s;
	if(data_size % SZBLK != 0){
		data_size = (data_size / SZBLK + 1) * SZBLK;
	}
	if(data_size % SZBLK != 0 && SHOW_ERR){
		std::cout << "Zone Write Failed, Padding size is fault\n";
		return -1;
	}

	s = blkmgr_->Append(id_, wp_, data_size);
	if(s == -1){
		if(SHOW_ERR)
			std::cout << "Zone Write Failed, zoneid:" << id_ << ", wp:" << wp_ << ", data_size:" << data_size << "\n";
		return -1;
	}
	capacity_ -= data_size;
	wp_ += data_size;
	used_capacity_ += data_size;
	return 0;
}

int Zone::Reset(){
	if(SHOW_RESET)
		std::cout << "Reset Zone[" << id_ << "]\n";
		
	capacity_ = SZZONE * SZBLK;	
	wp_ = 0;
	used_capacity_ = 0;
	blkmgr_->Reset(id_);
	return 0;
}

int Zone::Delete(int data_size){
	if(data_size > used_capacity_)
		return -1;
	if(SHOW_ZONE)
		std::cout << "Delete data whose length is " << data_size << " from Zone[" << id_ << "]\n";
	used_capacity_ -= data_size;
	return 0;
}

bool Zone::IsFull(){
	return capacity_ == 0;
}

int Zone::GetCapacity(){
	return capacity_;
}

int Zone::GetUsedCapacity(){
	return used_capacity_;
}

int Zone::GetMaxCapacity(){
	return max_capacity_;
}

int Zone::GetWP(){
	return wp_;
}

ZoneBackend::ZoneBackend(){
	blkmgr = new BlockManager();
	zones.resize(NRZONE, nullptr);
	for(int i = 0; i < NRZONE; ++i)
		zones[i] = new Zone(i, blkmgr);
}

int ZoneBackend::AllocateIOZone(int lifetime, Zone **zone){
	for(int i = 0; i < NRZONE; ++i){
		if(zones[i]->GetCapacity() != 0){
			*zone = zones[i];
			return 0;
		}
	}
	*zone = nullptr;
	return -1;
}

int ZoneBackend::GetFreeSpace(){
	int free = 0;
	for(int i = 0; i < NRZONE; ++i)
		free += zones[i]->GetCapacity();
	return free;
}

int ZoneBackend::GetUsedSpace(){
	int used = 0;
	for(int i = 0; i < NRZONE; ++i)
		used += zones[i]->GetUsedCapacity();
	return used;
}

int ZoneBackend::GetReclaimableSpace(){
	int reclaimable = 0;
	for(int i = 0; i < NRZONE; ++i)
		if(zones[i]->IsFull())
			reclaimable += (zones[i]->GetMaxCapacity() - zones[i]->GetUsedCapacity());
	return reclaimable;
}
