cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
add_library(ast STATIC ast.cpp)
target_include_directories(ast PUBLIC .)
