
cmake_minimum_required(VERSION 3.0)

project(cpp-delegates-example VERSION 0.0.1.1 LANGUAGES C CXX)

set (SOURCE_FILES
  example_main.cc
  example_1.cc
)

add_executable(cpp-delegates-example ${SOURCE_FILES})
install(TARGETS cpp-delegates-example DESTINATION ../out)

