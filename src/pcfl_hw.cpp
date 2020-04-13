/* Copyright 2020, Gurobi Optimization, LLC */

#include <boost/program_options.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>

#include "gurobi_c++.h"
#include "ProbData.h"
#include "pcfl_callback.h"
#include "pcfl_model.h"

using namespace std;
using namespace boost::program_options;

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

OutData solve(const ProbData &d, double tlimit=GRB_INFINITY) {
	try { 
		PCFLModel model(tlimit);
		model.constructModel(d);
		model.run();

		/*
		int model_status = model.get(GRB_IntAttr_Status);
		double cost = model.get(GRB_DoubleAttr_ObjVal);
		double runtime = model.get(GRB_DoubleAttr_Runtime);

		cout << fixed;
		cout.precision(6);
		cout << "\nCost: " << cost << endl;
		cout << "Runtime: " << runtime << endl;
		cout << "Status: " << model_status << endl;
		*/

		//return {cost, runtime, model_status};
		return {0, 0, 0};
	} catch (GRBException e) {
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
		throw e;
	}
	catch (...) {
		cout << "Exception during optimization" << endl;
	}
}

void writeAns(string outName, OutData ans) {
	ofstream file;
	file.open(outName);
	file << fixed;
	file.precision(6);
	file << ans;
}

int main(int argc, char *argv[]) {
	try {
		/*
		options_description desc{"Options"};
		desc.add_options()
			("help,h", "Help screen")
			("input", value<string>()->required(), "Set Input file")
			("output", value<string>(), "Set output")
			("TimeLimit", value<double>()->default_value(-1.0), "Time Limit(-1 is default, INF)");

		variables_map vm;
		store(parse_command_line(argc, argv, desc), vm);

		if(vm.count("help")) {
			cout << desc << endl;
		}
		notify(vm);
		*/

		ProbData d;
		OutData ans = solve(d);
		/*
		if(vm.count("output")) {
			string outName = vm["output"].as<string>();
			writeAns(outName, ans);
		}
		*/
	} catch (GRBException e) {
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
		throw e;
	} catch (const error &ex) {
		std::cerr << ex.what() << '\n';
	} catch (...) {
		cout << "Exception during optimization" << endl;
	}
		
	return 0;
}
