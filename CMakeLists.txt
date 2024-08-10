cmake_minimum_required(VERSION 3.28)
project(Compilation_HW5)

set(CMAKE_CXX_STANDARD 17)

add_executable(Compilation_HW5
        cg.cpp
        cg.hpp
        Types.h
        Types.cpp
        ScopesTable.h
        ScopesTable.cpp
        hw3_output.cpp)
