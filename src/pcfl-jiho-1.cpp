//
// Created by jiho on 20. 3. 31..
//

#include <gurobi_c++.h>
#include <stdio.h>
#include <string.h>
#include <thread>

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "--build-only") == 0) {
        return 0;
    }
    GRBVar *open = NULL;
    GRBVar *transport = NULL;
    GRBVar *parity = NULL;

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

        model.set(GRB_IntParam_Threads, std::thread::hardware_concurrency());

        // Prepare vars
        open = model.addVars(m, GRB_BINARY);
        parity = model.addVars(m, GRB_INTEGER);
        for (int i = 0; i < m; i++) {
            {
                auto &v = open[i];
                sprintf(buf, "Open%d", i);
                v.set(GRB_DoubleAttr_Obj, o[i]);
                v.set(GRB_StringAttr_VarName, buf);
            }
            {
                auto &v = parity[i];
                sprintf(buf, "Parity%d", i);
                v.set(GRB_DoubleAttr_Obj, 0);
                v.set(GRB_StringAttr_VarName, buf);
            }
        }

        transport = model.addVars(m * n, GRB_BINARY);
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                int index = i * n + j;
                auto &v = transport[index];
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
                model.addConstr(transport[i * n + j] <= open[i], buf);
            }
        }

        // Parity
        for (int i = 0; i < m; i++) {
            GRBLinExpr expr = 0;
            if (p[i]) {
                for (int j = 0; j < n; j++) {
                    expr += transport[i * n + j];
                }
                if (p[i] == 1) {
                    sprintf(buf, "Odd%d", i);
                    model.addConstr(parity[i] * 2 == (expr + open[i]), buf);
                } else {
                    sprintf(buf, "Even%d", i);
                    model.addConstr(parity[i] * 2 == expr, buf);
                }
            }
        }

        // Client
        for (int j = 0; j < n; j++) {
            sprintf(buf, "Client%d", j);
            GRBLinExpr expr = 0;
            for (int i = 0; i < m; i++) {
                expr += transport[i * n + j]; // FIXME: potential cache miss risk
            }
            model.addConstr(expr == 1, buf);
        }

        // Use barrier to solve root relaxation
        model.set(GRB_IntParam_Method, GRB_METHOD_BARRIER);

        // Solve
        model.optimize();

        // Print solution
        printf("%lf\n", model.get(GRB_DoubleAttr_ObjVal));

//        for (int i = 0; i < m; i++) {
//            printf(i ? " %lf" : "%lf", open[i].get(GRB_DoubleAttr_X));
//        }
//        printf("\n");
//        for (int i = 0; i < m; i++) {
//            for (int j = 0; j < n; j++) {
//                printf(j ? " %lf" : "%lf", transport[i * n + j].get(GRB_DoubleAttr_X));
//            }
//            printf("\n");
//        }
    } catch (GRBException e) {
        fprintf(stderr, "Error code = %d\n", e.getErrorCode());
        fprintf(stderr, "%s\n", e.getMessage().c_str());
    } catch (...) {
        fprintf(stderr, "Exception during optimization\n");
    }
    delete[] p;
    delete[] o;
    delete[] c;
    delete[] open;
    delete[] transport;
    return 0;
}
