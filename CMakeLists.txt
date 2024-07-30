cmake_minimum_required(VERSION 3.10)

project(glibco)
set(CMAKE_BUILD_TYPE "Realease")
set (CMAKE_CXX_COMPILER "/usr/bin/g++")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mytest test.cpp memorypool.cpp gcoroutine.cpp)
set(CMAKE_CXX_FLAGS "-O3 -g -DNDEBUG -ggdb -Wall -Werror -fPIC")

message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
