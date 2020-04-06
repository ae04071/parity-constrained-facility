#ifndef __PROB_DATA_H__
#define __PROB_DATA_H__

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

class ProbData {
public:
	unsigned long nrClient, nrFacility;				// Client 개수, Facility 개수
	vector<int> parityConstr;
	vector<double> openCost;
	vector<vector<double>> assignCost;				// Facility x Client metric

public:
	// Constructor
	ProbData();

	explicit ProbData(const ProbData &rhs);
	
	// Constructor with file name.
	ProbData(string fname);

	ProbData& operator=(const ProbData &rhs);

	// Read data from the file.
	void readDataFromFile(string fname);

	void clearData();
};

class OutData {
public:
	double cost, runtime;
	int status;
	
public:
	OutData();
	OutData(double cost, double runtime, int status);

	friend ostream& operator<<(ostream &out, const OutData &data);
};

ostream& operator<<(ostream &out, const OutData &data);

#endif 
