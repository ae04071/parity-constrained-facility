//
// Created by jiho on 20. 4. 24..
//

#include <pcfl.h>
#include <list>
#include <tuple>
#include <memory>
#include <math.h>
#include <string.h>
#include <functional>
#include <LPRelaxation.h>
#include <algorithm>

static bool bound_true(const PCFLProbData *data, const bool *open, const bool *mask, double cutoff, int open_lb, int open_ub) {
    return true;
}

static double naive(const PCFLProbData *data, int i, bool *open, int *assign, double cutoff,
        const std::function<bool(const PCFLProbData *data,
                const bool *open, const bool *mask, double cutoff,
                int open_lb, int open_ub)> &bound_func,
        int open_lb, int open_ub, int num_open) {
    if (i == data->m) {
        double ret = pcfl_find_assignment(data, open, assign, cutoff);
        if (ret < cutoff) {
            printf("Open:");
            for (int ii = 0; ii < data->m; ii++) {
                if (open[ii]) {
                    printf(" %d", ii);
                }
            }
            printf("\n");
            printf("%lf\n", ret);
        }
        return ret;
    } else {
        std::unique_ptr<bool[]> tmp_open(new bool[data->m]), open_mask(new bool[data->m]);
        std::unique_ptr<int[]> tmp_to(new int[data->n]);
        double ret = INFINITY;
        memcpy(tmp_open.get(), open, sizeof(*open) * i);
        for (int k = 0; k < data->m; k++) open_mask[k] = k <= i;

        for (auto val : {false, true}) {
            tmp_open[i] = val;
            int next_num_open = num_open + (val ? 1 : 0);
            int next_num_mask = i + 1;
            if (next_num_open > open_ub || next_num_open + (data->m - next_num_mask) < open_ub) {
                //sum(x) bound
            } else if (
                    !bound_func(data, tmp_open.get(), open_mask.get(), cutoff, open_lb, open_ub)) {
//                if (i < data->m / 2)
                printf("bound: %d\n", i);
            } else {
                double cost = naive(data, i + 1, tmp_open.get(), tmp_to.get(), cutoff, bound_func,
                        open_lb, open_ub, next_num_open);
                if (cost < ret) {
                    memcpy(open, tmp_open.get(), sizeof(*open) * data->m);
                    memcpy(assign, tmp_to.get(), sizeof(*assign) * data->n);
                    ret = cost;
                    if (ret < cutoff) {
                        cutoff = ret;
                    }
                }
            }
        }
        return ret;
    }
}
//static double naive_i(const PCFLProbData *data, bool *open, int *assign, double cutoff,
//        int num_open, int cur_open, int i,
//        const std::function<bool(const PCFLProbData *data,
//                const bool *open, const bool *mask, double cutoff,
//                int open_lb, int open_ub)> &bound_func) {
//    if (i == data->m) {
//        double ret = pcfl_find_assignment(data, open, assign);
//        if (ret < cutoff) {
//            printf("Open:");
//            for (int ii = 0; ii < data->m; ii++) {
//                if (open[ii]) {
//                    printf(" %d", ii);
//                }
//            }
//            printf("\n");
//            printf("%lf\n", ret);
//        }
//        return ret;
//    } else {
//        std::unique_ptr<bool[]> tmp_open(new bool[data->m]), open_mask(new bool[data->m]);
//        std::unique_ptr<int[]> tmp_to(new int[data->n]);
//        double ret = INFINITY;
//        memcpy(tmp_open.get(), open, sizeof(*open) * i);
//        for (int k = 0; k < data->m; k++) open_mask[k] = k <= i;
//
//        for (auto val : {false, true}) {
//            tmp_open[i] = val;
//            int next_num_open = num_open + (val ? 1 : 0);
//            int next_num_mask = i + 1;
//            if (next_num_open > open_ub || next_num_open + (data->m - next_num_mask) < open_ub) {
//                //sum(x) bound
//            } else if (!bound_func(data, tmp_open.get(), open_mask.get(), cutoff, open_lb, open_ub)) {
////                if (i < data->m / 2)
//                printf("bound: %d\n", i);
//            } else {
//                double cost = naive(data, i + 1, tmp_open.get(), tmp_to.get(), cutoff, bound_func,
//                        open_lb, open_ub, next_num_open);
//                if (cost < ret) {
//                    memcpy(open, tmp_open.get(), sizeof(*open) * data->m);
//                    memcpy(assign, tmp_to.get(), sizeof(*assign) * data->n);
//                    ret = cost;
//                    if (ret < cutoff) {
//                        cutoff = ret;
//                    }
//                }
//            }
//        }
//        return ret;
//    }
//}
static double less_open_first(const PCFLProbData *data, bool *open, int *assign, double cutoff,
        const std::function<bool(const PCFLProbData *data,
                const bool *open, const bool *mask, double cutoff,
                int open_lb, int open_ub)> &bound_func) {
    std::unique_ptr<bool[]> tmp_open(new bool[data->m]);
    std::unique_ptr<int[]> tmp_to(new int[data->n]);
    double ret = INFINITY;
//    cutoff = 1019.005459;
    for (int i = 1; i <= data->m; i++) {
        if (!bound_func(data, nullptr, nullptr, cutoff, i, i)) {
            printf("ignore: %d\n", i);
            continue;
        }
//        continue;
        double cost = naive(data, 0, tmp_open.get(), tmp_to.get(), cutoff, i <= 20 ? bound_true : bound_func, i, i, 0);
        if (cost < ret) {
            memcpy(open, tmp_open.get(), sizeof(*open) * data->m);
            memcpy(assign, tmp_to.get(), sizeof(*assign) * data->n);
            ret = cost;
            if (ret < cutoff) {
                cutoff = ret;
            }
        }
    }
    return ret;
}
static bool bound1(const PCFLProbData *data, const bool *open, const bool *mask, double cutoff,
        int open_lb, int open_ub) {
    double bound = 0;
    int num_open = 0;
    if (open != nullptr && mask != nullptr) {
        for (int i = 0; i < data->m; i++) {
            if (open[i] & mask[i]) {
                bound += data->opening_cost[i];
                num_open++;
            }
        }
        if (bound > cutoff)
            return false;
    }
    if (num_open > open_ub)
        return false;
    if (num_open < open_lb) {
        std::unique_ptr<int[]> tmp_f(new int[data->m]);
        std::sort(tmp_f.get(), tmp_f.get() + data->m, [data](int a, int b) {
            return data->opening_cost[a] < data->opening_cost[b];
        });
    }
    for (int j = 0; j < data->n; j++) {
        double cur_min_cost = INFINITY;
        for (int i = 0; i < data->m; i++) {
            if (open[i] || !mask[i]) {
                const double cur_cost = data->assign_cost[i * data->n + j];
                if (cur_cost < cur_min_cost)
                    cur_min_cost = cur_cost;
            }
        }
        bound += cur_min_cost;
        if (bound > cutoff)
            return false;
    }
    return true;
}
void pcfl_impl2(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol) {
//    return naive(data, 0, open, assign, INFINITY, bound1);

    LPRelaxation rel(data);
    auto  bound_lp = [&rel](const PCFLProbData *data, const bool *open,
                            const bool *mask, double cutoff,
                            int open_lb, int open_ub) {
        return rel.bound_func(open, mask, cutoff, open_lb, open_ub);
    };
    sol->obj = less_open_first(data, sol->open, sol->assign, INFINITY, bound1);
    sol->runtime = NAN;
    sol->status = GRB_OPTIMAL;
//    return naive(data, 0, open, assign, cutoff, bound_lp, 1, data->m, 0);
}

/*
 * first, find the optimal solution under sum(x) <= 5
 * then, cut by sum(x), and iterate all
 */
/*
 * just assume n > 0
 */
/*
 * filter junk facilities (high assign cost)
 * (opening this facility always makes a suboptimal solution)
 */
