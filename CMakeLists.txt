
cmake_minimum_required(VERSION 3.10)
project(cpp-delegates)

add_library(cpp-delegates INTERFACE)
target_include_directories(cpp-delegates INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(example)
add_subdirectory(tests)
