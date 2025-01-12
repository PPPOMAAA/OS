cmake_minimum_required(VERSION 3.10)

project(ProcessLib)

set(CMAKE_CXX_STANDARD 17)

add_library(process_lib src/process_lib.cpp src/process_lib.h)
add_executable(prog src/main.cpp)

target_link_libraries(prog process_lib)