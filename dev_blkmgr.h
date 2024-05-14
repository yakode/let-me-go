#include "parameter.h"
#include <iostream>
#include <iomanip>
#include <set>
#include <algorithm>

class LLNode;
class FBLNode;
class FreeBlockList;
class BlockEraseCountRecord;
class MappingTable;
class ResetHintTable;
class BlockManager;

class LLNode{
	friend class FBLNode;
private:
	int blkid;
	LLNode *next;
public:
	LLNode(int blkid_): blkid(blkid_), next(nullptr){}
	LLNode(int blkid_, LLNode *next_): blkid(blkid_), next(next_){}
	~LLNode(){}
	int GetBlkid();
};

class FBLNode{
	friend class FreeBlockList;
private:
	int key;
	int nr;
	LLNode *head;
	FBLNode *left, *right;
	FBLNode *p;
	bool color; // true black, false red
public:
	FBLNode(int key_): key(key_), nr(0), head(nullptr), left(nullptr), right(nullptr), p(nullptr), color(false){}
	FBLNode(int key_, FBLNode *p_): key(key_), nr(0), head(nullptr), left(nullptr), right(nullptr), p(p_), color(false){}
	FBLNode(int key_, FBLNode *p_, FBLNode *nil_): key(key_), nr(0), head(nullptr), left(nil_), right(nil_), p(p_), color(false){}
	~FBLNode();
	void AddLLNode(int blkid);
	int DeleteLLHead();
};

class FreeBlockList{
private:
	FBLNode *root_;
	FBLNode *min_;
	FBLNode *nil_;
	void LeftRotate(FBLNode *x);
	void RightRotate(FBLNode *x);
	void RBTransplant(FBLNode *u, FBLNode *v);
	void RBDeleteFixup(FBLNode *x);
	FBLNode* TreeMinimum(FBLNode *x);
	void RBInsertFixup(FBLNode *z);
public:
	FreeBlockList();
	~FreeBlockList();
	int Pop();
	void Insert(int blkid, int ec);
	int GetMinEC();
	void show(FBLNode*root);
	void show(){show(root_);}
};

class BlockEraseCountRecord{
private:
	int *record;
public:
	BlockEraseCountRecord();
	~BlockEraseCountRecord();
	void AddEC(int blkid);
	int GetEC(int blkid);
	int GetECMin();
	void Summary();
	void show();
};

class MappingTable{
private:
	int **mt;
	int *allocated;
public:
	MappingTable();
	~MappingTable();
	void AddBlockToZone(int zoneid, int blkid);
	bool IsAllocated(int zoneid, int idx);
	int GetSize(int zoneid);
	int GetBlk(int zoneid, int idx);
	void ResetMT(int zoneid);
	void show();
};

class ResetHintTable{
private:
	int *rht;
public:
	ResetHintTable();
	void SetResetHint(int zoneid, int rh);
	int GetResetHint(int zoneid);
	int GetMinRH();
	void show();
};


class BlockManager{
public:
	virtual int Append(int zoneid, int offset, int data_size) = 0;
	virtual int Read(int zoneid, int offset, int data_size) = 0;
	virtual int Reset(int zoneid) = 0;
	virtual int GetECMin() = 0;
	virtual int GetECMax() = 0;
	virtual int GetECMinFree() = 0;
	virtual int GetResetHint(int zoneid) = 0;

	virtual void show() = 0;
};

class BlockManagerDynamic: public BlockManager{
private:
	FreeBlockList *fblist;
	BlockEraseCountRecord *blkec;
	MappingTable *mtable;
	ResetHintTable *rhtable;

	int EC_max, EC_min, EC_min_free;
	
	int Allocate(int zoneid);
	int Erase(int blkid);
public:
	BlockManagerDynamic();
	~BlockManagerDynamic();
	int Append(int zoneid, int offset, int data_size);
	int Read(int zoneid, int offset, int data_size);
	int Reset(int zoneid);
	int GetECMin();
	int GetECMax();
	int GetECMinFree();
	int GetResetHint(int zoneid);

	void show(){
		// fblist->show();
		// std::cout << "\n\n";
		// mtable->show();
		// rhtable->show();
		// blkec->show();
		blkec->Summary();
		std::cout << "EC_max: " << EC_max << ", EC_min: " << EC_min << "\n";
	}
};

class BlockManagerStatic: public BlockManager{
private:
	BlockEraseCountRecord *blkec;

	int EC_max, EC_min;
	
	int Erase(int blkid);
public:
	BlockManagerStatic();
	~BlockManagerStatic();
	int Append(int zoneid, int offset, int data_size){return -1;}
	int Read(int zoneid, int offset, int data_size){return -1;}
	int Reset(int zoneid);
	int GetECMin();
	int GetECMax();

	int GetECMinFree(){return -1;}
	int GetResetHint(int zoneid){return -1;}

	void show(){
		blkec->Summary();
		std::cout << "EC_max: " << EC_max << ", EC_min: " << EC_min << "\n";
	}
};
