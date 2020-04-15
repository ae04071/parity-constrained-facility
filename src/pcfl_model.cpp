#include "pcfl_model.h"

/* Model Definition */
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
	setCallback(cb);
}

void PCFLModel::run() {
	if(PCFLModelSetter::getInstance().getAssignLazy()) {
		PCFLCallback *cb = new PCFLCallback(nrFacility, nrClient, m_openVar, m_assignVar, m_parityVar);
		cb->setAssignOnceConstr(m_assignConstr);
		cb->init();
		setCallback(cb);

		set(GRB_IntParam_LazyConstraints, 1);
	}

	PCFLModelSetter::getInstance().setModelProp(*this);

	set(GRB_DoubleParam_MIPGap, 0);

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

	if(m_assignConstr)
		delete []m_assignConstr;
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
	m_assignConstr = new GRBLinExpr[d.nrClient];
	for(int j=0; j<d.nrClient; j++) {
		GRBLinExpr &sum = m_assignConstr[j];
		for(int i=0; i<d.nrFacility; i++) {
			sum += assign[i][j];
		}
		ostringstream cname;
		cname << "Within One " << j;

		if(PCFLModelSetter::getInstance().getAssignLazy()) 
			addConstr(sum >= 1, cname.str());
		else
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

/* Setter definition */
PCFLModelSetter *PCFLModelSetter::instance = nullptr;
int PCFLModelSetter::m_iOpenPrior = 0;
int PCFLModelSetter::m_iAssignPrior = 0;
int PCFLModelSetter::m_iParityPrior = 0;

bool PCFLModelSetter::m_bAssignLazy = false;

PCFLModelSetter::PCFLModelSetter()
{
}

PCFLModelSetter& PCFLModelSetter::getInstance() {
	if(!instance)
		instance = new PCFLModelSetter();

	return *instance;
}

void PCFLModelSetter::setModelProp(PCFLModel &model) {
	do_openPrior(model);
	do_assignPrior(model);
	do_parityPrior(model);
}

void PCFLModelSetter::do_openPrior(PCFLModel &model) {
	if(!m_iOpenPrior) return;

	for(int i=0; i<model.nrFacility; i++) {
		model.m_openVar[i].set(GRB_IntAttr_BranchPriority, m_iOpenPrior);
	}
}
void PCFLModelSetter::do_assignPrior(PCFLModel &model) {
	if(!m_iAssignPrior) return;

	for(int i=0; i<model.nrFacility; i++) {
		for(int j=0; j<model.nrClient; j++) {
			model.m_assignVar[i][j].set(GRB_IntAttr_BranchPriority, m_iAssignPrior);
		}
	}
}
void PCFLModelSetter::do_parityPrior(PCFLModel &model) {
	if(!m_iParityPrior) return;

	for(int i=0; i<model.nrFacility; i++) {
		model.m_parityVar[i].set(GRB_IntAttr_BranchPriority, m_iParityPrior);
	}
}

void PCFLModelSetter::setOpenPrior(int _val) {
	m_iOpenPrior = _val;
}
int PCFLModelSetter::getOpenPrior() {
	return m_iOpenPrior;
}
void PCFLModelSetter::setAssignPrior(int _val) {
	m_iAssignPrior = _val;
}
int PCFLModelSetter::getAssignPrior() {
	return m_iAssignPrior;
}
void PCFLModelSetter::setParityPrior(int _val) {
	m_iParityPrior = _val;
}
int PCFLModelSetter::getParityPrior() {
	return m_iParityPrior;
}

void PCFLModelSetter::setAssignLazy(bool _val) {
	m_bAssignLazy = _val;
}
bool PCFLModelSetter::getAssignLazy() {
	return m_bAssignLazy;
}
