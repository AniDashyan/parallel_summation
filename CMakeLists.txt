cmake_minimum_required(VERSION 3.10)

project(parallel_summation LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sum main.cpp)