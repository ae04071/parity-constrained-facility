/* Copyright 2020, Gurobi Optimization, LLC */

#include "gurobi_c++.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
using namespace std;

class ProbData {
public:
	unsigned long nrClient, nrFacility;				// Client 개수, Facility 개수
	vector<int> parityConstr;
	vector<double> openCost;
	vector<vector<double>> assignCost;				// Facility x Client metric

public:
	// Constructor
	ProbData(): nrClient(0), nrFacility(0) {}

	explicit ProbData(const ProbData &rhs)
		: nrClient(rhs.nrClient), nrFacility(rhs.nrFacility) {
		openCost = rhs.openCost;
		assignCost = rhs.assignCost;
	}
	
	// Constructor with file name.
	ProbData(string fname) {
		readDataFromFile(fname);
	}

	ProbData& operator=(const ProbData &rhs) {
		nrClient = rhs.nrClient;
		nrFacility = rhs.nrFacility;
		openCost = rhs.openCost;
		assignCost = rhs.assignCost;
		return *this;
	}

	// Read data from the file.
	void readDataFromFile(string fname) {
		clearData();

		ifstream file;
		file.exceptions(ifstream::failbit | ifstream::badbit);
		try {
			file.open(fname);

			file >> nrClient >> nrFacility;

			parityConstr.resize(nrFacility);
			for(int i=0; i<nrFacility; i++) file >> parityConstr[i];

			openCost.resize(nrFacility);
			for(int i=0; i<nrFacility; i++) file >> openCost[i];

			assignCost.resize(nrFacility);
			for(int i=0; i<nrFacility; i++) {
				assignCost[i].resize(nrClient);
				for(int j=0; j<nrClient; j++) {
					file >> assignCost[i][j];
				}
			}
			cout << parityConstr[163] << ' ' << parityConstr[205] << endl;
			cout << assignCost[1][56] << ' ' << assignCost[1][163] << endl;
			cout << assignCost[9][205] << ' ' << assignCost[9][56] << endl;
			cout << assignCost[133][205] << ' ' << assignCost[133][163] << endl;
		} catch (ifstream::failure e) {
			cerr << "Error while readeing " << fname << endl;
			throw e;
		}
	}

	void clearData() {
		nrClient = nrFacility = 0;
		openCost.clear();
		assignCost.clear();
	}
};

// make facility and thier objective.
GRBVar* makeOpeningVars(const ProbData &d, GRBModel &model) {
	GRBVar* open = model.addVars(d.nrFacility, GRB_BINARY);
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
GRBVar** makeAssigningVars(const ProbData &d, GRBModel &model) {
	GRBVar** assign = new GRBVar* [d.nrFacility];
	for(int i=0; i<d.nrFacility; i++) assign[i] = model.addVars(d.nrClient, GRB_BINARY);
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

void addConstr_AssignWithinCap(const ProbData &d, GRBModel &model, 
		const GRBVar *open, GRBVar **assign) {
	for(int i=0; i<d.nrFacility; i++) {
		for(int j=0; j<d.nrClient; j++) {
			ostringstream cname;
			cname << "Within Cap" << j << " to " << i;
			model.addConstr(assign[i][j] <= open[i], cname.str());
		}
	}
}
void addConstr_AssignOnlyOnce(const ProbData &d, GRBModel &model,
		GRBVar **assign) {
	for(int j=0; j<d.nrClient; j++) {
		GRBLinExpr sum = 0;
		for(int i=0; i<d.nrFacility; i++) {
			sum += assign[i][j];
		}
		ostringstream cname;
		cname << "Within One " << j;
		model.addConstr(sum == 1, cname.str());
	}
}
GRBVar* addConstr_Parity(const ProbData &d, GRBModel &model,
		GRBVar *open, GRBVar **assign) {
	GRBVar *cap = model.addVars(d.nrFacility, GRB_INTEGER);
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
				model.addConstr(sum == 2 * cap[i] + open[i], cname.str());
			} else {
				model.addConstr(sum == 2 * cap[i], cname.str());
			}
		}
	}
	return cap;
}

double solve(const ProbData &d) {
	try { 
		// Create an environment
		GRBEnv env = GRBEnv(true);
		env.set("LogFile", "unconstrait_facility.log");
		env.start();

		// Create an empty model
		GRBModel model = GRBModel(env);
		model.set(GRB_StringAttr_ModelName, "unconstraint facility");
		
		GRBVar *openVar = makeOpeningVars(d, model);
		GRBVar **assignVar = makeAssigningVars(d, model);
		GRBVar *capVar;
		addConstr_AssignWithinCap(d, model, openVar, assignVar);
		addConstr_AssignOnlyOnce(d, model, assignVar);
		capVar = addConstr_Parity(d, model, openVar, assignVar);

		cout << "Construction is done " << endl;

		model.set(GRB_IntParam_Threads, std::thread::hardware_concurrency());
		model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
		model.optimize();

		cout.precision(10);
		cout << "\nCost: " << model.get(GRB_DoubleAttr_ObjVal) << endl;
		double s = 0;

		for(int i=0; i<d.nrFacility; i++) {
			if(openVar[i].get(GRB_DoubleAttr_X) > 0.99) {
				printf("%d, %.6lf\n", i, d.openCost[i]);
				s += d.openCost[i];
			}
		}
		for(int j=0; j<d.nrClient; j++) {
			for(int i=0; i<d.nrFacility; i++) if(assignVar[i][j].get(GRB_DoubleAttr_X) > 0.9) {
				printf("%d, %d, %.6lf\n", j, i, d.assignCost[i][j]);
				s += d.assignCost[i][j];
			}
		}
		cout << "TOTLA: " << s << endl;

		return model.get(GRB_DoubleAttr_ObjVal);
	} catch (GRBException e) {
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
		throw e;
	}
	catch (...) {
		cout << "Exception during optimization" << endl;
	}
}

void writeAns(string inName, double ans) {
	string outName = inName;
	int idx = outName.find("input");
	outName.erase(outName.begin()+idx, outName.begin()+idx+5);
	outName.insert(idx, "output");

	cout << outName << endl;;

	ofstream file;
	file.open(outName);
	file.precision(10);
	file << ans;
}

int main(int argc, char *argv[]) {
	// Get a file name
	if(argc < 2) { 
		cout << "Enter a filename" << endl;
		return 0;
	}
	string fname = argv[1];
	cout << "File name is " << fname << endl;
	
	ProbData d(fname);
	try { 
		double ans = solve(d);
		writeAns(fname, ans);
	} catch (GRBException e) {
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
		throw e;
	}
	catch (...) {
		cout << "Exception during optimization" << endl;
	}
		
	return 0;
}
