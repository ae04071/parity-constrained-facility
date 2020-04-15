#include "pcfl_callback.h"
#include <cmath>
using namespace std;

const double EPS = 1e-6;
double eq(double a, double b) { return fabs(a-b) < EPS; }

static string mapping_callback_state(int where) {
	switch(where) {
		case GRB_CB_POLLING:
			return "PULLING";
		case GRB_CB_PRESOLVE:
			return "PRESOLVE";
		case GRB_CB_SIMPLEX:
			return "SIMPLEX";
		case GRB_CB_MIP:
			return "MIP";
		case GRB_CB_MIPSOL:
			return "MIPSOL";
		case GRB_CB_MIPNODE:
			return "MIPNODE";
		case GRB_CB_BARRIER:
			return "BARRIER";
		default:
			return "NON OF THEM";
	}
}

PCFLCallback::PCFLCallback()
	: openVar(nullptr), assignVar(nullptr), parityVar(nullptr) {}
PCFLCallback::PCFLCallback(int _nrFacility, int _nrClient, 
		GRBVar *_openVar, GRBVar **_assignVar, GRBVar *_parityVar)
	: nrFacility(_nrFacility), nrClient(_nrClient),
	openVar(_openVar), assignVar(_assignVar), parityVar(_parityVar),
	assignOnceFlag(false), assignOnceCheck(nullptr), assignOnceConstr(nullptr) {}

PCFLCallback::~PCFLCallback() {
	if(assignOnceFlag) {
		delete []assignOnceCheck;
	}
}

void PCFLCallback::init() {
	if(assignOnceFlag) {
		assignOnceCheck = new bool[nrClient];
		fill(assignOnceCheck, assignOnceCheck+nrClient, false);
	}
}

void PCFLCallback::setAssignOnceConstr(GRBLinExpr *expr) {
	if(expr) {
		assignOnceFlag = true;
		assignOnceConstr = expr;
	} else {
		assignOnceFlag = false;
		assignOnceConstr = nullptr;
	}
}

void PCFLCallback::callback() {
	/*
	if(where!=0 && where != 6 && where != 1)
		cerr << "Current callback state: " << mapping_callback_state(where) << ' ' << where << endl;
	*/
	try {
		if (where == GRB_CB_POLLING) {
			// Ignore polling callback
		} else if (where == GRB_CB_PRESOLVE) {
			// Presolve callback
		} else if (where == GRB_CB_SIMPLEX) {
			// Simplex callback
		}else if (where == GRB_CB_MIP) {
			// General MIP callback
		} else if (where == GRB_CB_MIPSOL) {
			// MIP solution callback
			if(assignOnceFlag) {
				// too many data must be loaded.
				double **x = new double*[nrFacility];
				for(int i=0; i<nrFacility; i++)
					x[i] = getSolution(assignVar[i], nrClient);

				// some cache miss problem 
				for(int j=0; j<nrClient; j++) if(!assignOnceCheck[j]) {
					double sum=0;
					for(int i=0; i<nrFacility; i++) sum += x[i][j];
					// tolerance needed 
					if(sum > 1.01) {
						addLazy(assignOnceConstr[j] <= 1);
						assignOnceCheck[j] = true;
					}
				}

				for(int i=0; i<nrFacility; i++)
					delete []x[i];
				delete []x;
			}
			/*
			cout << "######################### MIPSOL INFO #####################################" << endl;
			double *x = getSolution(openVar, nrFacility);
			for(int i=0; i<nrFacility; i++) if(x[i] > 0.1) {
				cout << "Facility " << i << " has been opened" << endl;
				cout << "The Bound - LB: " << openVar[i].get(GRB_DoubleAttr_LB) << " RB: " << openVar[i].get(GRB_DoubleAttr_UB) << endl;
				cout << endl;
			}
			cout << endl;
			*/
			/*
			cout << "Solution: " << getDoubleInfo(GRB_CB_MIPSOL_OBJ) << endl;
			cout << "Current # of nodes: " << getDoubleInfo(GRB_CB_MIPSOL_NODCNT) << endl;
			cout << "Current # of sols: " << getIntInfo(GRB_CB_MIPSOL_SOLCNT) << endl;
			cout << endl;
			*/
		} else if (where == GRB_CB_MIPNODE) {
			// MIP node callback
			/*
			cout << "######################### MIPNODE INFO #####################################" << endl;
			double *x = getSolution(openVar, nrFacility);
			for(int i=0; i<nrFacility; i++) {
				double lb = openVar[i].get(GRB_DoubleAttr_LB);
				double ub = openVar[i].get(GRB_DoubleAttr_UB);
				if(lb > 0.1 || ub < 0.9)
					cout << "Bound was Changed: Facility " << i << ", (" << lb << ", " << ub << ")" << endl;
			}
			for(int i=0; i<nrFacility; i++) {
				for(int j=0; j<nrClient; j++) {
					double lb = assignVar[i][j].get(GRB_DoubleAttr_LB);
					double ub = assignVar[i][j].get(GRB_DoubleAttr_UB);
					if(lb > 0.1 || ub < 0.9)
						cout << "Bound was Changed: Assign " << j << " to " << i << ", (" << lb << ", " << ub << ")" << endl;
				}
			}
			for(int i=0; i<nrFacility; i++) {
				double lb = parityVar[i].get(GRB_DoubleAttr_LB);
				double ub = parityVar[i].get(GRB_DoubleAttr_UB);
				if(lb > 0.1 || ub < 0.9)
					cout << "Bound was Changed: Facility " << i << ", (" << lb << ", " << ub << ")" << endl;
			}
			cout << endl;
			*/
			
			/*
			cout << "Start Compare" << endl;
			cout << fixed;
			cout.precision(1);
			for(int i=0; i<nrFacility; i++) {
				double *y = getNodeRel(assignVar[i], nrClient);
				if(x[i]) {
					for(int j=0; j<nrClient; j++) if(!eq(x[i][j], y[j]))
						cout << "DIFF " << i << ' ' << j << " " << fabs(x[i][j]) << ' ' << fabs(y[j]) << endl;;
					delete []x[i];
				}
				x[i] = y;
			}

			cout << "# of cuts: " << getIntInfo(GRB_CB_MIPNODE_STATUS) << endl;
			*/
		} else if (where == GRB_CB_BARRIER) {
			// Barrier callback
		} else if (where == GRB_CB_MESSAGE) {
			// Message callback
		}
	} catch (GRBException e) {
		cout << "Error number: " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
	} catch (...) {
		cout << "Error during callback" << endl;
	}
}
