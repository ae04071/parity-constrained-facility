#ifndef __PCFL_CALLBACK_H__
#define __PCFL_CALLBACK_H__

#include "gurobi_c++.h"
#include <iostream>

class PCFLCallback: public GRBCallback {
public:
	int				nrFacility, nrClient;
	GRBVar*			openVar;
	GRBVar**		assignVar;
	GRBVar*			parityVar;

public:
	PCFLCallback();
	PCFLCallback(int _nrFacility, int _nrClient,
			GRBVar *_openVar, GRBVar **_assignVar, GRBVar *_parityVar);

protected:
	void callback();
};

#endif
