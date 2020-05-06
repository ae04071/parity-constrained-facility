#include "pcfl_utility.h"

ProbData* PCFLUtility::input = nullptr;
PCFLUtility* PCFLUtility::instance = nullptr;

double EPS = 1e-6;
double lt(double a, double b) {
	return a+EPS < b;
}

PCFLUtility::PCFLUtility() {
	input = new ProbData();
}
PCFLUtility::~PCFLUtility() {
	delete input;
}

PCFLUtility& PCFLUtility::getInstance() {
	if(!instance) instance = new PCFLUtility();
	return *instance;
}

ProbData* PCFLUtility::getInput() {
	return input;
}
void PCFLUtility::setInput(ProbData &rhs) {
	*input = rhs;
}
void PCFLUtility::setInput(string filename) {
	input->readDataFromFile(filename);
}

vector<int> PCFLUtility::getShorterClients(int my, int other) {
	vector<int> ret;
	for(int i=0; i<input->nrClient; i++) {
		if(lt(input->assignCost[my][i], input->assignCost[other][i]))
			ret.push_back(i);
	}
	return ret;
}

vector<int> PCFLUtility::getShorterClients(int my, vector<int> others) {
	vector<int> ret;
	for(int j=0; j<input->nrClient; j++) {
		bool flag = true;
		for(auto &other:others) {
			if(lt(input->assignCost[other][j], input->assignCost[my][j])) {
				flag = false;
				break;
			}
		}
		if(flag) ret.push_back(j);
	}
	return ret;
}

vector<int> PCFLUtility::groupFacilities(vector<int> stand) {
	vector<vector<double>> &ass = input->assignCost;
	vector<int> ret(input->nrFacility);

	const double INF = 1e100;
	for(int i=0; i<input->nrFacility; i++) {
		double cost = INF;
		int tar = -1;
		for(int k=0; k<stand.size(); k++) {
			int &f = stand[k];
			if(i == f) {
				tar = k;
				break;
			}

			double mc = INF;
			for(int j=0; j<input->nrClient; j++) if(mc > ass[i][j] + ass[i][f]) {
				mc = ass[i][j] + ass[i][f];
			}
			if(cost > mc) {
				cost = mc;
				tar = k;
			}
		}
		ret[i] = tar;
	}

	return ret;
}
