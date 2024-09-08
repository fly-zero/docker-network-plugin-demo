cmake_minimum_required(VERSION 3.10)

project(flyzero VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(test-net)

add_executable(test-net server.cpp main.cpp)