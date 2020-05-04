cmake_minimum_required(VERSION 3.0)
project(pcfl)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE -O3)
set(CMAKE_CXX_FLAGS_DEBUG -g)

# Gurobi path
set(GUROBI_HOME $ENV{GUROBI_HOME})
if ("${GUROBI_HOME}" EQUAL "" OR NOT IS_DIRECTORY "${GUROBI_HOME}")
    message(FATAL_ERROR "Set GUROBI_HOME environment variable to the root directory of GUROBI")
endif ()
link_directories(${GUROBI_HOME}/lib)
include_directories(${GUROBI_HOME}/include)

set(OUTPUT PCFL)

# Add source list
set(SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pcfl_jiho.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pcfl_model.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pcfl_callback.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dbg.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pcfl_assign.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pcfl_impl2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pcfl_impl3.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LPRelaxation.cpp
        )

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src/mcpm)

# Executable and link libraries
add_executable(${OUTPUT} ${SOURCE})
target_link_libraries(${OUTPUT} gurobi_g++5.2 gurobi90)
target_link_libraries(${OUTPUT} boost_system boost_program_options)
target_link_libraries(${OUTPUT} mcpm)
target_link_libraries(${OUTPUT} glpk)
