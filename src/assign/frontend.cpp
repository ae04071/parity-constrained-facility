//
// Created by jiho on 20. 5. 22..
//
#include "assign.h"
#include <functional>
#include "dbg.h"
#include <iostream>

static double find_assign(const struct PCFLProbData *data, const bool *open, int *assign, double cutoff,
        const std::function<struct aux_graph(const struct PCFLProbData *data,
                const std::vector<int> &vopen, const int *assign)> &graph_func) {
    const int m = data->m, n = data->n;

    std::vector<int> vopen = assign_vopen(m, open);

//    if (vopen.size() >= 10) {
//        auto &out = std::cout;
//        out << "too large vopen(size: " << vopen.size() << "):";
//        for (auto i : vopen)
//            out << " " << i;
//        out << std::endl;
//    }

    // feasibility check
    if (!assign_feasible(data, vopen))
        return INFINITY;

    // opening cost
    double total_open_cost = assign_open_cost(data, vopen);
    if (total_open_cost > cutoff)
        return INFINITY;

    // minimum cost unconstrained assignment
    {
        double obj = total_open_cost + assign_unconstrained(data, vopen, assign);
        if (obj > cutoff)
            return INFINITY;
    }

    {
        // construct graph
        auto graph = graph_func(data, vopen, assign);

        // solve and reassign
        assign_solve(graph, assign);
    }

    // calculate the total cost
    return total_open_cost + assign_assign_cost(data, assign);
}

double pcfl_find_assignment1(const struct PCFLProbData *data, const bool *open, int *assign, double cutoff) {
    return find_assign(data, open, assign, cutoff, assign1_aux_graph);
}
double pcfl_find_assignment2(const struct PCFLProbData *data, const bool *open, int *assign, double cutoff) {
    return find_assign(data, open, assign, cutoff, assign2_aux_graph);
}
double pcfl_find_assignment3(const struct PCFLProbData *data, const bool *open, int *assign, double cutoff) {
    return find_assign(data, open, assign, cutoff, assign3_aux_graph);
}
