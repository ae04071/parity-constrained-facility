		
#ifndef __PCFL_CALLBACK_H__
#define __PCFL_CALLBACK_H__

#include "gurobi_c++.h"
#include "pcfl_flag_manager.h"
#include "pcfl_utility.h"

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

class PCFLCallback: public GRBCallback {
public:
	int				nrFacility, nrClient;
	GRBVar*			openVar;
	GRBVar**		assignVar;
	GRBVar*			parityVar;

	bool			assignOnceFlag;
	bool*			assignOnceCheck;
	GRBLinExpr*		assignOnceConstr;

	bool			**btwFacilityCheck;
	bool			*parityConstrCheck;
	GRBTempConstr 	*parityExpr;

	int				lazyConstr;

public:
	PCFLCallback();
	PCFLCallback(int _nrFacility, int _nrClient,
			GRBVar *_openVar, GRBVar **_assignVar, GRBVar *_parityVar);

	~PCFLCallback();

	void init();

	// Linear expression of sum(x_ij) == 1
	void setAssignOnceConstr(GRBLinExpr*);
	void setParityExpr(GRBTempConstr*);

	void setLazyConstr(int);

protected:
	void callback();
};

#endif
