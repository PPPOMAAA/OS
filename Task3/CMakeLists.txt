cmake_minimum_required(VERSION 3.10)
project(CrossPlatformLogger)

set(CMAKE_CXX_STANDARD 17)

add_executable(prog
    src/main.cpp
    src/logger.cpp
    src/counter.cpp
    src/process_manager.cpp
)