#include <pcfl_model.h>
#include <cstring>
#include <cmath>
#include <memory>
#include <algorithm>

void pcfl_impl1(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol) {
    try {
        // Model
        PCFLModel model;
        model.configModel(config);
        model.constructModel(data);

        // Solve
        model.optimize();

        for (int i = 0; i < data->m; i++) {
            sol->open[i] = model.getY(i);
            for (int j = 0; j < data->n; j++) {
                if (model.getX(i, j))
                    sol->assign[j] = i;
            }
        }

        if (config->validate_feasibility) {
            for (int j = 0; j < data->n; j++) {
                int cnt = 0;
                for (int i = 0; i < data->m; i++) {
                    if (model.getX(i, j))
                        cnt++;
                }
                if (cnt != 1)
                    printf("assign cnt violation(%d: %d assigns)\n", j, cnt);
            }
        }

        sol->obj = model.get(GRB_DoubleAttr_ObjVal);
        sol->runtime = model.get(GRB_DoubleAttr_Runtime);
        sol->status = model.get(GRB_IntAttr_Status);

        return;
    } catch (const GRBException &e) {
        fprintf(stderr, "Error code = %d\n", e.getErrorCode());
        fprintf(stderr, "%s\n", e.getMessage().c_str());
    } catch (...) {
        fprintf(stderr, "Exception during optimization\n");
    }
    sol->obj = NAN;
}

/* Model Definition */
PCFLModel::PCFLModel()
	    : m_env(true), data(), GRBModel(false), m_callback(this),
        xvar(nullptr), yvar(nullptr), zvar(nullptr), config()
{
}

void PCFLModel::constructModel(const ProbData *d) {
    this->data.m = d->m;
    this->data.n = d->n;
    this->data.parity = new int[d->m];
    this->data.opening_cost = new double[d->m];
    this->data.assign_cost = new double[d->m * d->n];
    memcpy(this->data.parity, d->parity, d->m * sizeof(int));
    memcpy(this->data.opening_cost, d->opening_cost, d->m * sizeof(double));
    memcpy(this->data.assign_cost, d->assign_cost, d->m * d->n * sizeof(double));

    this->init_vars();
    this->init_constraints();

    this->setVariableProperties();
}

void PCFLModel::init_vars() {
    char buf[64];
    this->yvar = this->addVars(this->data.m, GRB_BINARY);
    this->zvar = this->addVars(this->data.m, GRB_INTEGER);
    for (int i = 0; i < this->data.m; i++) {
        {
            auto &v = this->yvar[i];
            sprintf(buf, "y%d", i);
            v.set(GRB_DoubleAttr_Obj, this->data.opening_cost[i]);
            v.set(GRB_StringAttr_VarName, buf);
        }
        {
            auto &v = this->zvar[i];
            sprintf(buf, "z%d", i);
            v.set(GRB_DoubleAttr_LB, 0);
            v.set(GRB_DoubleAttr_UB, (int)((this->data.n - (2 - this->data.parity[i])) / 2));
            v.set(GRB_DoubleAttr_Obj, 0);
            v.set(GRB_StringAttr_VarName, buf);
        }
    }

    this->xvar = this->addVars(this->data.m * this->data.n, GRB_BINARY);
    for (int i = 0; i < this->data.m; i++) {
        for (int j = 0; j < this->data.n; j++) {
            int index = i * this->data.n + j;
            auto &v = this->xvar[index];
            sprintf(buf, "x%d.%d", i, j);
            v.set(GRB_DoubleAttr_Obj, this->data.assign_cost[index]);
            v.set(GRB_StringAttr_VarName, buf);
        }
    }
}

void PCFLModel::init_constraints() {
    char buf[64];
    const int m = this->data.m, n = this->data.n;

    // assign to only open facilities
    if (!this->config.lazy_open) {
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                sprintf(buf, "Open%d.%d", i, j);
                this->addConstr(this->genOpenConstr(i, j), buf);
            }
        }
    } else {
        this->criticalOpenConstrs((int)fmin(pow(m * n, 0.95), m * n));
    }

    // parity
    if (!this->config.lazy_parity) {
        for (int i = 0; i < m; i++) {
            if (this->data.parity[i] != 0) {
                sprintf(buf, "Parity(%d)%d", this->data.parity[i], i);
                this->addConstr(this->genParityConstr(i), buf);
            }
        }
    }

    // assign only once
    for (int j = 0; j < n; j++) {
        sprintf(buf, "AssignOnlyOnce%d", j);
        GRBLinExpr expr = 0;
        for (int i = 0; i < m; i++) {
            expr += this->xvar[i * n + j];
        }
        this->addConstr(expr == 1, buf);
    }
}

GRBTempConstr PCFLModel::genParityConstr(int i) const {
    GRBLinExpr expr = 0;
    int n = this->data.n;
    GRBVar *xvars = this->xvar + i * n;
    for (int j = 0; j < n; j++) {
        expr += xvars[j];
    }
    if (this->data.parity[i] == 1)
        expr -= this->yvar[i];
    return expr == this->zvar[i] * 2;
}

GRBTempConstr PCFLModel::genOpenConstr(int i, int j) const {
    return this->xvar[i * this->data.n + j] <= this->yvar[i];
}

void PCFLModel::setVariableProperties() {
    this->set(GRB_StringAttr_ModelName,
            "parity constrained facility location");
//	this->set(GRB_IntParam_Threads, std::thread::hardware_concurrency());
    this->set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
    this->set(GRB_DoubleParam_MIPGap, 0);

    this->set(GRB_IntParam_OutputFlag, 0);
//	this->set(GRB_DoubleParam_Heuristics, 0.0);
    this->set(GRB_IntParam_LazyConstraints, 1);

    this->setCallback(&this->m_callback);

    for (int i = 0; i < this->data.m * this->data.n; i++) {
        this->xvar[i].set(GRB_IntAttr_BranchPriority, config.xprior);
    }
    for (int i = 0; i < this->data.m; i++) {
        this->yvar[i].set(GRB_IntAttr_BranchPriority, config.yprior);
        this->zvar[i].set(GRB_IntAttr_BranchPriority, config.zprior);
    }
    this->set(GRB_IntParam_Threads, config.threads);
    if (config.time_limit >= 0)
        this->set(GRB_DoubleParam_TimeLimit, config.time_limit);
}

void PCFLModel::configModel(const struct PCFLConfig *c) {
    this->config = *c;
}

PCFLModel::~PCFLModel() {
    delete[] this->xvar;
    delete[] this->yvar;
    delete[] this->zvar;
}

bool PCFLModel::getX(int i, int j, double *raw) const {
    double x = this->xvar[i * this->data.n + j].get(GRB_DoubleAttr_X);
    if (raw)
        *raw = x;
    return x >= 0.5;
}
bool PCFLModel::getY(int i, double *raw) const {
    double y = this->yvar[i].get(GRB_DoubleAttr_X);
    if (raw)
        *raw = y;
    return y >= 0.5;
}

void PCFLModel::criticalOpenConstrs(int cnt) {
    struct Edge {
        int i, j;
        double cost;
    };
    const int m = this->data.m, n = this->data.n;
    std::unique_ptr<struct Edge[]> edges(new struct Edge[m * n]);
    for (int i = 0, k = 0; i < m; i++) {
        for (int j = 0; j < n; j++, k++) {
            edges[k] = {i, j, this->data.assign_cost[k]};
        }
    }
    std::sort(edges.get(), edges.get() + m * n,
            [](const struct Edge &a, const struct Edge &b) {
        return a.cost < b.cost;
    });
    for (int k = 0; k < cnt; k++) {
        int i = edges[k].i, j = edges[k].j;
        this->addConstr(this->genOpenConstr(i, j));
    }
}
