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
    int xprior, yprior, zprior;
    int threads;
    double time_limit;
    bool lazy_parity, lazy_open;
    bool verbose;
    bool validate_feasibility;
};

double pcfl_find_assignment(const struct PCFLProbData *data, const bool *open, int *to_facility, double cutoff);

void pcfl_impl1(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);
void pcfl_impl2(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);
void pcfl_impl3(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);

#endif
