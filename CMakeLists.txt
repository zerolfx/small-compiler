cmake_minimum_required(VERSION 3.16)
project(small)

set(CMAKE_CXX_STANDARD 20)

add_executable(small main.cpp env.cpp ast.cpp)

find_package(fmt)
target_link_libraries(small fmt::fmt)

include_directories(.)

add_subdirectory(test)