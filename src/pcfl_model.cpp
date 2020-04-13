#include "pcfl_model.h"

PCFLModel::PCFLModel(double _timeLimit=30.0)
	:m_timeLimit(_timeLimit), m_env(true), m_openVar(nullptr), m_assignVar(nullptr), m_parityVar(nullptr), GRBModel(false)
{
}

void PCFLModel::constructModel(const ProbData &d) {
	nrFacility = d.nrFacility;
	nrClient = d.nrClient;
	m_openVar = makeOpeningVars(d);
	m_assignVar = makeAssigningVars(d);
	addConstr_AssignWithinCap(d, m_openVar, m_assignVar);
	addConstr_AssignOnlyOnce(d, m_assignVar);
	m_parityVar = addConstr_Parity(d, m_openVar, m_assignVar);
}

void PCFLModel::setVariableProperties() {
	set(GRB_IntParam_Threads, std::thread::hardware_concurrency());
	set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
	if(m_timeLimit >= 0) set(GRB_DoubleParam_TimeLimit, m_timeLimit);
	set(GRB_DoubleParam_MIPGap, 0);

	GRBCallback *cb = new PCFLCallback(nrFacility, nrClient, m_openVar, m_assignVar, m_parityVar);
}

void PCFLModel::run() {
	optimize();
}

PCFLModel::~PCFLModel() {
	if(m_openVar) 
		delete []m_openVar;

	if(m_parityVar)
		delete []m_parityVar;
		

	if(m_assignVar) {
		for(int i=0;i<nrFacility;i++) delete []m_assignVar[i];
		delete []m_assignVar;
	}
}
	

// make facility and thier objective.
GRBVar* PCFLModel::makeOpeningVars(const ProbData &d) {
	GRBVar* open = addVars(d.nrFacility, GRB_BINARY);
	for(int i=0; i<d.nrFacility; i++) {
		// name of varaible
		ostringstream vname;
		vname << "Open" << i;
		
		// set Object
		open[i].set(GRB_DoubleAttr_Obj, d.openCost[i]);
		open[i].set(GRB_StringAttr_VarName, vname.str());
	}
	return open;
}

// mae client and thier objective.
GRBVar** PCFLModel::makeAssigningVars(const ProbData &d) {
	GRBVar** assign = new GRBVar* [d.nrFacility];
	for(int i=0; i<d.nrFacility; i++) assign[i] = addVars(d.nrClient, GRB_BINARY);
	for(int j=0; j<d.nrClient; j++) {
		for(int i=0; i<d.nrFacility; i++) {
			// name of variable
			ostringstream vname;
			vname << "Assign Client " << j << " to " << i;
			
			// set objective
			assign[i][j].set(GRB_DoubleAttr_Obj, d.assignCost[i][j]);
			assign[i][j].set(GRB_StringAttr_VarName, vname.str());
		}
	}
	return assign;
}

void PCFLModel::addConstr_AssignWithinCap(const ProbData &d, const GRBVar *open, GRBVar **assign) {
	for(int i=0; i<d.nrFacility; i++) {
		for(int j=0; j<d.nrClient; j++) {
			ostringstream cname;
			cname << "Within Cap" << j << " to " << i;
			addConstr(assign[i][j] <= open[i], cname.str());
		}
	}
}
void PCFLModel::addConstr_AssignOnlyOnce(const ProbData &d,	GRBVar **assign) {
	for(int j=0; j<d.nrClient; j++) {
		GRBLinExpr sum = 0;
		for(int i=0; i<d.nrFacility; i++) {
			sum += assign[i][j];
		}
		ostringstream cname;
		cname << "Within One " << j;
		addConstr(sum == 1, cname.str());
	}
}
GRBVar* PCFLModel::addConstr_Parity(const ProbData &d, GRBVar *open, GRBVar **assign) {
	GRBVar *cap = addVars(d.nrFacility, GRB_INTEGER);
	for(int i=0; i<d.nrFacility; i++) {
		cap[i].set(GRB_DoubleAttr_Obj, 0);
		cap[i].set(GRB_DoubleAttr_LB, 0);
	}

	for(int i=0; i<d.nrFacility; i++) {
		if(d.parityConstr[i]!=0) {
			GRBLinExpr sum = 0;
			for(int j=0; j<d.nrClient; j++) {
				sum += assign[i][j];
			}
			ostringstream cname;
			cname << "Parity constraint for facility " << i;
			if(d.parityConstr[i]==1) {
				addConstr(sum == 2 * cap[i] + open[i], cname.str());
			} else {
				addConstr(sum == 2 * cap[i], cname.str());
			}
		}
	}
	return cap;
}
