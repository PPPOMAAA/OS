cmake_minimum_required(VERSION 3.10)

project(EmulatedDevice)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

add_executable(prog
    src/main.cpp
    src/data_aggregator.cpp)

target_link_libraries(prog PRIVATE pthread)

add_executable(emulated_device src/emulated_device.cpp)
target_link_libraries(emulated_device PRIVATE pthread)
