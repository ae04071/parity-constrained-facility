cmake_minimum_required(VERSION 3.13)
project(parity_constrained_facility)

set(CMAKE_CXX_STANDARD 14)
set(GUROBI_HOME $ENV{GUROBI_HOME})

link_directories(${GUROBI_HOME}/lib)
link_libraries(gurobi_g++5.2 gurobi90)
include_directories(${GUROBI_HOME}/include)

add_executable(mip1_c++
        src/mip1_c++.cpp)
add_executable(facility_c++
        ${GUROBI_HOME}/examples/c++/facility_c++.cpp)
add_executable(pcfl-jiho-1
        src/pcfl-jiho-1.cpp)
