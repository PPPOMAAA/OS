cmake_minimum_required(VERSION 3.10)

project(Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SQLite3 REQUIRED)

add_executable(prog 
	src/main.cpp
	src/data_aggregator.cpp
	src/server.cpp)

if(SQLite3_FOUND)
    target_include_directories(prog PRIVATE ${SQLite3_INCLUDE_DIRS})
    target_link_libraries(prog PRIVATE ${SQLite3_LIBRARIES})
else()
    message(FATAL_ERROR "SQLite3 not found!")
endif()

if(WIN32)
    target_link_libraries(prog PRIVATE ws2_32)
endif()