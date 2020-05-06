#ifndef __PCFL_FLAG_MANAGER_H__
#define __PCFL_FLAG_MANAGER_H__

const int BTW_FACILITY = 1 << 1;
const int LAZY_PARITY_CONSTR = 1 << 2;

const int DEFER_PARITY_CONSTR = 1 << 1;
const int DEFER_DIST_CONSTR = 1 << 2;
const int DEFER_ADJUST = 1 << 3;

const int STATE_UNCONSTRAINED = 1 << 0;
const int STATE_TRACE_LOGS = 1 << 1;

int 		getFlag(int val, int f);
void 		setFlag(int &val, int f);
void 		unsetFlag(int &val, int f);

enum {
	DIST_BASE_CONSTR
};

#endif
