/* Copyright 2020, Gurobi Optimization, LLC */

#include <boost/program_options.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <set>

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
	PCFLModelSetter::getInstance().setParityPrior(vm["parity-prior"].as<int>());
	PCFLModelSetter::getInstance().setAssignLazy(vm["assign-once-lazy"].as<int>() == 0 ? false : true);
	PCFLModelSetter::getInstance().setLazyConstr(vm["lazy-btw-fac"].as<int>() == 0 ? 0 : BTW_FACILITY);
	PCFLModelSetter::getInstance().setDeferConstr(vm["defer-parity-constr"].as<int>() == 0 ? 0 : DEFER_PARITY_CONSTR);
	PCFLModelSetter::getInstance().setLazyConstr(vm["lazy-parity-constr"].as<int>() == 0 ? 0 : LAZY_PARITY_CONSTR);
	PCFLModelSetter::getInstance().addConstr(DIST_BASE_CONSTR, vm["add-dist-assign"].as<int>());
	PCFLModelSetter::getInstance().setState(vm["state-unconstr"].as<int>() == 0 ? 0 : STATE_UNCONSTRAINED);
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
		vector<pair<int, set<int>>> arr;
		for(int i=0; i<d.nrFacility; i++) {
			double cost = model.m_openVar[i].get(GRB_DoubleAttr_X);
			if(cost > 0.5) {
				set<int> tr;
				for(int j=0; j<d.nrClient;j++) {
					double c = model.m_assignVar[i][j].get(GRB_DoubleAttr_X);
					if(c > 0.5) tr.insert(j);
				}
				arr.push_back({i, tr});
			}
		}
		cout << endl;

		for(int i=0;i<arr.size();i++) {
			for(int j=i+1;j<arr.size();j++) {
				for(auto &idx: arr[i].second) {
					if(arr[j].second.find(idx) != arr[j].second.end()) {
						cout << "Conflict " << arr[i].first << ' ' << arr[j].first << ' ' << idx << endl;
					}
				}
			}
		}

		for(int i=0; i<d.nrFacility; i++) {
			double sum=0;
			for(int j=0; j<d.nrClient; j++) {
				double cost = model.m_assignVar[i][j].get(GRB_DoubleAttr_X);
				if(cost > 0.5) {
					sum += 1;
				}
			}
			cout << i << ' ' << model.m_openVar[i].get(GRB_DoubleAttr_X) << ' ' << d.parityConstr[i] << ' ' << sum << endl;
		}
		*/

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
			("trace-solution", value<int>()->default_value(0), "Trace solution of IP")
			// Issue #2
			("open-prior", value<int>()->default_value(0), "Assign branch priority of open variable")
			("assign-prior", value<int>()->default_value(0), "Assign branch priority of assign variable")
			("parity-prior", value<int>()->default_value(0), "Assign branch priority of parity variable")
			// Issue #3
			("assign-once-lazy", value<int>()->default_value(0), "Assign branch priority of parity variable")
			// Issue #4
			("lazy-btw-fac", value<int>()->default_value(0), "Set lazy condtion - between two facility")

			// Issue #5
			("defer-parity-constr", value<int>()->default_value(0), "Presolve origin problem(facility location) first, and then, solve our problem")
			("lazy-parity-constr", value<int>()->default_value(0), "Make parity constraint lazily.")

			// Issue #6
			("add-dist-assign", value<int>()->default_value(0), "Make additional constraint: distnace-based constraint")

			("state-unconstr", value<int>()->default_value(0), "Solve unconstrained PCFL problem")
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

		PCFLUtility::getInstance().setInput("");
		prefetch(vm);
		OutData ans = solve(*(PCFLUtility::getInstance().getInput()));
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
