//
// Created by jiho on 20. 3. 31..
//

#include <gurobi_c++.h>
#include <cstdio>
#include <cstring>
//#include <thread>
#include <memory>
#include <pcfl.h>
#include <cassert>
#include <cerrno>

static bool skip_prefix(const char **str, const char *pre) {
    const char *p = *str;
    while (*pre != '\0')
        if (*p++ != *pre++) return false;
    *str = p;
    return true;
}

int main(int argc, char *argv[]) {
    bool cli_build_only = false;
    PCFLConfig config = {
            .which_impl = 1,
//            .xprior = 2,
//            .yprior = 1,
//            .zprior = 3,
            .threads = 2,
            .time_limit = -1,
            .lazy_parity = false,
            .lazy_open = false,
            .verbose = false,
            .validate_feasibility = false,
            .assignment_method = 1,
            .impl3_use_parity = false,
            .use_open_parity = true,
    };
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "--build-only") == 0)
            cli_build_only = true;
        else if (strcmp(a, "--verbose") == 0)
            config.verbose = true;
//        else if (strcmp(a, "--barrier") == 0)
//            cli_barrier = true;
//        else if (strcmp(a, "--lazy-parity") == 0)
//            config.lazy_parity = true;
//        else if (strcmp(a, "--lazy-open") == 0)
//            config.lazy_open = true;
        else if (strcmp(a, "--impl1") == 0)
            config.which_impl = 1;
        else if (strcmp(a, "--impl2") == 0)
            config.which_impl = 2;
        else if (strcmp(a, "--impl3") == 0)
            config.which_impl = 3;
        else if (strcmp(a, "--impl4") == 0)
            config.which_impl = 4;
        else if (strcmp(a, "--assignment1") == 0)
            config.assignment_method = 1;
        else if (strcmp(a, "--assignment2") == 0)
            config.assignment_method = 2;
        else if (strcmp(a, "--assignment3") == 0)
            config.assignment_method = 3;
        else if (strcmp(a, "--impl3-no-parity") == 0)
            config.impl3_use_parity = false;
        else if (strcmp(a, "--impl3-parity") == 0)
            config.impl3_use_parity = true;
        else if (strcmp(a, "--open-parity") == 0)
            config.use_open_parity = true;
        else if (strcmp(a, "--no-open-parity") == 0)
            config.use_open_parity = false;
        else if (strcmp(a, "--validate") == 0)
            config.validate_feasibility = true;
        else if (skip_prefix(&a, "--threads=")) {
            long value;
            {
                char *end = nullptr;
                errno = 0;
                value = strtol(a, &end, 0);
                if (errno != 0 || end == a || end == nullptr || *end != '\0') {
                    fprintf(stderr, "Conversion error for option --threads=<x>\n");
                    return 1;
                }
            }
            config.threads = (int)value;
        }
        else if (skip_prefix(&a, "--time-limit=")) {
            double value;
            {
                char *end = nullptr;
                errno = 0;
                value = strtod(a, &end);
                if (errno != 0 || end == a || end == nullptr || *end != '\0') {
                    fprintf(stderr, "Conversion error for option --time-limit=<x>\n");
                    return 1;
                }
            }
            config.time_limit = value;
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", a);
            return 1;
        }
    }
    if (cli_build_only) {
        return 0;
    }

    static char buf[1024];

    struct PCFLProbData data = {};
    // input
    std::cin >> data.n >> data.m;
    data.parity = new int[data.m];
    data.opening_cost = new double[data.m];
    data.assign_cost = new double[data.m * data.n];
    std::unique_ptr<int[]> _1(data.parity);
    std::unique_ptr<double[]> _2(data.opening_cost);
    std::unique_ptr<double[]> _3(data.assign_cost);

    for (int i = 0; i < data.m; i++) {
        std::cin >> data.parity[i];
    }
    for (int i = 0; i < data.m; i++) {
        std::cin >> data.opening_cost[i];
    }
    for (int i = 0; i < data.m; i++) {
        for (int j = 0; j < data.n; j++) {
            std::cin >> data.assign_cost[i * data.n + j];
        }
    }

    std::unique_ptr<bool[]> guard_open(new bool[data.m]);
    std::unique_ptr<int[]> guard_assign(new int[data.n]);
    PCFLSolution sol = {
            .open = guard_open.get(),
            .assign = guard_assign.get(),
    };
    switch (config.which_impl) {
    case 1:
        pcfl_impl1(&data, &config, &sol);
        break;
    case 2:
        pcfl_impl2(&data, &config, &sol);
        break;
    case 3:
        pcfl_impl3(&data, &config, &sol);
        break;
    case 4:
        pcfl_impl4(&data, &config, &sol);
		pcfl_impl3_with_initial(&data, &config, &sol);
        break;
    default:
        assert(false);
    }
    if (config.validate_feasibility) {
        std::ostream &out = std::cerr;
        out << std::fixed;
        const double ABSTOL = 0.5e-6;
        double cost_val = 0;
        for (int i = 0; i < data.m; i++) {
            if (sol.open[i]) {
                cost_val += data.opening_cost[i];
            }
        }
        for (int j = 0; j < data.n; j++) {
            int i = sol.assign[j];
            cost_val += data.assign_cost[i * data.n + j];
        }
        if (sol.obj < cost_val - ABSTOL || sol.obj > cost_val + ABSTOL) {
            out.precision(12);
            out << "obj invalid(output: " << sol.obj << ", calculated to: " << cost_val << ")" << std::endl;
        }
        for (int j = 0; j < data.n; j++) {
            int i = sol.assign[j];
            if (!sol.open[i])
                out << "not open(" << j << " to " << i << ")" << std::endl;
        }
        for (int i = 0; i < data.m; i++) {
            if (sol.open[i]) {
                int p = data.parity[i];
                if (p == 0) continue;
                for (int j = 0; j < data.n; j++) {
                    if (sol.assign[j] == i)
                        p++;
                }
                if (p % 2 != 0) {
                    out << "parity violated(" << i << " is " << (data.parity[i] == 1 ? "odd" : "even")
                            << " but " << (p - data.parity[i]) << " assigned)" << std::endl;
                }
            }
        }
    }
    if (config.verbose) {
        std::ostream &out = std::cout;
        out << std::fixed;
        out.precision(8);
        out << "Open:";
        for (int i = 0; i < data.m; i++) {
            if (sol.open[i]) {
                out << " " << i;
            }
        }
        out << std::endl;
        for (int i = 0; i < data.m; i++) {
            if (sol.open[i]) {
                out << "Transport[" << i << "]:";
                for (int j = 0; j < data.n; j++) {
                    if (sol.assign[j] == i) {
                        out << " " << j;
                    }
                }
                out << std::endl;
            }
        }
        out.precision(8);
        out << "obj: " << sol.obj << std::endl;
    }
    {
        std::ostream &out = std::cout;
        out << std::fixed;
        out.precision(6);
        out << sol.obj << std::endl;
        out.precision(6);
        out << sol.runtime << std::endl;
        out << sol.status << std::endl;
    }
    return 0;
}
