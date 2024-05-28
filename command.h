#include "parameter.h"

class WaitTime{
private:
	int64_t *Te;
	double *Tc;
	int n;
	int head;
	int64_t total;
public:
	WaitTime(int n);
	~WaitTime();
	int64_t GetWaitTime();
	void PutWaitTime(int64_t, double);
};
