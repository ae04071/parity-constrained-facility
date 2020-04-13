#ifndef __PCFL_MODEL_H__
#define __PCFL_MODEL_H__

#include "gurobi_c++.h"
#include "ProbData.h"
#include "pcfl_callback.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>

using namespace std;

class PCFLModel : public GRBModel {
private:
	int 		nrFacility, nrClient;
	GRBEnv 		m_env;
	GRBVar 		*m_openVar, **m_assignVar, *m_parityVar;
	
	double		m_timeLimit;
	
public:
	PCFLModel(double _timeLimit);
	
	void constructModel(const ProbData &d);

	void setVariableProperties();
	void run();

	~PCFLModel();

protected:
	GRBVar* makeOpeningVars(const ProbData &d);
	GRBVar** makeAssigningVars(const ProbData &d);
	
	void addConstr_AssignWithinCap(const ProbData &d, const GRBVar *openVar, GRBVar **assignVar);
	void addConstr_AssignOnlyOnce(const ProbData &d, GRBVar **assignVar);
	GRBVar* addConstr_Parity(const ProbData &d, GRBVar *openVar, GRBVar **assignVar);
};

#endif
