cmake_minimum_required(VERSION 3.10)

project(glibco)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 17)
# set (CMAKE_CXX_COMPILER "/usr/bin/g++")

set (BOOST_ROOT D:/dev/boost_1_74_0/build/lib/cmake)

find_package(Boost COMPONENTS context system REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src
${Boost_INCLUDE_DIRS})
add_definitions(-DTIMEOUTPUT)
add_executable(mytest_boost test_boost_ctx.cpp)
target_link_libraries(
    mytest_boost
    Boost::context
    Boost::system
)
# file(
#     GLOB_RECURSE
#     SOURCE_LIST
#     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
# )

# add_library(colib_static STATIC ${SOURCE_LIST})
# add_library(colib_shared SHARED ${SOURCE_LIST})

add_library(colib_static STATIC src/glibco.cpp)
add_library(colib_shared SHARED src/glibco.cpp)

target_link_libraries(colib_static
Boost::context
Boost::system)

target_link_libraries(colib_shared
Boost::context
Boost::system)

set_target_properties(colib_static PROPERTIES OUTPUT_NAME colib)
set_target_properties(colib_shared PROPERTIES OUTPUT_NAME colib)

add_executable(mytest 
test.cpp 
)

target_link_libraries(mytest
colib_static
)
# add_executable(test_hook
# ${CMAKE_CURRENT_SOURCE_DIR}/test_hook.cpp)
# add_library(colib_static STATIC ${CMAKE_CURRENT_SOURCE_DIR}/sharedhook.cpp)
# add_library(colib_shared SHARED ${CMAKE_CURRENT_SOURCE_DIR}/sharedhook.cpp)
# set_target_properties(colib_static PROPERTIES OUTPUT_NAME colib)
# set_target_properties(colib_shared PROPERTIES OUTPUT_NAME colib)

# set_target_properties(colib_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
# set_target_properties(colib_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
# target_link_libraries(test_hook ${CMAKE_CURRENT_SOURCE_DIR}/build/libcolib.so dl capstone)
# target_link_libraries(mytest_boost
# ${Boost_LIBRARIES}
# )
# add_compile_options(-fno-elide-constructors)  
# set(CMAKE_CXX_FLAGS "-O3 -g -DNDEBUG -ggdb -Wall -fPIC")
set(BUILD_TESTING true)
message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
if (BUILD_TESTING)
    set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_subdirectory(test)
endif()