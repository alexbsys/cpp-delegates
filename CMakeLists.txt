cmake_minimum_required(VERSION 3.10)
include(CMakeDependentOption)

project(cpp-delegates)

cmake_dependent_option(CPPDELEGATES_BUILD_TESTS
  "Build tests when BUILD_TESTING is enabled and cpp-delegates is a root project" ON
  "BUILD_TESTING;CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF)
cmake_dependent_option(CPPDELEGATES_BUILD_SAMPLES
  "Build examples" ON
  "" OFF)

add_library(cpp-delegates INTERFACE)
target_include_directories(cpp-delegates INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CPPDELEGATES_BUILD_SAMPLES)
add_subdirectory(example)
endif()

if(CPPDELEGATES_BUILD_TESTS)
add_subdirectory(tests)
endif()
