#include "ProbData.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

// Constructor
ProbData::ProbData(): nrClient(0), nrFacility(0) {}

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

void ProbData::clearData() {
	nrClient = nrFacility = 0;
	openCost.clear();
	assignCost.clear();
}
