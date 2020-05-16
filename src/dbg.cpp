//
// Created by jiho on 20. 4. 15..
//
#include <iostream>
#include <chrono>
#include <string>
#include <utility>
#include "dbg.h"

time_measure::time_measure()
        : time_measure("", "", std::cout, false) {}

time_measure::time_measure(std::string left, std::string right,
        std::ostream &target, bool on_destroy)
        : out(target), left(std::move(left)), right(std::move(right)),
        on_destroy(on_destroy), start(std::chrono::system_clock::now()) {}

time_measure::~time_measure() {
    if (on_destroy)
        print();
}

void time_measure::print() {
    out << left << std::chrono::duration<double>(this->get()).count() << right << std::endl;
}

double time_measure::get() const {
    return std::chrono::duration<double>(std::chrono::system_clock::now() - start).count();
}
