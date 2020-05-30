//
// Created by jiho on 20. 5. 27..
//

#include "common.h"
#include <list>
#include <memory>
#include <cmath>
#include <cstring>
#include <cassert>
#include <dbg.h>

Model::Callback::Callback(Model *p_owner)
        : owner(p_owner) {
//    int m = this->owner->data.m, n = this->owner->data.n;
//    this->tmp_open = new bool[m];
//    this->tmp_assign = new int[n];
}
Model::Callback::~Callback() {
//    delete[] this->tmp_open;
//    delete[] this->tmp_assign;
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
        bool *tmp_open = new bool[m]; // this may leak when aborting, but not serious
        for (int i = 0; i < m; i++) {
            assert(fmin(fabs(y_val[i]), fabs(y_val[i] - 1)) < this->owner->model.get(GRB_DoubleParam_IntFeasTol));
            tmp_open[i] = y_val[i] >= 0.5;
        }

        /* Add lazy constraint */
        {
            GRBLinExpr expr = 0;
            for (int i = 0; i < m; i++) {
                if (tmp_open[i]) {
                    expr += 1 - this->owner->vars.y[i];
                } else {
                    expr += this->owner->vars.y[i];
                }
            }
            this->addLazy(expr >= 1);
        }

        /* Assign func */
        auto func = [this, m, n, tmp_open]() {
            bool verbose = this->owner->config.verbose;
            auto &out = std::clog;
            if (verbose) {
                out << std::fixed;
                out.precision(6);
                out << "find_assignment" << std::endl;
                out << " Open:";
                for (int i = 0; i < m; i++) {
                    if (tmp_open[i]) {
                        out << " " << i;
                    }
                }
                out << std::endl;
            }

            {
                time_measure tm1;
                std::vector<int> tmp_assign_buf(n);
                int *tmp_assign = tmp_assign_buf.data();
                double obj;
                obj = this->owner->find_assignment(&this->owner->data, tmp_open, tmp_assign,
                        this->owner->sol_obj);
                double assign_time = tm1.get();
                if (verbose) {
                    out << " obj: " << obj << std::endl;
                    out << " time: " << assign_time << "s" << std::endl;
                }
                {
                    std::unique_lock<std::mutex> lock(this->owner->sol_mutex);
                    this->owner->find_assignment_time += assign_time;
                    if (obj < this->owner->sol_obj) {
                        this->owner->sol_obj = obj;
                        memcpy(this->owner->sol_open, tmp_open, m * sizeof(*this->owner->sol_open));
                        memcpy(this->owner->sol_assign, tmp_assign, n * sizeof(*this->owner->sol_assign));
                        lock.unlock();
                        this->cutoff();
                    }
                }
            }

            delete[] tmp_open;
        };

        if (this->owner->thread_pool != nullptr) {
            this->owner->thread_pool->post(func, -this->getDoubleInfo(GRB_CB_MIPSOL_OBJ));
            if (this->owner->config.verbose) {
                size_t qsz = this->owner->thread_pool->queue_size();
                if (qsz > 1)
                    std::clog << "ThreadPool queue size: " << qsz << std::endl;
            }
        } else {
            func();
        }
    }
}

void Model::Callback::cutoff() {
    if (this->owner->sol_obj <= this->owner->obj_best_bound)
        this->owner->model.terminate();
}
