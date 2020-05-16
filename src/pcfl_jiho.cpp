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

int main(int argc, char *argv[]) {
    bool cli_build_only = false;
    PCFLConfig config = {
            .which_impl = 1,
            .xprior = 2,
            .yprior = 1,
            .zprior = 3,
            .threads = 2,
            .time_limit = -1,
            .lazy_parity = false,
            .lazy_open = false,
            .verbose = false,
            .validate_feasibility = false,
    };
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "--build-only") == 0)
            cli_build_only = true;
        else if (strcmp(a, "--verbose") == 0)
            config.verbose = true;
//        else if (strcmp(a, "--barrier") == 0)
//            cli_barrier = true;
        else if (strcmp(a, "--lazy-parity") == 0)
            config.lazy_parity = true;
        else if (strcmp(a, "--lazy-open") == 0)
            config.lazy_open = true;
        else if (strcmp(a, "--impl1") == 0)
            config.which_impl = 1;
        else if (strcmp(a, "--impl2") == 0)
            config.which_impl = 2;
        else if (strcmp(a, "--impl3") == 0)
            config.which_impl = 3;
        else if (strcmp(a, "--validate") == 0)
            config.validate_feasibility = true;
        else if (strcmp(a, "--threads") == 0) {
            i += 1;
            if (i >= argc) {
                fprintf(stderr, "Not enough parameters for option --threads\n");
                return 1;
            }
            long value;
            {
                char *end = nullptr;
                errno = 0;
                value = strtol(argv[i], &end, 0);
                if (errno != 0 || end == argv[i] || end == nullptr || *end != '\0') {
                    fprintf(stderr, "Conversion error for option --threads\n");
                }
            }
            config.threads = (int)value;
        }
        else if (strcmp(a, "--time-limit") == 0) {
            i += 1;
            if (i >= argc) {
                fprintf(stderr, "Not enough parameters for option --time-limit\n");
                return 1;
            }
            double value;
            {
                char *end = nullptr;
                errno = 0;
                value = strtod(argv[i], &end);
                if (errno != 0 || end == argv[i] || end == nullptr || *end != '\0') {
                    fprintf(stderr, "Conversion error for option --time-limit\n");
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
    default:
        assert(false);
    }
    if (config.validate_feasibility) {
        std::ostream &out = std::cerr;
        out << std::fixed;
        out.precision(6);
        const double ABSTOL = 1e-6;
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
        out.precision(6);
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
                        out << " " << i;
                    }
                }
                out << std::endl;
            }
        }
    }
    {
        std::ostream &out = std::cerr;
        out << std::fixed;
        out.precision(6);
        out << sol.obj << std::endl;
        out << sol.runtime << std::endl;
        out << sol.status << std::endl;
    }
    return 0;
}
