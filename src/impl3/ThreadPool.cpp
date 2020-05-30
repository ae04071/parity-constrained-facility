//
// Created by jiho on 20. 5. 30..
//
#include "common.h"

#define FLAGS_OPEN      0x0001UL
#define FLAGS_CLOSE     0x0002UL

ThreadPool::ThreadPool(size_t number_of_threads, bool open)
        : workers(number_of_threads), queue(), mutex(), flags(0) {
    if (open)
        this->open();
}

ThreadPool::~ThreadPool() {
    if ((this->flags & FLAGS_OPEN) && !(this->flags & FLAGS_CLOSE))
        this->close();
}

void ThreadPool::open() {
    this->flags |= FLAGS_OPEN;
    for (intptr_t i = 0; i < this->workers.size(); i++) {
        this->workers[i] = std::thread([this, i]() {this->worker_func(i);});
    }
}

void ThreadPool::close() {
    this->flags |= FLAGS_CLOSE;
    {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->condition.notify_all();
    }
    for (auto &th : this->workers) {
        th.join();
    }
}

void ThreadPool::worker_func(intptr_t tid) {
    std::unique_lock<std::mutex> lock(this->mutex);
    while (!(this->flags & FLAGS_CLOSE)) {
        if (this->queue.empty()) {
            this->condition.wait(lock);
        } else {
            auto task = this->queue.top();
            this->queue.pop();
            lock.unlock();
            task.func();
            lock.lock();
        }
    }
}

bool operator<(const ThreadPool::Task &a, const ThreadPool::Task &b) {
    return b.priority > a.priority;
}

void *ThreadPool::post(const std::function<void()> &func, double priority) {
    return this->post(std::function<void()>(func), priority);
}

void *ThreadPool::post(std::function<void()> &&func, double priority) {
    std::unique_lock<std::mutex> lock(this->mutex);
    this->queue.push({func, priority});
    this->condition.notify_one();
    return nullptr;
}

void ThreadPool::abort() {
    this->flags |= FLAGS_CLOSE;
    this->workers.clear(); // abort all the workers
    // the destructor of std::thread calls std::terminate()
    // https://en.cppreference.com/w/cpp/thread/thread/~thread
}

size_t ThreadPool::queue_size() const {
    return this->queue.size();
}
