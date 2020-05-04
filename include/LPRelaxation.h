//
// Created by jiho on 20. 4. 29..
//

#ifndef PCFL_LPRELAXATION_H
#define PCFL_LPRELAXATION_H


#include <pcfl.h>
#include <gurobi_c++.h>

class LPRelaxation {
public:
    explicit LPRelaxation(const PCFLProbData *in_data);
    LPRelaxation() = delete;
    ~LPRelaxation();
    bool bound_func(const bool *open, const bool *mask, double cutoff,
            int open_lb, int open_ub);

private:
    GRBEnv env;
    GRBModel model;
    PCFLProbData data;
    GRBVar *xvars, *yvars, *sumy_var;

    class Callback : public GRBCallback {
    public:
        explicit Callback(LPRelaxation *p_owner);
        Callback() = delete;
    protected:
        void callback() override;
    private:
        LPRelaxation *owner;
    };

    Callback callback;

    void init_vars();

    void init_constrs();

    void set_properties();
};


#endif //PCFL_LPRELAXATION_H
