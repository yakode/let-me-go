#include "dev_blkmgr.h"

FBLNode::~FBLNode(){
	LLNode *tmp;
	while(head != nullptr){
		tmp = head;
		head = head->next;
		delete tmp;
	}
}

void FBLNode::AddLLNode(int blkid){
	this->nr += 1;
	LLNode *tmp = new LLNode(blkid, this->head);
	this->head = tmp;
}

int FBLNode::DeleteLLHead(){
	int ret = this->head->blkid;
	this->nr -= 1;
	LLNode *tmp = this->head;
	this->head = this->head->next;
	delete tmp;
	return ret;
}

FreeBlockList::FreeBlockList(){
	this->nil_ = new FBLNode(-1);
	this->root_ = this->nil_;
	this->nil_->p = this->nil_;
	this->nil_->color = true;
	this->min_ = this->nil_;
}

FreeBlockList::~FreeBlockList(){
}

void FreeBlockList::LeftRotate(FBLNode *x){
	FBLNode *y;
	y = x->right;
	x->right = y->left;
	if(y->left != this->nil_)
		y->left->p = x;
	y->p = x->p;
	if(x->p == this->nil_){
		this->root_ = y;
	}else if(x == x->p->left){
		x->p->left = y;
	}else{
		x->p->right = y;
	}
	y->left = x;
	x->p = y;
}

void FreeBlockList::RightRotate(FBLNode *x){
	FBLNode *y;
	y = x->left;
	x->left = y->right;
	if(y->right != this->nil_)
		y->right->p = x;
	y->p = x->p;
	if(x->p == this->nil_){
		this->root_ = y;
	}else if(x == x->p->right){
		x->p->right = y;
	}else{
		x->p->left = y;
	}
	y->right = x;
	x->p = y;
}

void FreeBlockList::RBTransplant(FBLNode *u, FBLNode *v){
	if(u->p == this->nil_)
		this->root_ = v;
	else if(u == u->p->left)
		u->p->left = v;
	else
		u->p->right = v;
	v->p = u->p;
}

void FreeBlockList::RBDeleteFixup(FBLNode *x){
	FBLNode *w;
	while(x != this->root_ && x->color == true){
		if(x == x->p->left){
			w = x->p->right;
			if(w->color == false){
				w->color = true;
				x->p->color = false;
				LeftRotate(x->p);
				w = x->p->right;
			}
			if(w->left->color == true && w->right->color == true){
				w->color = false;
				x = x->p;
			}else{
				if(w->right->color == true){
					w->left->color = true;
					w->color = false;
					RightRotate(w);
					w = x->p->right;
				}
				w->color = x->p->color;
				x->p->color = true;
				w->right->color = true;
				LeftRotate(x->p);
				x = this->root_;
			}
		}else{
			w = x->p->left;
			if(w->color == false){
				w->color = true;
				x->p->color = false;
				RightRotate(x->p);
				w = x->p->left;
			}
			if(w->right->color == true && w->left->color == true){
				w->color = false;
				x = x->p;
			}else{
				if(w->left->color == true){
					w->right->color = true;
					w->color = false;
					LeftRotate(w);
					w = x->p->left;
				}
				w->color = x->p->color;
				x->p->color = true;
				w->left->color = true;
				RightRotate(x->p);
				x = this->root_;
			}
		}
	}
	x->color = true;
}

FBLNode* FreeBlockList::TreeMinimum(FBLNode *x){
	if(x == this->nil_)
		return x;
	while(x->left != this->nil_)
		x = x->left;
	return x;
}

int FreeBlockList::Pop(){
	FBLNode *x, *y, *z;
	int ret;
	z = this->min_;
	if(z == this->nil_)
		return -1;
	if(z->nr > 1)
		return z->DeleteLLHead();
	if(this->min_ == z){
		if(z->right == this->nil_)
			this->min_ = z->p;
		else
			this->min_ = z->right;
	}
	ret = z->key;
	
	y = z;
	bool y_original_color = y->color;
	if(z->left == this->nil_){
		x = z->right;
		RBTransplant(z, z->right);
	}else if(z->right == this->nil_){
		x = z->left;
		RBTransplant(z, z->left);
	}else{
		y = TreeMinimum(z->right);
		y_original_color = y->color;
		x = y->right;

		if(y->p == z){
			x->p = y;
		}else{
			RBTransplant(y, y->right);
			y->right = z->right;
			y->right->p = y;
		}
		RBTransplant(z, y);
		y->left = z->left;
		y->left->p = y;
		y->color = z->color;
	}
	if(y_original_color == true){
		RBDeleteFixup(x);
	}
	if(this->min_ == this->nil_)
		this->min_ = this->root_;
	
	delete z;
	return ret;
}

void FreeBlockList::RBInsertFixup(FBLNode *z){
	FBLNode *y;
	while(z->p->color == false){
		if(z->p == z->p->p->left){
			y = z->p->p->right;
			if(y->color == false){
				z->p->color = true;
				y->color = true;
				z->p->p->color = false;
				z = z->p->p;
			}else{
				if(z == z->p->right){
					z = z->p;
					LeftRotate(z);
				}
				z->p->color = true;
				z->p->p->color = false;
				RightRotate(z->p->p);
			}
		}else{
			y = z->p->p->left;
			if(y->color == false){
				z->p->color = true;
				y->color = true;
				z->p->p->color = false;
				z = z->p->p;
			}else{
				if(z == z->p->left){
					z = z->p;
					RightRotate(z);
				}
				z->p->color = true;
				z->p->p->color = false;
				LeftRotate(z->p->p);
			}
		}
	}
	this->root_->color = true;
}

void FreeBlockList::Insert(int blkid, int ec){
	FBLNode *x, *y, *z;
	bool flag;
	y = this->nil_;
	x = this->root_;
	flag = true;
	while(x != this->nil_){
		y = x;
		if(ec < x->key){
			x = x->left;
		}else if(ec > x->key){
			x = x->right;
			flag = false;
		}else{
			x->AddLLNode(blkid);
			return;
		}
	}
	z = new FBLNode(ec, y, this->nil_);
	if(flag)
		this->min_ = z;
	z->AddLLNode(blkid);
	if(y == this->nil_)
		this->root_ = z;
	else if(z->key < y->key)
		y->left = z;
	else
		y->right = z;
	RBInsertFixup(z);
}

int FreeBlockList::GetMinEC(){
	return this->min_->key;
}

BlockEraseCountRecord::BlockEraseCountRecord(){
	record = new int[NRBLK];
	for(int i = 0; i < NRBLK; ++i)
		record[i] = 0;
}

BlockEraseCountRecord::~BlockEraseCountRecord(){
	delete [] record;
}

void BlockEraseCountRecord::AddEC(int blkid){
	record[blkid] += 1;
}

int BlockEraseCountRecord::GetEC(int blkid){
	return record[blkid];
}

MappingTable::MappingTable(){
	mt = new int*[NRZONE];
	for(int i = 0; i < NRZONE; ++i)
		mt[i] = new int[SZZONE];
	allocated = new int[NRZONE];

	if(YAK){
		for(int i = 0; i < NRZONE; ++i)
			allocated[i] = 0;
		for(int i = 0; i < NRZONE; ++i)
			for(int j = 0; j < SZZONE; ++j)
				mt[i][j] = -1;
	}else{
		for(int i = 0; i < NRZONE; ++i)
			allocated[i] = SZZONE;
		for(int i = 0; i < NRZONE; ++i)
			for(int j = 0; j < SZZONE; ++j)
				mt[i][j] = SZZONE * i + j;
	}
}

MappingTable::~MappingTable(){
	delete [] allocated;
	for(int i = 0; i < NRZONE; ++i)
		delete [] mt[i];
	delete [] mt;
}

void MappingTable::AddBlockToZone(int zoneid, int blkid){
	if(SHOW_ZONE)
		std::cout << "Add Block[" << blkid << "] to Zone[" << zoneid << "][" << allocated[zoneid] << "]\n";
	mt[zoneid][allocated[zoneid]] = blkid;
	allocated[zoneid] += 1;
}

bool MappingTable::IsAllocated(int zoneid, int idx){
	return idx < allocated[zoneid];
}

int MappingTable::GetSize(int zoneid){
	return allocated[zoneid];
}

int MappingTable::GetBlk(int zoneid, int idx){
	return mt[zoneid][idx];
}

void MappingTable::ResetMT(int zoneid){
	allocated[zoneid] = 0;
	for(int i = 0; i < SZZONE; ++i)
		mt[zoneid][i] = -1;
}

ResetHintTable::ResetHintTable(){
	rht = new int[NRZONE];
	for(int i = 0; i < NRZONE; ++i)
		rht[i] = -1;
}

void ResetHintTable::SetResetHint(int zoneid, int rh){
	if(zoneid >= NRZONE){
		if(SHOW_ERR)
			std::cout << "wrong zoneid: " << zoneid << "\n";
		return;
	}
	rht[zoneid] = rh;
}

int ResetHintTable::GetResetHint(int zoneid){
	if(zoneid >= NRZONE){
		if(SHOW_ERR)
			std::cout << "wrong zoneid: " << zoneid << "\n";
		return -1;
	}
	return rht[zoneid];
}

int ResetHintTable::GetMinRH(){
	int ret = -1;
	for(int i; i < NRZONE; ++i)
		if(rht[i] != -1)
			if(ret == -1 || ret > rht[i])
				ret = rht[i];
	return ret;
}

BlockManager::BlockManager(){
	fblist = new FreeBlockList();
	blkec = new BlockEraseCountRecord();
	mtable = new MappingTable();
	rhtable = new ResetHintTable();

	if(YAK){
		for(int i = 0; i < NRBLK; ++i)
			fblist->Insert(i, 0);
	}

	EC_max = 0;
	EC_min_free = 0;
}

BlockManager::~BlockManager(){
	delete fblist;
	delete blkec;
	delete mtable;
	delete rhtable;
}

int BlockManager::Allocate(int zoneid){
	int blkid = this->fblist->Pop();
	if(blkid == -1){
		return -1;
	}
	int ec = blkec->GetEC(blkid);
	int hint = rhtable->GetResetHint(zoneid);
	if(hint == -1 || ec < hint)
		rhtable->SetResetHint(zoneid, ec);
	this->EC_min_free = this->fblist->GetMinEC();
	this->mtable->AddBlockToZone(zoneid, blkid);
	
	return 0;
}

int BlockManager::Reset(int zoneid){
	// Reset Reset Hint of zone[zoneid]
	// Add Block erase count
	int sz = this->mtable->GetSize(zoneid);
	if(YAK){
		int rh = this->rhtable->GetResetHint(zoneid);
		this->rhtable->SetResetHint(zoneid, -1);

		for(int i = 0; i < sz; ++i){
			int blkid = this->mtable->GetBlk(zoneid, i);
			int ec = this->blkec->GetEC(blkid);
			this->Erase(blkid);
			this->blkec->AddEC(blkid);
			if(ec == this->EC_max)
				this->EC_max += 1;
			this->fblist->Insert(blkid, ec + 1);
		}

		if(rh == this->EC_min){
			if(this->EC_min_free != this->EC_min){
				int rh_min = this->rhtable->GetMinRH();
				if(rh_min < this->EC_min_free)
					this->EC_min = rh_min;
				else
					this->EC_min = this->EC_min_free;
			}
		}
		this->mtable->ResetMT(zoneid);
	}else{
		for(int i = 0; i < sz; ++i){
			int blkid = this->mtable->GetBlk(zoneid, i);
			this->Erase(blkid);
			this->blkec->AddEC(blkid);
		}
	}

	return 0;
}

int BlockManager::Erase(int blkid){
	return -1;
}

int BlockManager::Append(int zoneid, int offset, int data_size){
	if(!YAK)
		return 0;

	int idx = offset / SZBLK;
	int last_idx = (offset + data_size - 1) / SZBLK;
	for(int i = idx; i <= last_idx; ++i)
		if(!mtable->IsAllocated(zoneid, i)){
			int s = Allocate(zoneid);
			if(s == -1){
				if(SHOW_ERR)
					std::cout << "no block\n";
				return -1;
			}
		}
	return 0;
}

int BlockManager::Read(int zoneid, int offset, int data_size){
	return -1;
}

void MappingTable::show(){
	std::cout << "Mapping Table:\n";
	for(int i = 0; i < NRZONE; ++i){
		for(int j = 0; j < SZZONE; ++j)
			std::cout << mt[i][j] << " ";
		std::cout << "\n";
	}
	std::cout << "\n";
}

void ResetHintTable::show(){
	std::cout << "Reset Hint Table:\n";
	for(int i = 0; i < NRZONE; ++i)
		std::cout << rht[i] << "\n";
	std::cout << "\n";
}

void BlockEraseCountRecord::show(){
	std::cout<< "Block Erase Count Record:\n";
	for(int i = 0; i < NRBLK; ++i){
		std::cout << record[i] << " ";
		if((i + 1) % SZZONE == 0)
			std::cout << "\n";
	}	
	std::cout << "\n";
}
