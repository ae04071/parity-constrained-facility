//
// Created by jiho on 20. 3. 31..
//

#include <gurobi_c++.h>
#include <stdio.h>
#include <string.h>
#include <thread>

int main(int argc, char *argv[]) {
    bool cli_build_only = false, cli_verbose = false, cli_barrier = false;
    unsigned int num_threads = std::thread::hardware_concurrency();
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "--build-only") == 0)
            cli_build_only = true;
        else if (strcmp(a, "--verbose") == 0)
            cli_verbose = true;
        else if (strcmp(a, "--barrier") == 0)
            cli_barrier = true;
        else if (strcmp(a, "--threads") == 0) {
            i += 1;
            if (i >= argc) {
                fprintf(stderr, "Not enough parameters for option --threads\n");
                return 1;
            }
            num_threads = atoi(argv[i]);
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", a);
            return 1;
        }
    }
    if (cli_build_only) {
        return 0;
    }
    GRBVar *x = NULL;
    GRBVar *y = NULL;
    GRBVar *z = NULL;

    int n, m;
    int *p = NULL;
    double *o = NULL;
    double *c = NULL; // m x n

    static char buf[1024];
    try {
        // input
        scanf("%d%d", &n, &m);
        p = new int[m];
        o = new double[m];
        c = new double[m * n];
        for (int i = 0; i < m; i++) {
            scanf("%d", &p[i]);
        }
        for (int i = 0; i < m; i++) {
            scanf("%lf", &o[i]);
        }
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                scanf("%lf", &c[i * n + j]);
            }
        }

        // Model
        GRBEnv env;
        GRBModel model(env);
        model.set(GRB_StringAttr_ModelName,
                "capacity constrained facility location");

        model.set(GRB_IntParam_Threads, num_threads);

        // Prepare vars
        y = model.addVars(m, GRB_BINARY);
        z = model.addVars(m, GRB_INTEGER);
        for (int i = 0; i < m; i++) {
            {
                auto &v = y[i];
                sprintf(buf, "Open%d", i);
                v.set(GRB_DoubleAttr_Obj, o[i]);
                v.set(GRB_StringAttr_VarName, buf);
            }
            {
                auto &v = z[i];
                sprintf(buf, "Parity%d", i);
                v.set(GRB_DoubleAttr_Obj, 0);
                v.set(GRB_StringAttr_VarName, buf);
            }
        }

        x = model.addVars(m * n, GRB_BINARY);
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                int index = i * n + j;
                auto &v = x[index];
                sprintf(buf, "Trans%d.%d", i, j);
                v.set(GRB_DoubleAttr_Obj, c[index]);
                v.set(GRB_StringAttr_VarName, buf);
            }
        }

        // The objective is to minimize the total fixed and variable costs
        model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);

        // Constraints
        // Open
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                sprintf(buf, "Open constraint%d.%d", i, j);
                model.addConstr(x[i * n + j] <= y[i], buf);
            }
        }

        // Parity
        for (int i = 0; i < m; i++) {
            GRBLinExpr expr = 0;
            if (p[i]) {
                for (int j = 0; j < n; j++) {
                    expr += x[i * n + j];
                }
                if (p[i] == 1) {
                    sprintf(buf, "Odd%d", i);
                    model.addConstr(expr == z[i] * 2 + y[i], buf);
                } else {
                    sprintf(buf, "Even%d", i);
                    model.addConstr(expr == z[i] * 2, buf);
                }
            }
        }

        // Client
        for (int j = 0; j < n; j++) {
            sprintf(buf, "Client%d", j);
            GRBLinExpr expr = 0;
            for (int i = 0; i < m; i++) {
                expr += x[i * n + j];
            }
            model.addConstr(expr == 1, buf);
        }

        if (cli_barrier) {
            // Use barrier to solve root relaxation
            model.set(GRB_IntParam_Method, GRB_METHOD_BARRIER);
        }

        model.set(GRB_DoubleParam_MIPGap, 0);

        // Solve
        model.optimize();

        // Print solution
        printf("%lf\n", model.get(GRB_DoubleAttr_ObjVal));

        if (cli_verbose) {
            const double integrality_tol = 0.00000001;
            printf("Open:");
            for (int i = 0; i < m; i++) {
                const double v = y[i].get(GRB_DoubleAttr_X);
                if (v >= -integrality_tol && v <= integrality_tol) {
                    //0
                } else if (v >= 1 - integrality_tol && v <= 1 + integrality_tol) {
                    printf(" %d", i);
                } else {
                    printf(" %d:NotInteger", i);
                }
            }
            printf("\n");
            for (int i = 0; i < m; i++) {
                printf("Transport[%d]:", i);
                for (int j = 0; j < n; j++) {
                    const double v = x[i * n + j].get(GRB_DoubleAttr_X);
                    if (v >= -integrality_tol && v <= integrality_tol) {
                        //0
                    } else if (v >= 1 - integrality_tol && v <= 1 + integrality_tol) {
                        printf(" %d", j);
                    } else {
                        printf(" %d:NotInteger", j);
                    }
                }
                printf("\n");
            }
        }
    } catch (GRBException e) {
        fprintf(stderr, "Error code = %d\n", e.getErrorCode());
        fprintf(stderr, "%s\n", e.getMessage().c_str());
    } catch (...) {
        fprintf(stderr, "Exception during optimization\n");
    }
    delete[] p;
    delete[] o;
    delete[] c;
    delete[] x;
    delete[] y;
    delete[] z;
    return 0;
}
