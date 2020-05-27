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
    explicit Model(const PCFLProbData *in_data, const PCFLConfig *in_config);
    Model() = delete;
    ~Model();
    double solve(bool *open, int *assign, bool initial=false);

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

        void cutoff();
    };

    Callback callback;

    double sol_obj;
    bool *sol_open;
    int *sol_assign;

    double obj_best_bound;

    void init_vars();
    void init_constrs();
    void init_cuts();
    void set_properties();

    friend void pcfl_impl3(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);
    friend void pcfl_impl3_with_initial(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);
};

void pcfl_impl3(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol) {
    Model model(data, config);
    sol->obj = model.solve(sol->open, sol->assign);
    sol->runtime = model.model.get(GRB_DoubleAttr_Runtime);
//    sol->status = model.model.get(GRB_IntAttr_Status);
    sol->status = GRB_OPTIMAL;
}

void pcfl_impl3_with_initial(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol) {
    Model model(data, config);
    sol->obj = model.solve(sol->open, sol->assign, true);
    sol->runtime = model.model.get(GRB_DoubleAttr_Runtime);
//    sol->status = model.model.get(GRB_IntAttr_Status);
    sol->status = GRB_OPTIMAL;
}

template<typename T>
static T *copy_array(T src[], size_t size) {
    T *ret = new T[size];
    memcpy(ret, src, size * sizeof(T));
    return ret;
}
Model::Model(const PCFLProbData *in_data, const PCFLConfig *in_config)
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
    this->init_cuts();
    this->set_properties();
}

Model::~Model() {
    delete[] this->data.parity;
    delete[] this->data.opening_cost;
    delete[] this->data.assign_cost;
    delete[] this->vars.x;
    delete[] this->vars.y;
    delete[] this->vars.parity;
    delete this->vars.open_facility_parity;
}

void Model::init_vars() {
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

void Model::init_constrs() {
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

void Model::init_cuts() {
    int m = this->data.m, n = this->data.n;
    char buf[64];

    if (this->config.impl3_use_parity) {
        this->vars.parity = this->model.addVars(m, GRB_INTEGER);
        for (int i = 0; i < m; i++) {
            auto &v = this->vars.parity[i];
            int parity = this->data.parity[i];
            sprintf(buf, "parity_var%d", i);
            v.set(GRB_DoubleAttr_LB, 0);
            v.set(GRB_DoubleAttr_UB, (int)((n - (2 - parity)) / 2));//2v <= n for even, 2v <= n - 1 for odd
            v.set(GRB_DoubleAttr_Obj, 0);
            v.set(GRB_IntAttr_BranchPriority, 2000);
            v.set(GRB_StringAttr_VarName, buf);
            if (parity != 0) {
                GRBLinExpr expr = 0;
                sprintf(buf, "Parity(%d)%d", parity, i);
                GRBVar *xvars = this->vars.x + i * n;
                for (int j = 0; j < n; j++) {
                    expr += xvars[j];
                }
                if (parity == 1)
                    expr -= this->vars.y[i];
                this->model.addConstr(expr == v * 2, buf);
            }
        }
    }
    if (this->config.use_open_parity) {
        this->vars.open_facility_parity = this->model.addVars(1, GRB_INTEGER);
        auto &v = *this->vars.open_facility_parity;
        {
            //-n - (num of odds) <= 2v <= -n
            int num_odd = 0;
            for (int i = 0; i < m; i++) {
                num_odd += this->data.parity[i] == 1;
            }
            v.set(GRB_DoubleAttr_LB, (int)((-n - num_odd) / 2));//floor towards 0
            v.set(GRB_DoubleAttr_UB, (int)((-n - 1) / 2));//ceil towards -inf
        }
        v.set(GRB_DoubleAttr_Obj, 0);
        v.set(GRB_IntAttr_BranchPriority, 500);
        v.set(GRB_StringAttr_VarName, "open_facility_parity_var");
        //0 <= n + (sum of odd y) + (even) <= (sum of unconstrained y)
        GRBLinExpr e1 = n + v * 2, e2 = 0;
        for (int i = 0; i < m; i++) {
            int parity = this->data.parity[i];
            auto &yv = this->vars.y[i];
            if (parity == 0)
                e2 += yv;
            else if (parity == 1)
                e1 += yv;
        }
        this->model.addConstr(e1 >= 0, "open_facility_parity_lb");
        this->model.addConstr(e1 <= e2, "open_facility_parity_ub");
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

double Model::solve(bool *open, int *assign, bool initial) {
    this->sol_obj = INFINITY;
    this->sol_open = open;
    this->sol_assign = assign;
    this->obj_best_bound = 0;

//    int m = this->data.m, n = this->data.n;

    this->find_assignment_time = 0;

	if(initial) {
		for(int i=0; i<data.m; i++) {
			int cnt = 0;
			vars.y[i].set(GRB_DoubleAttr_Start, open[i] ? 1.0 : 0.0);
			for(int j=0; j<data.n; j++) {
				vars.x[i*data.n + j].set(GRB_DoubleAttr_Start, assign[j]==i ? 1.0 : 0.0);
				cnt += assign[j] == i;
			}
			if(data.parity[i] != 0) {
				vars.parity[i].set(GRB_DoubleAttr_Start, std::max(0, cnt-data.parity[i]%2)/2);
			}
		}
	}
	
    this->model.optimize();

    if (this->config.verbose) {
        auto &out = std::cout;
        out << std::fixed;
        out.precision(6);
        out << "find_assignment took: " << this->find_assignment_time << std::endl;
    }

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
        this->owner->obj_best_bound = this->getDoubleInfo(GRB_CB_MIP_OBJBND);
        break;
    case GRB_CB_MIPSOL:
        this->owner->obj_best_bound = this->getDoubleInfo(GRB_CB_MIPSOL_OBJBND);
        this->cutoff();
        this->mipsol();
        break;
    case GRB_CB_MIPNODE:
        this->owner->obj_best_bound = this->getDoubleInfo(GRB_CB_MIPNODE_OBJBND);
        break;
    default:
        break;
    }
    this->cutoff();
}

void Model::Callback::mipsol() {
    int m = this->owner->data.m, n = this->owner->data.n;
    if (this->getDoubleInfo(GRB_CB_MIPSOL_OBJ) < this->owner->sol_obj) {
        std::unique_ptr<double[]> y_val(this->getSolution(this->owner->vars.y, m));
        for (int i = 0; i < m; i++) {
            assert(fmin(fabs(y_val[i]), fabs(y_val[i] - 1)) < this->owner->model.get(GRB_DoubleParam_IntFeasTol));
            this->tmp_open[i] = y_val[i] >= 0.5;
        }
        time_measure tm1;
        double obj = this->owner->find_assignment(&this->owner->data, this->tmp_open, this->tmp_assign, this->owner->sol_obj);
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
                expr += 1 - this->owner->vars.y[i];
            } else {
                expr += this->owner->vars.y[i];
            }
        }
        this->addLazy(expr >= 1);
    }
}

void Model::Callback::cutoff() {
    if (this->owner->sol_obj <= this->owner->obj_best_bound)
        this->abort();
}

double (*pcfl_find_assignment_methods[])
        (const struct PCFLProbData *data, const bool *open, int *to_facility, double cutoff) = {
        nullptr,
        pcfl_find_assignment1,
        pcfl_find_assignment2,
        pcfl_find_assignment3,
};

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
