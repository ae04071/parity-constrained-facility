#ifndef __PCFL_MODEL_H__
#define __PCFL_MODEL_H__

#include "gurobi_c++.h"
#include "ProbData.h"
#include "pcfl_callback.h"
#include "pcfl_flag_manager.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>

using namespace std;

/* Model Class */
class PCFLModel : public GRBModel {
private:
	GRBEnv 		m_env;
	
	double		m_timeLimit;

public:
	// base problem data
	int 		nrFacility, nrClient;
	GRBVar 		*m_openVar, **m_assignVar, *m_parityVar;

	// some extra constarint
	GRBLinExpr	*m_assignConstr;
	GRBTempConstr *m_parityConstr;

public:
	PCFLModel(double _timeLimit);
	PCFLModel(const GRBEnv &env, double _timeLimit);
	
	void constructModel(const ProbData &d);

	void setVariableProperties();
	void run();

	~PCFLModel();

protected:
	GRBVar* makeOpeningVars(const ProbData &d);
	GRBVar** makeAssigningVars(const ProbData &d);
	
	void addConstr_AssignWithinCap(const ProbData &d, const GRBVar *openVar, GRBVar **assignVar);
	void addConstr_AssignOnlyOnce(const ProbData &d, GRBVar **assignVar);

	GRBVar* makeConstr_Parity(const ProbData &d, GRBVar *openVar, GRBVar **assignVar);
	void addConstr_Parity();

	void addConstr_DistAssign(const ProbData&, GRBVar*, GRBVar**);
};

/* Setter Class */
class PCFLModelSetter {
private:
	static PCFLModelSetter *instance;

	static int m_iOpenPrior;
	static int m_iAssignPrior;
	static int m_iParityPrior;

	static bool m_bAssignLazy;

	static int m_iLazyConstr;
	static int m_iDeferConstr;
	static int m_iState;

public:
	static int m_iDistAssignRatio;

	static string m_sTraceDir;

private:
	PCFLModelSetter();

public:
	static PCFLModelSetter& getInstance();

	static void setModelProp(PCFLModel&);

	// do setting
	static void do_openPrior(PCFLModel&);
	static void do_assignPrior(PCFLModel&);
	static void do_parityPrior(PCFLModel&);

	// set or get flags
	static void 		setOpenPrior(int);
	static int 			getOpenPrior();
	static void 		setAssignPrior(int);
	static int			getAssignPrior();
	static void			setParityPrior(int);
	static int			getParityPrior();

	static void			setAssignLazy(bool);
	static bool			getAssignLazy();

	static void			setLazyConstr(int);
	static void			unsetLazyConstr(int);
	static int			getLazyConstr();

	static void			setDeferConstr(int);
	static void			unsetDeferConstr(int);
	static int			getDeferConstr();
	
	static void			setState(int);
	static void			unsetState(int);
	static int			getState();

	static void			addConstr(int, int);

	static void			setTraceDir(string);
};

#endif
