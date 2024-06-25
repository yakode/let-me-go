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

type_latency Zone::Read(int rd_size){
	int64_t latency = ceil((double)rd_size / (double)SZPAGE) * LATENCY_READ;
	blkmgr_->AddReadAmount( ceil((double)rd_size / (double)SZPAGE) );
	return latency;
}

// Manage the metadata of the Zone
// and transport append command to Device side
type_latency Zone::Write(int data_size){
	int64_t s;
	int64_t wr_size = data_size;

	// padding
	if(data_size % SZBLK != 0){
		wr_size = (data_size / SZBLK + 1) * SZBLK;
	}
	if(wr_size % SZBLK != 0 && SHOW_ERR){
		std::cout << "Zone Write Failed, Padding size is fault\n";
		return -1;
	}

	// Append data to physical block
	// (what I really do is to allocate dynamically physical block to virtual zone)
	s = blkmgr_->Append(id_, wp_, wr_size);
	if(s == -1){
		if(SHOW_ERR)
			std::cout << "Zone Write Failed, zoneid:" << id_ << ", wp:" << wp_ << ", data_size:" << data_size << "\n";
		return -1;
	}
	capacity_ -= wr_size;
	wp_ += wr_size;
	used_capacity_ += data_size;
	return s;
}

void Zone::Dummy(){
	int s = blkmgr_->Append(id_, wp_, capacity_);
	if(s == -1){
		if(SHOW_ERR)
			std::cout << "Dummy Failed\n"; 
		return;
	}
	capacity_ = 0;
	wp_ = max_capacity_;
}

// Manage the metadata of the Zone
// and transport RESET command to Device side
type_latency Zone::Reset(){
	if(SHOW_RESET)
		std::cout << "Reset Zone[" << id_ 
		<< "] whose reset hint is " << blkmgr_->GetResetHint(id_)
		<< " and migrate " << used_capacity_ 
		<< "bytes(" << used_capacity_ * 100 / (SZZONE * SZBLK) << "\%)\n";
		
	capacity_ = (int64_t)SZZONE * (int64_t)SZBLK;	
	wp_ = 0;
	used_capacity_ = 0;
	int64_t latency = blkmgr_->Reset(id_);
	return latency;
}

// Manage the metadata of the Zone
int Zone::Delete(int64_t data_size){
	if(data_size > used_capacity_){
		if(SHOW_ERR){
			std::cout << "Zone Deletion Failed, try to delete " << data_size 
			<< "bytes from zone[" << id_ << "] whose used capacity is " << used_capacity_ << "\n";
		}
		return -1;
	}

	if(SHOW_ZONE)
		std::cout << "Delete data whose length is " << data_size << " bytes from Zone[" << id_ << "]\n";
	used_capacity_ -= data_size;
	return 0;
}

bool Zone::IsFull(){
	return capacity_ == 0;
}

bool Zone::IsBroken(){
	return blkmgr_->GetResetHint(id_) >= EC_LIMIT;
}

int Zone::GetId(){
	return id_;
}

int64_t Zone::GetCapacity(){
	return capacity_;
}

int64_t Zone::GetWP(){
	return wp_;
}

int64_t Zone::GetUsedCapacity(){
	return used_capacity_;
}

int64_t Zone::GetMaxCapacity(){
	return max_capacity_;
}

ZoneBackend::ZoneBackend(){
	if(ENABLE_DYNAMIC_MAPPING)
		blkmgr = new BlockManagerDynamic();
	else
		blkmgr = new BlockManagerStatic();
	zones.resize(NRZONE, nullptr);
	for(int i = 0; i < NRZONE; ++i)
		zones[i] = new Zone(i, blkmgr);
}

int ZoneBackend::AllocateIOZone(int lifetime, Zone **zone){
	for(int i = 0; i < NRZONE; ++i){
		if(zones[i]->GetCapacity() != 0){
			if(!ENABLE_DYNAMIC_MAPPING && blkmgr->GetResetHint(i) >= EC_LIMIT)
				continue;
			*zone = zones[i];
			return 0;
		}
	}
	*zone = nullptr;
	return -1;
}

int64_t ZoneBackend::GetFreeSpace(){
	int64_t free = 0;
	for(int i = 0; i < NRZONE; ++i)
		free += zones[i]->GetCapacity();
	return free;
}

int64_t ZoneBackend::GetUsedSpace(){
	int64_t used = 0;
	for(int i = 0; i < NRZONE; ++i)
		used += zones[i]->GetUsedCapacity();
	return used;
}

// the size of garbage of full zones
int64_t ZoneBackend::GetReclaimableSpace(){
	int64_t reclaimable = 0;
	for(int i = 0; i < NRZONE; ++i)
		if(zones[i]->IsFull())
			reclaimable += (zones[i]->GetMaxCapacity() - zones[i]->GetUsedCapacity());
	return reclaimable;
}

int64_t ZoneBackend::GetGarbage(){
	int64_t garbage = 0;
	for(int i = 0; i < NRZONE; ++i)
		garbage += (zones[i]->GetWP() - zones[i]->GetUsedCapacity());
	return garbage;
}

int ZoneBackend::GetECMin(){
	return blkmgr->GetECMin(); 
}

int ZoneBackend::GetECMax(){
	return blkmgr->GetECMax(); 
}

int ZoneBackend::GetECMinFree(){
	return blkmgr->GetECMinFree(); 
}

int ZoneBackend::GetResetHint(int zoneid){
	return blkmgr->GetResetHint(zoneid);
}
