//
// Created by jiho on 20. 5. 22..
//
#include "assign.h"

static size_t mcpm_cost(size_t V, size_t E) {
    return V * V * E;
}
static size_t mcpm_cost(struct aux_graph &graph) {
    return mcpm_cost(graph.nv, graph.edges.size());
    //TODO: this time complexity is for blossom, but might be different for blossom V
}

template<typename T>
static T min2(T a, T b) { return a <= b ? a : b; }

template<typename T>
static T choose2(T n) { return n * (n - 1) / 2; }

struct aux_graph assign3_aux_graph(const PCFLProbData *data,
        const std::vector<int> &vopen, const int *assign) {
#define gen1 assign1_aux_graph(data, vopen, assign)
#define gen2 assign2_aux_graph(data, vopen, assign)
    const size_t m = vopen.size(), n = data->n;
    /* predict */
    size_t g1_cost;
    {
        size_t sno = 0;
        for (int i : vopen)
            sno += data->parity[i] == 1;
        size_t nc2 = choose2(n);
        g1_cost = mcpm_cost(m + n, nc2 + n * sno);
        size_t g2_min, g2_max;
        size_t max_size_s = (n - 1) * (n - 1);
        g2_max = mcpm_cost(min2(m * m + 1 + n, n * max_size_s), nc2 * (n - 1) + n * choose2(max_size_s));
        if (g2_max < g1_cost)
            return gen2;
        g2_min = mcpm_cost(n * (n - 1), nc2 + n * choose2(n - 1));
        if (g2_min > g1_cost)
            return gen1;
    }
    /* generate and compare */
    {
        struct aux_graph g1 = gen1, g2 = gen2;
        return g1_cost < mcpm_cost(g2) ? g1 : g2;
    }
#undef gen1
#undef gen2
}

/*
 * |V| = |S| + |D|
 * |E| = |D|(|D| - 1)/2 + |D||S n O|
 */
/*
 * |V| >= |S|(|S| - 1)
 * |V| <= min{|S|^2 + 1 + |D|, |S|max_size(|S|)}
 * |E| >= |S| choose 2 + |S|((|S| - 1) choose 2)
 * |E| <= (|S| choose 2)(|S| - 1) + |S|(max_size(|S|) choose 2)
 * max_size(s) = (s - 1)^2 # not very tight
 *
 * TODO: find better method by predicting |V| and |E|
 */
