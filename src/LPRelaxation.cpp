//
// Created by jiho on 20. 4. 29..
//

#include <LPRelaxation.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define OPTIMALITY_TOL 1e-9

template<typename T>
static T *copy_array(T src[], size_t size) {
    T *ret = new T[size];
    memcpy(ret, src, size * sizeof(T));
    return ret;
}
LPRelaxation::LPRelaxation(const PCFLProbData *in_data)
        : data(*in_data), env(), model(this->env),
        xvars(nullptr), yvars(nullptr), sumy_var(nullptr), callback(this) {
    /*
     * initialization is in order of declaration
     * ref: https://en.cppreference.com/w/cpp/language/initializer_list
     */
    int m = in_data->m, n = in_data->n;
    this->data.parity = copy_array(in_data->parity, m);
    this->data.opening_cost = copy_array(in_data->opening_cost, m);
    this->data.assign_cost = copy_array(in_data->assign_cost, m * n);

    this->init_vars();
    this->init_constrs();
    this->set_properties();
}

LPRelaxation::~LPRelaxation() {
    delete[] this->data.parity;
    delete[] this->data.opening_cost;
    delete[] this->data.assign_cost;
    delete[] this->xvars;
    delete[] this->yvars;
    delete this->sumy_var;
}

void LPRelaxation::init_vars() {
    int m = this->data.m, n = this->data.n;
    char buf[64];

    this->xvars = this->model.addVars(m * n);
    this->yvars = this->model.addVars(m);
    this->sumy_var = new GRBVar(this->model.addVar(1, m, 0, GRB_CONTINUOUS));

    for (int i = 0; i < m; i++) {
        auto &v = this->yvars[i];
        sprintf(buf, "y%d", i);
        v.set(GRB_DoubleAttr_Obj, this->data.opening_cost[i]);
        v.set(GRB_StringAttr_VarName, buf);
    }

    for (int i = 0; i < m; i++) {
        auto xvars_i = this->xvars + i * n;
        for (int j = 0; j < n; j++) {
            int index = i * this->data.n + j;
            auto &v = this->xvars[index];
            sprintf(buf, "x%d.%d", i, j);
            v.set(GRB_DoubleAttr_Obj, this->data.assign_cost[index]);
            v.set(GRB_StringAttr_VarName, buf);
            v.set(GRB_DoubleAttr_LB, 0);
            v.set(GRB_DoubleAttr_UB, 1);
        }
    }

    {
        auto &v = *this->sumy_var;
        GRBLinExpr expr = 0;
        for (int i = 0; i < m; i++) {
            expr += this->yvars[i];
        }
        this->model.addConstr(v == expr);
    }
}

void LPRelaxation::init_constrs() {
    char buf[64];
    const int m = this->data.m, n = this->data.n;

    // assign to only open facilities
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            sprintf(buf, "Open%d.%d", i, j);
            this->model.addConstr(
                    this->xvars[i * this->data.n + j] <= this->yvars[i], buf);
        }
    }

//    // parity
//    if (!this->config.lazy_parity) {
//        for (int i = 0; i < m; i++) {
//            if (this->data.parity[i] != 0) {
//                sprintf(buf, "Parity(%d)%d", this->data.parity[i], i);
//                this->model.addConstr(this->genParityConstr(i), buf);
//            }
//        }
//    }

    // assign only once
    for (int j = 0; j < n; j++) {
        sprintf(buf, "AssignOnlyOnce%d", j);
        GRBLinExpr expr = 0;
        for (int i = 0; i < m; i++) {
            expr += this->xvars[i * n + j];
        }
//        this->model.addConstr(expr == 1, buf);
        this->model.addConstr(expr >= 1, buf); // for unconstrained
    }
}
void LPRelaxation::set_properties() {
//    this->model.set(GRB_IntParam_OutputFlag, 0);
    this->model.set(GRB_StringAttr_ModelName,
            "parity constrained facility location LPRelaxation subroutine");

    this->model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
//    this->model.set(GRB_IntParam_Method, GRB_METHOD_BARRIER);
    this->model.set(GRB_IntParam_Method, GRB_METHOD_PRIMAL);
//    this->model.set(GRB_IntParam_Method, GRB_METHOD_CONCURRENT);
    this->model.set(GRB_DoubleParam_OptimalityTol, OPTIMALITY_TOL);

//    this->model.set(GRB_IntParam_Threads, 1);
    this->model.setCallback(&this->callback);
}

bool LPRelaxation::bound_func(const bool *open, const bool *mask,
        double cutoff, int open_lb, int open_ub) {
    if (isinf(cutoff))
        return true;
//    printf("bound_func\n");
    int m = this->data.m, n = this->data.n;
//    this->model.reset();
    bool open_constr = open != nullptr && mask != nullptr;
    for (int i = 0; i < m; i++) {
        this->yvars[i].set(GRB_DoubleAttr_LB, (open_constr && (mask[i] & open[i])) ? 1 : 0);
        this->yvars[i].set(GRB_DoubleAttr_UB, (open_constr && (mask[i] & !open[i])) ? 0 : 1);
    }
    this->sumy_var->set(GRB_DoubleAttr_LB, open_lb);
    this->sumy_var->set(GRB_DoubleAttr_UB, open_ub);
    this->model.set(GRB_DoubleParam_Cutoff, cutoff);
    assert(!this->model.get(GRB_IntAttr_IsMIP));
    this->model.optimize();
//    printf(" result: %d\n", this->model.get(GRB_IntAttr_Status));
    printf(" time: %lf\n", this->model.get(GRB_DoubleAttr_Runtime));
    return this->model.get(GRB_IntAttr_Status) != GRB_CUTOFF;
    /*
     * TODO: should exit if a solution at least as good as cutoff
     */
}

LPRelaxation::Callback::Callback(LPRelaxation *p_owner) : owner(p_owner) {}

void LPRelaxation::Callback::callback() {
//    printf("callback: %d\n", where);
    switch (where) {
    case GRB_CB_BARRIER:
        printf("Barrier(%d, %lf, %lf, %lf, %lf, %lf)\n",
                this->getIntInfo(GRB_CB_BARRIER_ITRCNT),
                this->getDoubleInfo(GRB_CB_BARRIER_PRIMOBJ),
                this->getDoubleInfo(GRB_CB_BARRIER_DUALOBJ),
                this->getDoubleInfo(GRB_CB_BARRIER_PRIMINF),
                this->getDoubleInfo(GRB_CB_BARRIER_DUALINF),
                this->getDoubleInfo(GRB_CB_BARRIER_COMPL));
        break;
    case GRB_CB_SIMPLEX:
        printf("Simplex(%d, %lf, %lf, %lf, %d)\n",
                (int)this->getDoubleInfo(GRB_CB_SPX_ITRCNT),
                this->getDoubleInfo(GRB_CB_SPX_OBJVAL),
                this->getDoubleInfo(GRB_CB_SPX_PRIMINF),
                this->getDoubleInfo(GRB_CB_SPX_DUALINF),
                this->getIntInfo(GRB_CB_SPX_ISPERT));
        break;
    }
}
