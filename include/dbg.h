//
// Created by jiho on 20. 4. 15..
//

#ifndef PCFL_DBG_H
#define PCFL_DBG_H

#include <iostream>
#include <chrono>
#include <string>

class time_measure {
public:
    explicit time_measure(std::ostream &target,
            std::string left, std::string right, bool on_destroy=true);
    ~time_measure();
    void print();
private:
    std::chrono::system_clock::time_point start;
    std::ostream &out;
    std::string left, right;
    bool on_destroy;
};

#endif //PCFL_DBG_H
