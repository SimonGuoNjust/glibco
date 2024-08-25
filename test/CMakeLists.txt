
file(
    GLOB_RECURSE
    TEST_CPP_LIST
    ${TEST_DIR}/*.cpp
)
foreach(f ${TEST_CPP_LIST})
    # file relative path from src/
    string(REGEX MATCH "test/.*" relative_path ${f})
    # delete string "src/"
    string(REGEX REPLACE "test/" "" target_name ${relative_path})
    # rename '/' and '.' to '_'
    string(REGEX REPLACE "/|\\." "_" target_name ${target_name})
    add_executable(${target_name} ${f})
    target_link_libraries(${target_name} colib_static Boost::context
    Boost::system)
endforeach()

