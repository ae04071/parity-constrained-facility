//
// Created by jiho on 20. 4. 24..
//

#include <pcfl.h>
#include <mcpm.h>
#include <vector>
#include <list>
#include <math.h>

struct reassign {
    int u, v;
    double cost;
    int j;
};

static std::pair<std::list<int>, double> mcpm(int n, const std::vector<struct reassign> &edges) {
    int m = edges.size();
    Graph g(n);
    std::vector<double> cost(m);
    for (auto e : edges) {
        g.AddEdge(e.u, e.v);
        cost[g.GetEdgeIndex(e.u, e.v)] = e.cost;
    }
    Matching matching(g);
    return matching.SolveMinimumCostPerfectMatching(cost);
}

static struct reassign best_reassign(const PCFLProbData *data, const int *assign,
        int u, int v, int iu, int iv) {
    struct reassign ret = {u, v, INFINITY, -1};
    const double *ucost = data->assign_cost + iu * data->n,
            *vcost = data->assign_cost + iv * data->n;
    for (int j = 0; j < data->n; j++) {
        if (assign[j] == iu) {
            const double cost = vcost[j] - ucost[j];
            if (cost < ret.cost)
                ret = {u, v, cost, j};
        }
    }
    return ret;
}

double pcfl_find_assignment(const struct PCFLProbData *data, const bool *open, int *assign, double cutoff) {
//    printf("Open:");
//    for (int i = 0; i < data->m; i++) {
//        if (open[i]) {
//            printf(" %d", i);
//        }
//    }
//    printf("\n");
    // feasibility check
    {
        static const int DFA_TABLE[4][3] = {
                {0, 0, 0},
                {0, 2, 1},
                {0, 1, 2},
                {0, 1, 2},
        };
        int total_parity = 3;
        for (int i = 0; i < data->m; i++) {
            if (open[i]) {
                total_parity = DFA_TABLE[total_parity][data->parity[i]];
            }
        }
        if (total_parity == 3) return INFINITY;
        if (total_parity != 0 && (total_parity + data->n) % 2 != 0) return INFINITY;
    }

    double total_open_cost = 0;
    for (int i = 0; i < data->m; i++) {
        if (open[i]) total_open_cost += data->opening_cost[i];
    }

    // minimum cost unconstrained assignment
    auto *assign_cost = new double[data->n];
    for (int j = 0; j < data->n; j++) {
        assign[j] = -1;
        assign_cost[j] = INFINITY;
    }
    for (int i = 0; i < data->m; i++) {
        if (open[i]) {
            const double *assignment_cost = data->assign_cost + i * data->n;
            for (int j = 0; j < data->n; j++) {
                if (assignment_cost[j] < assign_cost[j]) {
                    assign[j] = i;
                    assign_cost[j] = assignment_cost[j];
                }
            }
        }
    }
    {
        double ret = total_open_cost;
        for (int j = 0; j < data->n; j++) {
            ret += assign_cost[j];
        }
        if (ret > cutoff)
            return INFINITY;
    }
    delete[] assign_cost;

    // re-assignment graph
    std::vector<int> V; // parity violated facilities + unconstrained
    std::vector<struct reassign> E;
    for (int i = 0; i < data->m; i++) {
        if (open[i]) {
            int cnt = data->parity[i];
            if (cnt == 0) {
//                int u = (int)V.size();
//                E.push_back({u, u, 0, -1});
            } else {
                for (int j = 0; j < data->n; j++) {
                    if (assign[j] == i)
                        cnt++;
                }
                if (cnt % 2 == 0) continue;
            }
            V.push_back(i);
        }
    }
    if (V.size() % 2 != 0) {
        V.push_back(-1);
    }
    for (int u = 0; u < V.size(); u++) {
        int iu = V[u];
        if (iu == -1) continue;
        bool used = false;
        for (int j = 0; j < data->n; j++) {
            if (assign[j] == iu) {
                used = true;
                break;
            }
        }
        if (data->parity[iu] != 0 && !used) continue;
        for (int v = 0; v < V.size(); v++) {
            int iv = V[v];
            if (u != v) {
                if (data->parity[iu] == 0 && (iv == -1 || data->parity[iv] == 0)) {
                    if (iv < iu)
                        E.push_back({u, v, 0, -1});
                } else if (iv != -1) {
                    if (used)
                        E.push_back(best_reassign(data, assign, u, v, iu, iv));
                }
            }
        }
    }
    auto sol = mcpm(V.size(), E);
    for (int k : sol.first) {
        auto e = E[k];
        if (e.j != -1)
            assign[e.j] = V[e.v];
    }

    // calculate the total cost
    {
        double ret = 0;
        for (int i = 0; i < data->m; i++) {
            if (open[i]) ret += data->opening_cost[i];
        }
        for (int j = 0; j < data->n; j++) {
            ret += data->assign_cost[assign[j] * data->n + j];
        }
        return ret;
    }
}

/*
 * FIXME: this algorithm is incorrect
 */
