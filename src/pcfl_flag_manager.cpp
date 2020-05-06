#include "pcfl_flag_manager.h"

int getFlag(int val, int f) {
	return val&f;
}
void setFlag(int &val, int f) {
	val |= f;
}
void unsetFlag(int &val, int f) {
	if(getFlag(val, f)) val ^= f;
}
