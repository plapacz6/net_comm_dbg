cmake_minimum_required(VERSION 3.19)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED)

find_package(GTest REQUIRED)

list(APPEND lib_target_tests
    test_server_c
    test_client_c
)

foreach(lib_target_test lib_target IN ZIP_LISTS lib_target_tests lib_targets)
    add_executable("${lib_target_test}" "${lib_target_test}.cpp")
    target_compile_definitions("${lib_target_test}" PUBLIC LDFLAGS="-stdlib=libstdc++")
    target_compile_options("${lib_target_test}" PUBLIC -g3 -Wall -pthread)
    target_link_libraries("${lib_target_test}" 
        pthread
        GTest::GTest
        GTest::gtest_main
        "${lib_target}"
        Ttest_server_for_one_connection
    )
    add_test("${lib_target_test}_runner" "${lib_target_test}")
endforeach()


