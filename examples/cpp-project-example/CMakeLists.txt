cmake_minimum_required(VERSION 3.27)
project(clay_examples_cpp_project_example CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")

add_executable(clay_examples_cpp_project_example main.cpp)

target_include_directories(clay_examples_cpp_project_example PUBLIC .)

set(CMAKE_C_FLAGS_DEBUG "-Werror -Wall -Wno-error=missing-braces")
set(CMAKE_C_FLAGS_RELEASE "-O3")
