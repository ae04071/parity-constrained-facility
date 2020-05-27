#include <pcfl.h>
#include <list>
#include <tuple>
#include <memory>
#include <cmath>
#include <cstring>
#include <LPRelaxation.h>
#include <cassert>
#include <dbg.h>

#include "t-join/tjoin.h"

class BModel {
public:
    explicit BModel(const PCFLProbData *in_data, const PCFLConfig *in_config);
    BModel() = delete;
    ~BModel();
    double solve(bool *open, int *assign);

    double find_assignment_time;

private:
    GRBEnv env;
    GRBModel model;
    PCFLProbData data;
    PCFLConfig config;
    struct {
        GRBVar *x, *y;
        GRBVar *parity, *open_facility_parity;
    } vars;

    double (*find_assignment)
            (const struct PCFLProbData *data, const bool *open, int *to_facility, double cutoff);


    class Callback : public GRBCallback {
    public:
        explicit Callback(BModel *p_owner);
        Callback() = delete;
        ~Callback() override;
    protected:
        void callback() override;
    private:
        BModel *owner;
        bool *tmp_open;
        int *tmp_assign;

        void cutoff();
    };

    Callback callback;

    double sol_obj;
    bool *sol_open;
    int *sol_assign;

    double obj_best_bound;

    void init_vars();
    void init_constrs();
    void set_properties();

    friend void pcfl_impl4(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);
};

void pcfl_impl4(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol) {
    BModel model(data, config);
    sol->obj = model.solve(sol->open, sol->assign);
    sol->runtime = 0.0;
//    sol->status = model.model.get(GRB_IntAttr_Status);
    sol->status = GRB_OPTIMAL;
}

template<typename T>
static T *copy_array(T src[], size_t size) {
    T *ret = new T[size];
    memcpy(ret, src, size * sizeof(T));
    return ret;
}
BModel::BModel(const PCFLProbData *in_data, const PCFLConfig *in_config)
        : data(*in_data), config(*in_config), env(), model(this->env),
          vars(), callback(this),
          sol_obj(0), sol_open(nullptr), sol_assign(nullptr), obj_best_bound(),
          find_assignment_time(), find_assignment() {
    /*
     * initialization is in order of declaration
     * ref: https://en.cppreference.com/w/cpp/language/initializer_list
     */
    int m = in_data->m, n = in_data->n;
    this->data.parity = copy_array(in_data->parity, m);
    this->data.opening_cost = copy_array(in_data->opening_cost, m);
    this->data.assign_cost = copy_array(in_data->assign_cost, m * n);

    if (in_config->assignment_method < 1 || in_config->assignment_method > 3)
        throw std::runtime_error("invalid assignment method");
    this->find_assignment = pcfl_find_assignment_methods[in_config->assignment_method];

    this->init_vars();
    this->init_constrs();
    this->set_properties();
}

BModel::~BModel() {
    delete[] this->data.parity;
    delete[] this->data.opening_cost;
    delete[] this->data.assign_cost;
    delete[] this->vars.x;
    delete[] this->vars.y;
}

void BModel::init_vars() {
    int m = this->data.m, n = this->data.n;
    char buf[64];

    this->vars.x = this->model.addVars(m * n, GRB_BINARY);
    this->vars.y = this->model.addVars(m, GRB_BINARY);

    for (int i = 0; i < m; i++) {
        auto &v = this->vars.y[i];
        sprintf(buf, "y%d", i);
        v.set(GRB_DoubleAttr_Obj, this->data.opening_cost[i]);
        v.set(GRB_IntAttr_BranchPriority, 0);
        v.set(GRB_StringAttr_VarName, buf);
    }

    for (int i = 0; i < m; i++) {
//        auto xvars_i = this->vars.x + i * n;
        for (int j = 0; j < n; j++) {
            int index = i * this->data.n + j;
            auto &v = this->vars.x[index];
            sprintf(buf, "x%d.%d", i, j);
            v.set(GRB_DoubleAttr_Obj, this->data.assign_cost[index]);
            v.set(GRB_IntAttr_BranchPriority, 1000);
            v.set(GRB_StringAttr_VarName, buf);
        }
    }
}

void BModel::init_constrs() {
    const int m = this->data.m, n = this->data.n;
    char buf[64];

    // assign to only open facilities
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            sprintf(buf, "Open%d.%d", i, j);
            this->model.addConstr(
                    this->vars.x[i * this->data.n + j] <= this->vars.y[i], buf);
        }
    }

    // assign only once
    for (int j = 0; j < n; j++) {
        sprintf(buf, "AssignOnlyOnce%d", j);
        GRBLinExpr expr = 0;
        for (int i = 0; i < m; i++) {
            expr += this->vars.x[i * n + j];
        }
//        this->model.addConstr(expr == 1, buf);
        this->model.addConstr(expr >= 1, buf); // for unconstrained
    }
}

void BModel::set_properties() {
    this->model.set(GRB_IntParam_OutputFlag, 0);
    this->model.set(GRB_StringAttr_ModelName,
            "parity constrained facility location impl3");

    this->model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
    this->model.set(GRB_DoubleParam_MIPGap, 0);
    this->model.set(GRB_IntParam_LazyConstraints, 1);

//    this->model.set(GRB_IntParam_Threads, 1);
    this->model.setCallback(&this->callback);
}

double BModel::solve(bool *open, int *assign) {
    this->sol_obj = INFINITY;
    this->sol_open = open;
    this->sol_assign = assign;
    this->obj_best_bound = 0;

//    int m = this->data.m, n = this->data.n;

    this->find_assignment_time = 0;

	std::cerr << "RUN" << std::endl;
    this->model.optimize();
	std::cerr << "DONE" << std::endl;


	double *y = new double[this->data.m], *x = new double[this->data.m * this->data.n];
	for(int i=0;i<data.m;i++) {
		y[i] = vars.y[i].get(GRB_DoubleAttr_X);
		for(int j=0;j<data.n;j++)
			x[i*data.n+j] = vars.x[i*data.n+j].get(GRB_DoubleAttr_X);
	}
	tjoin::approximate_unconstraint(&data, y, x);

	for(int i=0;i<data.m;i++)
		open[i] = y[i] > 0.5;
	for(int i=0;i<data.m;i++) for(int j=0;j<data.n;j++) if(x[i*data.n + j] > 0.5) {
		assign[j] = i;
	}

    if (this->config.verbose) {
        auto &out = std::cout;
        out << std::fixed;
        out.precision(6);
        out << "find_assignment took: " << this->find_assignment_time << std::endl;
    }

	delete []y;
	delete []x;
    return this->sol_obj;
}

BModel::Callback::Callback(BModel *p_owner)
        : owner(p_owner), tmp_open(nullptr), tmp_assign(nullptr) {
    int m = this->owner->data.m, n = this->owner->data.n;
    this->tmp_open = new bool[m];
    this->tmp_assign = new int[n];
}
BModel::Callback::~Callback() {
    delete[] this->tmp_open;
    delete[] this->tmp_assign;
}

void BModel::Callback::callback() {
//    printf("callback: %d\n", where);
    switch (where) {
    case GRB_CB_MIP:
        this->owner->obj_best_bound = this->getDoubleInfo(GRB_CB_MIP_OBJBND);
        break;
    case GRB_CB_MIPSOL:
        this->owner->obj_best_bound = this->getDoubleInfo(GRB_CB_MIPSOL_OBJBND);
        break;
    case GRB_CB_MIPNODE:
        this->owner->obj_best_bound = this->getDoubleInfo(GRB_CB_MIPNODE_OBJBND);
        break;
    default:
        break;
    }
}

/*
 * start with unconstrained
 * CB_MIPSOL -> blossom v -> lazy constraint to prevent the same y, update cutoff (if can)
 * if best obj bound is worse than a feasible solution, abort
 *
 * (1)
 * may have to defer some solution from gurobi
 * some initial solution might consume very much time for blossom V.
 * defer initial solutions until a 'probable' solution is found,
 * apply blossom V to that solution first and then apply cutoff to deferred initial solutions.
 * an alternative way is to pre-solve using heuristic and set the cutoff.
 * using approximation method will be a good idea.
 *
 * concurrent method to solve this issue.
 * if the best bound of an assignment problem already running is worse than cutoff, abort the thread
 *
 * (3)
 * performance loss for easy problems
 * suspect that gurobi struggles finding the first solution.
 * check the assignment time or the callback time => (1) !!!!! it was solving vopen = 500 facilities for about 40s
 * check the number of infeasible open facilities => (2)
 *
 * (4) multi-thread, return to gurobi and run assignment concurrently
 *
 * TODO: find initial solution by approximation
 * TODO: concurrent method
 */
