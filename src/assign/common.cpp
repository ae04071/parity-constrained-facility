//
// Created by jiho on 20. 5. 22..
//
#include "assign.h"

#if 0
#include <mcpm.h>
mcpm_t assign_mcpm(int n, const std::vector<struct edge> &edges) {
    int m = edges.size();
    Graph g(n);
    std::vector<double> cost(m);
    for (auto &e : edges) {
        g.AddEdge(e.u, e.v);
        cost[g.GetEdgeIndex(e.u, e.v)] = e.cost;
    }
    Matching matching(g);
    auto sol = matching.SolveMinimumCostPerfectMatching(cost).first;
    mcpm_t ret;
    for (int k : sol) {
        if (edges[k].check)
            ret.push_back(&edges[k]);
    }
    return ret;
}

#else
#include <blossom5.h>

mcpm_t assign_mcpm(int n, const std::vector<struct edge> &edges) {
    PerfectMatching pm(n, edges.size());
    pm.options.fractional_jumpstart = true;
    pm.options.dual_greedy_update_option = 0;
    pm.options.dual_LP_threshold = 0.00;
    pm.options.update_duals_before = false;
    pm.options.update_duals_after = false;
    pm.options.single_tree_threshold = 1.00;
    pm.options.verbose = false;
    for (auto &e : edges) {
        pm.AddEdge(e.u, e.v, e.cost);
    }
    pm.Solve();
    mcpm_t ret;
//    double cost;
    for (int i = 0; i < edges.size(); i++) {
        auto &e = edges[i];
        if (e.check && pm.GetSolution(i))
            ret.push_back(&e);
    }
    return ret;
}
#endif
