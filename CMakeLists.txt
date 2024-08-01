cmake_minimum_required(VERSION 3.10)

project(glibco)
set(CMAKE_BUILD_TYPE "Debug")
set (CMAKE_CXX_COMPILER "/usr/bin/g++")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
find_package(Boost COMPONENTS context system REQUIRED)

add_executable(mytest test.cpp memorypool.cpp gcoroutine.cpp)
add_executable(mytest_boost test_boost_ctx.cpp)
target_link_libraries(mytest_boost
${Boost_LIBRARIES}
)
# set(CMAKE_CXX_FLAGS "-O3 -g -DNDEBUG -ggdb -Wall -fPIC")

message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
