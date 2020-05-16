//
// Created by jiho on 20. 4. 24..
//

#include <pcfl.h>
#include <list>
#include <tuple>
#include <memory>
#include <cmath>
#include <cstring>
#include <LPRelaxation.h>
#include <cassert>
#include <dbg.h>

class Model {
public:
    explicit Model(const PCFLProbData *in_data);
    Model() = delete;
    ~Model();
    double solve(bool *open, int *assign);

    double find_assignment_time;

private:
    GRBEnv env;
    GRBModel model;
    PCFLProbData data;
    GRBVar *xvars, *yvars;

    class Callback : public GRBCallback {
    public:
        explicit Callback(Model *p_owner);
        Callback() = delete;
        ~Callback() override;
    protected:
        void callback() override;
    private:
        Model *owner;
        bool *tmp_open;
        int *tmp_assign;

        void mipsol();
    };

    Callback callback;

    double sol_obj;
    bool *sol_open;
    int *sol_assign;

    void init_vars();

    void init_constrs();

    void set_properties();

    friend void pcfl_impl3(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);
};

void pcfl_impl3(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol) {
    Model model(data);
    sol->obj = model.solve(sol->open, sol->assign);
    sol->runtime = model.model.get(GRB_DoubleAttr_Runtime);
//    sol->status = model.model.get(GRB_IntAttr_Status);
    sol->status = GRB_OPTIMAL;

    if (config->verbose)
        std::cout << "find_assignment took: " << model.find_assignment_time << std::endl;
}

template<typename T>
static T *copy_array(T src[], size_t size) {
    T *ret = new T[size];
    memcpy(ret, src, size * sizeof(T));
    return ret;
}
Model::Model(const PCFLProbData *in_data)
        : data(*in_data), env(), model(this->env),
          xvars(nullptr), yvars(nullptr), callback(this),
          sol_obj(0), sol_open(nullptr), sol_assign(nullptr), find_assignment_time() {
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

Model::~Model() {
    delete[] this->data.parity;
    delete[] this->data.opening_cost;
    delete[] this->data.assign_cost;
    delete[] this->xvars;
    delete[] this->yvars;
}

void Model::init_vars() {
    int m = this->data.m, n = this->data.n;
    char buf[64];

    this->xvars = this->model.addVars(m * n, GRB_BINARY);
    this->yvars = this->model.addVars(m, GRB_BINARY);

    for (int i = 0; i < m; i++) {
        auto &v = this->yvars[i];
        sprintf(buf, "y%d", i);
        v.set(GRB_DoubleAttr_Obj, this->data.opening_cost[i]);
        v.set(GRB_StringAttr_VarName, buf);
        v.set(GRB_IntAttr_BranchPriority, 1);
    }

    for (int i = 0; i < m; i++) {
//        auto xvars_i = this->xvars + i * n;
        for (int j = 0; j < n; j++) {
            int index = i * this->data.n + j;
            auto &v = this->xvars[index];
            sprintf(buf, "x%d.%d", i, j);
            v.set(GRB_DoubleAttr_Obj, this->data.assign_cost[index]);
            v.set(GRB_StringAttr_VarName, buf);
            v.set(GRB_IntAttr_BranchPriority, 2);
        }
    }
}

void Model::init_constrs() {
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
void Model::set_properties() {
    this->model.set(GRB_IntParam_OutputFlag, 0);
    this->model.set(GRB_StringAttr_ModelName,
            "parity constrained facility location impl3");

    this->model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
    this->model.set(GRB_DoubleParam_MIPGap, 0);
    this->model.set(GRB_IntParam_LazyConstraints, 1);

//    this->model.set(GRB_IntParam_Threads, 1);
    this->model.setCallback(&this->callback);
}

double Model::solve(bool *open, int *assign) {
    this->sol_obj = INFINITY;
    this->sol_open = open;
    this->sol_assign = assign;

//    int m = this->data.m, n = this->data.n;

    this->find_assignment_time = {};

    this->model.optimize();

    return this->sol_obj;
}

Model::Callback::Callback(Model *p_owner)
        : owner(p_owner), tmp_open(nullptr), tmp_assign(nullptr) {
    int m = this->owner->data.m, n = this->owner->data.n;
    this->tmp_open = new bool[m];
    this->tmp_assign = new int[n];
}
Model::Callback::~Callback() {
    delete[] this->tmp_open;
    delete[] this->tmp_assign;
}

void Model::Callback::callback() {
//    printf("callback: %d\n", where);
    switch (where) {
    case GRB_CB_MIP:
        if (this->getDoubleInfo(GRB_CB_MIP_OBJBND) >= this->owner->sol_obj)
            this->abort();
        break;
    case GRB_CB_MIPSOL:
        if (this->getDoubleInfo(GRB_CB_MIPSOL_OBJBND) >= this->owner->sol_obj)
            this->abort();
        this->mipsol();
        break;
    case GRB_CB_MIPNODE:
        if (this->getDoubleInfo(GRB_CB_MIPNODE_OBJBND) >= this->owner->sol_obj)
            this->abort();
        break;
    }
}

void Model::Callback::mipsol() {
    int m = this->owner->data.m, n = this->owner->data.n;
    if (this->getDoubleInfo(GRB_CB_MIPSOL_OBJ) < this->owner->sol_obj) {
        std::unique_ptr<double[]> y_val(this->getSolution(this->owner->yvars, m));
        for (int i = 0; i < m; i++) {
            assert(fmin(fabs(y_val[i]), fabs(y_val[i] - 1)) < 1e-10);
            this->tmp_open[i] = y_val[i] >= 0.5;
        }
        time_measure tm1;
        double obj = pcfl_find_assignment(&this->owner->data, this->tmp_open, this->tmp_assign, this->owner->sol_obj);
        this->owner->find_assignment_time += tm1.get();
        if (obj < this->owner->sol_obj) {
            this->owner->sol_obj = obj;
            memcpy(this->owner->sol_open, this->tmp_open, m * sizeof(*this->owner->sol_open));
            memcpy(this->owner->sol_assign, this->tmp_assign, n * sizeof(*this->owner->sol_assign));
            //cutoff..
        }
    }
    {
        GRBLinExpr expr = 0;
        for (int i = 0; i < m; i++) {
            if (this->tmp_open[i]) {
                expr += 1 - this->owner->yvars[i];
            } else {
                expr += this->owner->yvars[i];
            }
        }
        this->addLazy(expr >= 1);
    }
}


/*
 * start with unconstrained
 * CB_MIPSOL -> blossom v -> lazy constraint to prevent the same y, update cutoff (if can)
 * if best obj bound is worse than a feasible solution, abort
 *
 * may have to defer some solution from gurobi
 * some initial solution might consume very much time for blossom V.
 * defer initial solutions until a 'probable' solution is found,
 * apply blossom V to that solution first and then apply cutoff to deferred initial solutions.
 * an alternative way is to pre-solve using heuristic and set the cutoff.
 * using approximation method will be a good idea.
 */
