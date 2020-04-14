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

void prefetch(variables_map &vm) {
	//cout << "open priority: " << vm["open-prior"].as<int>() << endl;
	
	PCFLModelSetter::getInstance().setOpenPrior(vm["open-prior"].as<int>());
	PCFLModelSetter::getInstance().setAssignPrior(vm["assign-prior"].as<int>());
}

OutData solve(const ProbData &d, double tlimit=GRB_INFINITY) {
	try { 
		PCFLModel model(tlimit);
		model.constructModel(d);
		model.run();

		cout << fixed;
		cout.precision(6);
		cout << model.get(GRB_DoubleAttr_ObjVal) << endl;

		int model_status = model.get(GRB_IntAttr_Status);
		double cost = model.get(GRB_DoubleAttr_ObjVal);
		double runtime = model.get(GRB_DoubleAttr_Runtime);

		/*
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
		options_description desc{"Options"};
		desc.add_options()
			("help,h", "Help screen")
			("open-prior", value<int>()->default_value(0), "Assign branch priority of open variable")
			("assign-prior", value<int>()->default_value(0), "Assign branch priority of assign variable")
			/*
			("input", value<string>()->required(), "Set Input file")
			("output", value<string>(), "Set output")
			("TimeLimit", value<double>()->default_value(-1.0), "Time Limit(-1 is default, INF)")
			*/
		;	

		variables_map vm;
		store(parse_command_line(argc, argv, desc), vm);

		if(vm.count("help")) {
			cout << desc << endl;
		}
		notify(vm);

		ProbData d;
		prefetch(vm);
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
