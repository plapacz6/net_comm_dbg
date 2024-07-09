cmake_minimum_required(VERSION 3.19)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED)

add_library(helper STATIC helper.c)
target_compile_options(helper PUBLIC -g3 -Wall -pthread)
target_link_options(helper PUBLIC ${FSTANITIZE_FLAG} -g3 -Wall -pthread)
target_link_libraries(helper PUBLIC pthread)

add_library(TSvSt STATIC TSvSt.cpp)
target_compile_options(TSvSt PUBLIC -g3 -Wall -pthread)
target_link_options(TSvSt PUBLIC ${FSTANITIZE_FLAG} -g3 -Wall -pthread)
target_link_libraries(TSvSt PUBLIC pthread)

add_library(Ttest_server_for_one_connection STATIC Ttest_server_for_one_connection.cpp)
target_compile_options(Ttest_server_for_one_connection PUBLIC -g3 -Wall -pthread)
target_link_options(Ttest_server_for_one_connection PUBLIC ${FSTANITIZE_FLAG} -g3 -Wall -pthread)
target_link_libraries(Ttest_server_for_one_connection PUBLIC pthread)


list(APPEND lib_targets
    server_c
    client_c        
)

foreach(lib_target IN LISTS lib_targets)
    add_library("${lib_target}" SHARED "${lib_target}.c")
    target_compile_options("${lib_target}" PUBLIC -g3 -Wall -pthread)
    target_link_options("${lib_target}" PUBLIC ${FSTANITIZE_FLAG} -g3 -Wall -pthread)
    target_include_directories("${lib_target}" PUBLIC .)
    target_link_libraries("${lib_target}" 
        pthread
    )
endforeach()

add_subdirectory(tests)
