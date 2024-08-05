cmake_minimum_required(VERSION 3.10)

project(glibco)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_COMPILER "/usr/bin/g++")
# set(Boost_USE_STATIC_LIBS ON)
# set(Boost_USE_STATIC_RUNTIME ON)
set (BOOST_ROOT /home/ubuntu/install/lib/cmake)

find_package(Boost COMPONENTS context system REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
${Boost_INCLUDE_DIRS})
add_definitions(-DUSE_BOOST_CONTEXT)
add_executable(mytest test.cpp memorypool.cpp gcoroutine.cpp)
add_executable(mytest_boost test_boost_ctx.cpp)
target_link_libraries(mytest
/home/ubuntu/install/lib/libboost_context-mt-x64.so
/home/ubuntu/install/lib/libboost_system-mt-x64.so
)

target_link_libraries(mytest_boost
${Boost_LIBRARIES}
)
# add_compile_options(-fno-elide-constructors)  
set(CMAKE_CXX_FLAGS "-O3 -g -DNDEBUG -ggdb -Wall -fPIC")

message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
