#ifndef __PCFL_MODEL_H__
#define __PCFL_MODEL_H__

#include "pcfl.h"
#include <gurobi_c++.h>

/* Model Class */
class PCFLModel : public GRBModel {
private:

public:
    typedef struct PCFLProbData ProbData;

private:
    class Callback: public GRBCallback {
    private:
        PCFLModel *model;

    public:
        explicit Callback(PCFLModel *_model);

    protected:
        void callback() override;
    };
    friend Callback;

private:
    GRBEnv 		m_env;
    struct PCFLConfig config;
    Callback m_callback;

    ProbData data;
    GRBVar      *xvar, *yvar, *zvar;

public:
	explicit PCFLModel();
	
	void constructModel(const ProbData *d);

	void configModel(const struct PCFLConfig *c);

	~PCFLModel();

	bool getX(int i, int j, double *raw = nullptr) const;
	bool getY(int i, double *raw = nullptr) const;

private:
    void init_vars();
    void init_constraints();
    void setVariableProperties();

    GRBTempConstr genParityConstr(int i) const;
    GRBTempConstr genOpenConstr(int i, int j) const;

    void criticalOpenConstrs(int cnt);

};

#endif
