#include "parameter.h"
#include <iostream>

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
	void Show();
};

class BlockEraseCountRecord{
private:
	int *record;
public:
	BlockEraseCountRecord();
	~BlockEraseCountRecord();
	void AddEC(int blkid);
	int GetEC(int blkid);
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
private:
	FreeBlockList *fblist;
	BlockEraseCountRecord *blkec;
	MappingTable *mtable;
	ResetHintTable *rhtable;

	int EC_max, EC_min, EC_min_free;
	
	int Allocate(int zoneid);
	int Erase(int blkid);
public:
	BlockManager();
	~BlockManager();
	int Append(int zoneid, int offset, int data_size);
	int Read(int zoneid, int offset, int data_size);
	int Reset(int zoneid);

	void show(){
		mtable->show();
		rhtable->show();
		blkec->show();
	}
};
