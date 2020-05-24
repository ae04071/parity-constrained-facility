//
// Created by jiho on 20. 4. 24..
//

#include <pcfl.h>
#include "assign.h"
#include <cmath>
//#include <memory>
#include <algorithm>
#include <functional>
#include <iostream>

//struct vertex {
//    int index;
//    int type;//0: demand, 1: receive, 2: dummy
//    union {
//        struct demand *demand;
//        struct receive *receive;
//        struct facility *facility;
//    } payload;
//};
typedef int vertex;

struct assign2_facility {
    int index;
    int i;
    bool unconstrained;
    bool invalid;
    int size;
    struct assign2_receive *vrecv;
    struct assign2_demand **vpdemands, **vpdemands_end;//vector of pointers to demands
    vertex dummy;
};
struct assign2_reassign {
    struct assign2_demand *from;
    struct assign2_receive *to;
    double cost;

    int depth;
    struct assign2_reassign *alternative_prev; //this is alternative of alternative_prev
    struct assign2_reassign *demand_next; //next node for list in demand
};

struct assign2_demand {
    int j;
    struct assign2_facility *initial;
    int reassign_count;
    struct assign2_reassign *reassign_list;
    vertex v;
};

struct assign2_receive {
    struct assign2_facility *from, *to;
    vertex v;
};

static struct assign2_reassign best_reassign(const PCFLProbData *data,
        struct assign2_demand *demands,
        struct assign2_facility *fp, struct assign2_facility *fq,
        const struct assign2_reassign *exclude_list) {
    const int n = data->n;
    struct assign2_reassign ret = {nullptr, nullptr, INFINITY};
    for (int j = 0; j < n; j++) {
        {
            for (const struct assign2_reassign *p = exclude_list; p != nullptr; p = p->alternative_prev) {
                if (p->from->j == j) {
                    goto exclude_label;
                }
            }
            goto noexclude_label;
        exclude_label:
            continue;
        noexclude_label:;
        }
        struct assign2_demand &d = demands[j];
        struct assign2_facility *from, *to;
        if (d.initial == fp) {
            from = fp;
            to = fq;
        } else if (d.initial == fq) {
            from = fq;
            to = fp;
        } else {
            continue;
        }
        const double cost = data->assign_cost[to->i * n + j] -
                data->assign_cost[from->i * n + j];
        if (cost < ret.cost) {
            ret = {&d, &to->vrecv[from->index], cost};
        }
    }
    return ret;
}

/*
 * dynamic array, but preserve the address for each value
 */
template<typename T, size_t size>
class list_of_array {
private:
    struct node {
        T buf[size]; //it is used
    };
    std::list<struct node> list;
    ptrdiff_t i;
public:
    T *allocate() {
        if (i == size) {
            list.push_back({});
            i = 0;
        }
        return &list.back().buf[i++];
    }
    list_of_array() : i(size) {}
};

struct aux_graph assign2_aux_graph(const PCFLProbData *data,
        const std::vector<int> &vopen, const int *assign) {
    const int n = data->n, m = vopen.size();
    std::vector<struct assign2_demand> demands(n);
    std::vector<struct assign2_demand*> vpdemands_buf(n);
    std::vector<struct assign2_facility> facilities(m);
    int nv = 0;
//    std::vector<struct vertex*> V;
    std::vector<struct assign2_reassign*> reassigns;
    std::vector<struct assign2_receive> recv_mat_buf(m * m);
    std::vector<struct assign2_reassign*> need_alternative;
    list_of_array<struct assign2_reassign, 1024> reassign_alloc;

    /* initialize facilities */
    for (int p = 0; p < m; p++) {
        auto &f = facilities[p];
        f.index = p;
        f.i = vopen[p];
        f.size = 0;
//        f.dummy = {-1, 2, {.facility = &f}};
        f.dummy = -1;
        f.vrecv = recv_mat_buf.data() + p * m;
        for (int q = 0; q < m; q++) {
            auto &r = f.vrecv[q];
            r.from = &facilities[q];
            r.to = &f;
//            r.v = {-1, 1, {.receive = &r}};
            r.v = -1;
        }
        f.unconstrained = data->parity[f.i] == 0;
        if (!f.unconstrained) {
            f.invalid = data->parity[f.i] != 2;
            for (int j = 0; j < n; j++) {
                f.invalid ^= assign[j] == f.i;
            }
        }
    }

    /* initialize demands */
    for (int j = 0; j < n; j++) {
        auto &d = demands[j];
        d.j = j;
        d.initial = std::lower_bound(facilities.data(), facilities.data() + m,
                (struct assign2_facility){.i = assign[j]},
                [](const struct assign2_facility &a, const struct assign2_facility &b) {
                    return a.i < b.i;
                });
        d.reassign_count = 0;
        d.reassign_list = nullptr;
//        d.v = {-1, 0, {.demand = &d}};
        d.v = -1;
    }

    /* vpdemands */
    {
        auto cmp = [](struct assign2_demand *a, struct assign2_demand *b){
            return a->initial->i < b->initial->i;
        };
        for (int j = 0; j < n; j++)
            vpdemands_buf[j] = &demands[j];
        std::sort(vpdemands_buf.data(), vpdemands_buf.data() + n, cmp);
        for (auto &f : facilities) {
            struct assign2_demand key = {.initial = &f};
            f.vpdemands = std::lower_bound(vpdemands_buf.data(), vpdemands_buf.data() + n, &key, cmp);
            f.vpdemands_end = std::upper_bound(vpdemands_buf.data(), vpdemands_buf.data() + n, &key, cmp);
        }
    }

    {
        const int max_depth = m - 1;
#define add_reassign_ARGS (struct assign2_facility *fp, struct assign2_facility *fq, struct assign2_reassign *prev)
        std::function<void add_reassign_ARGS> add_reassign = [&]add_reassign_ARGS {
            int prev_depth = prev != nullptr ? prev->depth : 0;
            if (prev_depth >= max_depth) return;
            struct assign2_reassign *r = reassign_alloc.allocate();
            *r = best_reassign(data, demands.data(), fp, fq, prev);
            if (r->from == nullptr) return; // all demands are consumed, O(n^2)
            r->depth = prev_depth + 1;

            auto *from = r->from;
            auto *to = r->to;

            /* vertex allocation */
            if (to->v == -1)
                to->v = nv++;
            if (from->v == -1)
                from->v = nv++;

            // add to list
            r->demand_next = from->reassign_list;
            from->reassign_list = r;
            from->reassign_count++;

            // add prev list
            r->alternative_prev = prev;

            // add to the vector
            reassigns.push_back(r);

            auto r2 = r->demand_next;
            if (r2 != nullptr) {
                /* alternative */
                if (r2->demand_next == nullptr) {
                    /*
                     * the first reassign of the current demand
                     * we should make an alternative here
                     */
                    add_reassign(r2->from->initial, r2->to->to, r2);
                }
                add_reassign(fp, fq, r);
            }

        };
#undef add_reassign_ARGS
        for (int p = 0; p < m; p++) {
            for (int q = p + 1; q < m; q++) {
                struct assign2_facility *fp = &facilities[p], *fq = &facilities[q];
                add_reassign(fp, fq, nullptr);
            }
        }
    }
    {
        std::vector<struct edge> edges;
        {
            auto pgsz = [](size_t n) { return n * (n - 1) / 2; };
//            edges.reserve(pgsz(nv));
            {
                //wasted size <= nv
                size_t edges_size = reassigns.size();
                for (auto &f : facilities) {
                    size_t sz = 1;
                    for (int k = 0; k < m; k++)
                        sz += f.vrecv[k].v != -1;
                    for (auto p = f.vpdemands; p < f.vpdemands_end; p++)
                        sz += (*p)->v != -1;
                    edges_size += pgsz(sz);
                }
                {
                    size_t sz = 1;
                    for (auto &f : facilities)
                        sz += f.unconstrained;
                    edges_size += pgsz(sz);
                }
                edges.reserve(edges_size);
            }
        }

        /* reassignment edges */
        for (auto r : reassigns) {
            edges.push_back({r->from->v, r->to->v, r->cost,
                    {r->from->j, -1}, r->to->to->i, true});
        }

        auto add_pg = [&edges](const std::vector<int> &group) {
            for (int p = 1; p < group.size(); p++)
                for (int q = 0; q < p; q++)
                    edges.push_back({group[p], group[q], 0, {-1, -1}, -1, false});
        };

        /* complete graphs for each facility */
        for (auto &f : facilities) {
            std::vector<int> group;
            //receive vertices
            for (int k = 0; k < m; k++) {
                int v = f.vrecv[k].v;
                if (v != -1)
                    group.push_back(v);
            }
            //demand vertices
            for (auto p = f.vpdemands; p < f.vpdemands_end; p++) {
                int v = (*p)->v;
                if (v != -1)
                    group.push_back(v);
            }
            //dummy vertex
            if (f.unconstrained ||
                    ((group.size() % 2 != 0) ^ f.invalid)) {
//                if (f.dummy == -1)
                f.dummy = nv++;
                group.push_back(f.dummy);
            }
            add_pg(group);
        }

        /* unconstrained group */
        {
            std::vector<int> group;
            if (nv % 2 != 0)
                group.push_back(nv++);
            for (auto &f : facilities) {
                if (f.unconstrained) {
                    if (f.dummy == -1)
                        f.dummy = nv++;
                    group.push_back(f.dummy);
                }
            }
            add_pg(group);
        }
        return {nv, edges};
    }
}

/*
 * |V| >= |S|(|S| - 1)
 * |V| <= min{|S|^2 + 1 + |D|, |S|max_size(|S|)}
 * |E| >= |S| choose 2 + |S|((|S| - 1) choose 2)
 * |E| <= (|S| choose 2)(|S| - 1) + |S|(max_size(|S|) choose 2)
 * max_size(s) = (s - 1)^2 # not very tight
 */
