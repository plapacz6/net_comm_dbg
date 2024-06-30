cmake_minimum_required(VERSION 3.19)
project(maze VERSION 0.0.1)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED)



# set(FSTANITIZE_FLAG -fsanitize=address,undefined -v)
set(FSTANITIZE_FLAG -fsanitize=thread,undefined -v)
# set(FSTANITIZE_FLAG -fsanitize=thread -v)
# set(FSTANITIZE_FLAG -fsanitize=memory -fsanitize-memory-track-origins -v)

if(FSTANITIZE_FLAG)
message("----------------------------------------------------------------------")
message("to run path/executable compiled with -fsanitize by clang on ubuntu 22:")
message("setarch `uname -m` -R path/executable")
message("----------------------------------------------------------------------")
endif()

enable_testing()

add_subdirectory(src)

