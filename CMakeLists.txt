cmake_minimum_required(VERSION 3.14)
include(CMakeDependentOption)
include(FetchContent)

project(cpp-delegates)

option(CPPDELEGATES_BUILD_TESTS
  "Build tests" ON)
cmake_dependent_option(CPPDELEGATES_BUILD_SAMPLES
  "Build examples" ON
  "" OFF)

# Serialization options
option(CPPDELEGATES_WITH_JSON_SERIALIZATION
  "Enable JSON serialization support (requires nlohmann/json)" OFF)

option(CPPDELEGATES_WITH_BINARY_SERIALIZATION
  "Enable binary serialization support (requires msgpack)" OFF)

add_library(cpp-delegates INTERFACE)
target_include_directories(cpp-delegates INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add compile definitions for serialization
if(CPPDELEGATES_WITH_JSON_SERIALIZATION)
  target_compile_definitions(cpp-delegates INTERFACE DELEGATES_WITH_JSON_SERIALIZATION)
  
  # Fetch nlohmann/json
  FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
  )
  FetchContent_MakeAvailable(json)
  target_link_libraries(cpp-delegates INTERFACE nlohmann_json::nlohmann_json)
endif()

if(CPPDELEGATES_WITH_BINARY_SERIALIZATION)
  target_compile_definitions(cpp-delegates INTERFACE DELEGATES_WITH_BINARY_SERIALIZATION)
  
  # Fetch msgpack-c (C version, no boost dependency)
  FetchContent_Declare(
    msgpack-c
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG c-6.1.0
  )
  
  # Configure msgpack-c - we need at least static library for generated headers
  set(MSGPACK_ENABLE_STATIC ON CACHE BOOL "" FORCE)
  set(MSGPACK_ENABLE_SHARED OFF CACHE BOOL "" FORCE)
  set(MSGPACK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(MSGPACK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  
  FetchContent_MakeAvailable(msgpack-c)
  
  # Use msgpack-c target which has correct include directories set up
  # It includes both source and build directories (for generated headers like sysdep.h)
  if(TARGET msgpack-c-static)
    target_link_libraries(cpp-delegates INTERFACE msgpack-c-static)
  elseif(TARGET msgpack-c)
    target_link_libraries(cpp-delegates INTERFACE msgpack-c)
  else()
    # Fallback: manually add include directories if target not found
    target_include_directories(cpp-delegates INTERFACE 
      ${msgpack-c_SOURCE_DIR}/include
      ${msgpack-c_BINARY_DIR}/include
      ${msgpack-c_BINARY_DIR}/include/msgpack
    )
  endif()
endif()

if(CPPDELEGATES_BUILD_SAMPLES)
add_subdirectory(example)
endif()

if(CPPDELEGATES_BUILD_TESTS)
add_subdirectory(tests)
endif()
