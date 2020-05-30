//
// Created by jiho on 20. 5. 27..
//

#ifndef PCFL_COMMON_H
#define PCFL_COMMON_H

#include <gurobi_c++.h>
#include <pcfl.h>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>

class ThreadPool {
public:
    explicit ThreadPool(size_t number_of_threads, bool open = true);
    ~ThreadPool();

    void *post(const std::function<void()> &func, double priority);
    void *post(std::function<void()> &&func, double priority);

    void join(); // close();
    void abort();
    void join(void *task); //omit
    void abort(void *task); //omit

    void open();
    void close();

    size_t queue_size() const;

private:
    void worker_func(intptr_t tid);

private:
    unsigned long flags;
    struct Task {
        std::function<void()> func;
        double priority;
    };
    friend bool operator<(const Task &a, const Task &b);
    std::priority_queue<Task> queue;
    std::mutex mutex;
    std::condition_variable condition;
    std::vector<std::thread> workers;
};

class Model {
public:
    explicit Model(const PCFLProbData *in_data, const PCFLConfig *in_config);
    Model() = delete;
    ~Model();
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
        explicit Callback(Model *p_owner);
        Callback() = delete;
        ~Callback() override;
    protected:
        void callback() override;
    private:
        Model *owner;
//        bool *tmp_open;
//        int *tmp_assign;

        void mipsol();

        void cutoff();
    };

    Callback callback;

    double sol_obj;
    bool *sol_open;
    int *sol_assign;
    std::mutex sol_mutex;

    double obj_best_bound;

    ThreadPool *thread_pool;

    void init_vars();
    void init_constrs();
    void init_cuts();
    void set_properties();

    friend void pcfl_impl3(const PCFLProbData *data, const PCFLConfig *config, PCFLSolution *sol);
};

#endif //PCFL_COMMON_H
