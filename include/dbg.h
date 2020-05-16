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
    time_measure(std::string left, std::string right,
            std::ostream &target=std::cout, bool on_destroy=true);
    time_measure();
    ~time_measure();
    void print();
    double get() const;
private:
    std::chrono::system_clock::time_point start;
    std::ostream &out;
    std::string left, right;
    bool on_destroy;
};

#endif //PCFL_DBG_H
