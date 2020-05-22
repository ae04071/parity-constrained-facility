//
// Created by jiho on 20. 4. 24..
//

#include <pcfl.h>
#include "assign.h"
#include <mcpm.h>
#include <cmath>
#include <iostream>

struct aux_graph assign1_aux_graph(const PCFLProbData *data,
        const std::vector<int> &vopen, const int *assign) {
    const int n = data->n, m = vopen.size();
    std::vector<int> V;
    std::vector<struct edge> E;

    for (int j1 = 0; j1 < n; j1++) {
        for (int j2 = j1 + 1; j2 < n; j2++) {
            int iopt = -1;
            double iopt_cost = INFINITY;
            for (int i : vopen) {
                const double i_cost = data->assign_cost[i * n + j1] +
                        data->assign_cost[i * n + j2];
                if (i_cost < iopt_cost) {
                    iopt = i;
                    iopt_cost = i_cost;
                }
            }
            E.push_back({j1, j2, iopt_cost, {j1, j2}, iopt, true});
        }
    }
    for (int i : vopen) {
        if (data->parity[i] != 2) {
            V.push_back(i);
        }
    }
    for (int u = 0; u < V.size(); u++) {
        const int i = V[u];
        for (int j = 0; j < n; j++) {
            E.push_back({n + u, j, data->assign_cost[i * n + j],
                    {j, -1}, i, true});
        }
    }
    if ((n + V.size()) % 2 != 0) {
        V.push_back(-1);
    }
    for (int u = 0; u < V.size(); u++) {
        const int iu = V[u];
        if (iu == -1 || data->parity[iu] == 0) {
            for (int v = u + 1; v < V.size(); v++) {
                const int iv = V[v];
                if (iv == -1 || data->parity[iv] == 0) {
                    E.push_back({n + u, n + v, 0, {-1, -1}, -1, true});
                }
            }
        }
    }
    return {n + (int)V.size(), E};
}

//double pcfl_find_assignment1(const struct PCFLProbData *data, const bool *open, int *assign, double cutoff) {
//    const int m = data->m, n = data->n;
//
//    std::vector<int> vopen;
//
//    for (int i = 0; i < m; i++) {
//        if (open[i]) {
//            vopen.push_back(i);
//        }
//    }
//
//    if (vopen.size() >= 10) {
//        auto &out = std::cout;
//        out << "vopen(size: " << vopen.size() << "):";
//        for (auto i : vopen)
//            out << " " << i;
//        out << std::endl;
//    }
//
//    // feasibility check
//    {
//        if (vopen.empty())
//            return INFINITY;
//        static const int DFA_TABLE[4][3] = {
//                {0, 0, 0},
//                {0, 2, 1},
//                {0, 1, 2},
//                {0, 1, 2},
//        };
//        int total_parity = 3;
//        for (int i : vopen) {
//            total_parity = DFA_TABLE[total_parity][data->parity[i]];
//        }
//        if (total_parity == 3) return INFINITY;
//        if (total_parity != 0 && (total_parity + n) % 2 != 0) return INFINITY;
//    }
//
//    double total_open_cost = 0;
//    for (int i : vopen) {
//        total_open_cost += data->opening_cost[i];
//    }
//
//#if 1 //unconstrained cutoff
//    {
//        // minimum cost unconstrained assignment
//        auto *assign_cost = new double[n];
//        for (int j = 0; j < n; j++) {
//            assign[j] = -1;
//            assign_cost[j] = INFINITY;
//        }
//        for (int i : vopen) {
//            const double *assignment_cost = data->assign_cost + i * n;
//            for (int j = 0; j < n; j++) {
//                if (assignment_cost[j] < assign_cost[j]) {
//                    assign[j] = i;
//                    assign_cost[j] = assignment_cost[j];
//                }
//            }
//        }
//        {
//            double ret = total_open_cost;
//            for (int j = 0; j < n; j++) {
//                ret += assign_cost[j];
//            }
//            if (ret > cutoff)
//                return INFINITY;
//        }
//        delete[] assign_cost;
//    }
//#endif
//
//    {
//        auto graph = assign1_aux_graph(data, vopen, assign);
//        auto sol = assign_mcpm(graph.nv, graph.edges);
//
//        for (auto r : sol) {
//            for (int j : r->j)
//                if (j != -1)
//                    assign[j] = r->i;
//        }
//    }
//
//    // calculate the total cost
//    {
//        double ret = total_open_cost;
//        for (int j = 0; j < n; j++) {
//            ret += data->assign_cost[assign[j] * n + j];
//        }
//        return ret;
//    }
//}

/*
 * |V| = |S| + |D|
 * |E| = |D|(|D| - 1)/2 + |D||S n O|
 */
