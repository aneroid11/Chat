cmake_minimum_required(VERSION 3.24)
project(socket)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS -pthread)

add_executable(socket main.cpp message.h timehelper.h timehelper.cpp)
