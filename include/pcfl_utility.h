#ifndef __PCFL_UTILITY_H__
#define __PCFL_UTILITY_H__

#include "ProbData.h"

#include <string>

class PCFLUtility {
private:
	static ProbData *input;
	static PCFLUtility *instance;

private:
	PCFLUtility();

public:
	~PCFLUtility();

	static PCFLUtility& getInstance();
	static ProbData* getInput();
	static void setInput(ProbData&);
	static void setInput(string);

	static vector<int> getShorterClients(int, int);
	vector<int> getShorterClients(int, vector<int>);
	vector<int> groupFacilities(vector<int>);
};

#endif
