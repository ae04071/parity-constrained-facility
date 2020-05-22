#ifndef __PCFL_H__
#define __PCFL_H__

struct PCFLProbData {
    int m, n; // n: number of client, m: number of facility
    int *parity;
    double *opening_cost;
    double *assign_cost; // m x n
};

struct PCFLSolution {
    double obj;
    double runtime;
    int status;
    bool *open;
    int *assign;
};

struct PCFLConfig {
    int which_impl;
//    int xprior, yprior, zprior;
    int threads;
    double time_limit;
    bool lazy_parity, lazy_open;
    bool verbose;
    bool validate_feasibility;
    int assignment_method;
    bool impl3_use_parity;
    bool use_open_parity;
    int assignment_threads;
};

double pcfl_find_assignment1(const struct PCFLProbData *data, const bool *open, int *to_facility, double cutoff);
double pcfl_find_assignment2(const struct PCFLProbData *data, const bool *open, int *to_facility, double cutoff);
double pcfl_find_assignment3(const struct PCFLProbData *data, const bool *open, int *to_facility, double cutoff);
//#define pcfl_find_assignment pcfl_find_assignment2

extern double (*pcfl_find_assignment_methods[])(const struct PCFLProbData *data, const bool *open, int *to_facility, double cutoff);

void pcfl_impl1(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);
void pcfl_impl2(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);
void pcfl_impl3(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);

#endif
