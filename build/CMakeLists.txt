project(proj)

set(ROOT ..)
set(LIB_DIR ${ROOT}/lib)
set(MAIN_DIR ${ROOT}/src)
set(COMMON_DIR ${ROOT}/include)
set(OUTPUT PCFL)


# Add source list
set(SOURCE 
${MAIN_DIR}/pcfl_hw.cpp
${MAIN_DIR}/pcfl_model.cpp
${MAIN_DIR}/pcfl_callback.cpp
${MAIN_DIR}/ProbData.cpp
)

# Add include list
set(INCLUDE
${COMMON_DIR}/gurobi_c++.h
${COMMON_DIR}/pcfl_model.h
${COMMON_DIR}/pcfl_callback.h
${COMMON_DIR}/ProbData.h
)

include_directories(${COMMON_DIR})

cmake_minimum_required(VERSION 3.0)
message(${CMAKE_PROJECT_NAME})
add_compile_options(-m64 -g -std=c++11)

include_directories(${COMMON_DIR})
link_directories(${LIB_DIR})
add_executable(${OUTPUT} ${SOURCE})
target_link_libraries(${OUTPUT} libgurobi_c++.a)
target_link_libraries(${OUTPUT} libgurobi90.so)
target_link_libraries(${OUTPUT} libboost_system.so)
target_link_libraries(${OUTPUT} libboost_program_options.so)
