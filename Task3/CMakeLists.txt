cmake_minimum_required(VERSION 3.10)

project(CrossPlatformLogger)

set(CMAKE_CXX_STANDARD 11)

# Add main source file
set(SOURCES
    src/main.cpp
    src/logger.cpp
    src/counter.cpp
    src/process_manager.cpp
)

# Add include directories
include_directories(src)

# Define the executable
add_executable(prog ${SOURCES})

target_link_libraries(prog pthread)
