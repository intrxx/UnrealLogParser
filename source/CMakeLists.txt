cmake_minimum_required(VERSION 4.0)
project(LogParser)

set(CMAKE_CXX_STANDARD 23)

add_executable(LogParser main.cpp)
target_include_directories(LogParser PRIVATE ../include/cxxopts)
