cmake_minimum_required(VERSION 3.10)

project(glibco)
set(CMAKE_BUILD_TYPE "Debug")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mytest test.cpp memorypool.cpp gcoroutine.cpp)


