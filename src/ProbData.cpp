#include "ProbData.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

// Constructor
ProbData::ProbData(): nrClient(0), nrFacility(0) {
}

ProbData::ProbData(const ProbData &rhs)
	: nrClient(rhs.nrClient), nrFacility(rhs.nrFacility) {
	openCost = rhs.openCost;
	assignCost = rhs.assignCost;
}
	
// Constructor with file name.
ProbData::ProbData(string fname) {
	readDataFromFile(fname);
}

ProbData& ProbData::operator=(const ProbData &rhs) {
	nrClient = rhs.nrClient;
	nrFacility = rhs.nrFacility;
	openCost = rhs.openCost;
	assignCost = rhs.assignCost;
	return *this;
}

// Read data from the file.
void ProbData::readDataFromFile(string fname) {
	clearData();

	ifstream file;
	file.exceptions(ifstream::failbit | ifstream::badbit);
	istream *in = &std::cin;
	try {
		if(fname != "") {
			file.open(fname);
			in = &file;
		}

		*in >> nrClient >> nrFacility;

		parityConstr.resize(nrFacility);
		for(int i=0; i<nrFacility; i++) *in >> parityConstr[i];

		openCost.resize(nrFacility);
		for(int i=0; i<nrFacility; i++) *in >> openCost[i];

		assignCost.resize(nrFacility);
		for(int i=0; i<nrFacility; i++) {
			assignCost[i].resize(nrClient);
			for(int j=0; j<nrClient; j++) {
				*in >> assignCost[i][j];
			}
		}
	} catch (ifstream::failure e) {
		cerr << "Error while readeing " << fname << endl;
		throw e;
	}
}

void ProbData::clearData() {
	nrClient = nrFacility = 0;
	openCost.clear();
	assignCost.clear();
}


OutData::OutData()
	: cost(0), runtime(0), status(0) {}
	
OutData::OutData(double cost, double runtime, int status)
	: cost(cost), runtime(runtime), status(status) {}

ostream& operator<<(ostream &out, const OutData &data) {
	out << data.cost << endl << data.runtime << endl << data.status;
	return out;
}

