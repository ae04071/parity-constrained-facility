//
// Created by jiho on 20. 5. 22..
//

#ifndef PCFL_ASSIGN_H
#define PCFL_ASSIGN_H

#include <pcfl.h>
#include <vector>
#include <list>
#include <cmath>

struct edge {
    int u, v;
    double cost;
    int j[2], i;
    bool check;
};

struct aux_graph {
    int nv;
    std::vector<struct edge> edges;
};

typedef std::list<const struct edge*> mcpm_t;
mcpm_t assign_mcpm(int n, const std::vector<struct edge> &edges);

struct aux_graph assign1_aux_graph(const PCFLProbData *data,
        const std::vector<int> &vopen, const int *assign);
struct aux_graph assign2_aux_graph(const PCFLProbData *data,
        const std::vector<int> &vopen, const int *assign);
struct aux_graph assign3_aux_graph(const PCFLProbData *data,
        const std::vector<int> &vopen, const int *assign);

static inline std::vector<int> assign_vopen(int m, const bool *open) {
    std::vector<int> vopen;
    for (int i = 0; i < m; i++) {
        if (open[i]) {
            vopen.push_back(i);
        }
    }
    return vopen;
}

static inline bool assign_feasible(const struct PCFLProbData *data, const std::vector<int> &vopen) {
    const int n = data->n;
    if (vopen.empty())
        return false;
    static const int DFA_TABLE[4][3] = {
            {0, 0, 0},
            {0, 2, 1},
            {0, 1, 2},
            {0, 1, 2},
    };
    int total_parity = 3;
    for (int i : vopen) {
        total_parity = DFA_TABLE[total_parity][data->parity[i]];
    }
    return total_parity != 3 && (total_parity == 0 || (total_parity + n) % 2 == 0);
}

static inline double assign_open_cost(const struct PCFLProbData *data,
        const std::vector<int> &vopen) {
    double ret = 0;
    for (int i : vopen) {
        ret += data->opening_cost[i];
    }
    return ret;
}

static inline double assign_assign_cost(const struct PCFLProbData *data,
        const int *assign) {
    const int n = data->n;
    double ret = 0;
    for (int j = 0; j < n; j++) {
        ret += data->assign_cost[assign[j] * n + j];
    }
    return ret;
}

static inline double assign_unconstrained(const struct PCFLProbData *data,
        const std::vector<int> &vopen, int *assign) {
    const int n = data->n;
    std::vector<double> assign_cost(n, INFINITY);
    for (int i : vopen) {
        const double *assignment_cost = data->assign_cost + i * n;
        for (int j = 0; j < n; j++) {
            if (assignment_cost[j] < assign_cost[j]) {
                assign[j] = i;
                assign_cost[j] = assignment_cost[j];
            }
        }
    }
    double ret = 0;
    for (auto v : assign_cost)
        ret += v;
    return ret;
}

static inline void assign_solve(const struct aux_graph &graph, int *assign) {
    auto sol = assign_mcpm(graph.nv, graph.edges);
    for (auto r : sol) {
        for (int j : r->j)
            if (j != -1)
                assign[j] = r->i;
    }
}
#endif //PCFL_ASSIGN_H
