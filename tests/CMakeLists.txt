
cmake_minimum_required(VERSION 3.0)

project(cpp-delegates-tests VERSION 0.0.1.1 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Now simply link against gtest or gtest_main as needed. Eg
#add_test(NAME example_test COMMAND example)

#enable_testing()
#find_package(GTest REQUIRED)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "v")
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #Optional

set (SOURCE_FILES
  delegates_tests.cc
  delegates_tests_main.cc
  utils/cprintf.c
  utils/mem_checker.cc
)

set (HEADER_FILES
  delegates_tests.h
  utils/cprintf.h
  utils/mem_checker.h
)  

set (DELEGATES_LIB_HEADER_FILES
  ../include/delegates/delegates.hpp
  ../include/delegates/i_delegate.h
  ../include/delegates/detail/call_helper.hpp
  ../include/delegates/detail/signal.hpp
  ../include/delegates/detail/delegate_args_impl.hpp
  ../include/delegates/detail/delegate_result_impl.hpp
  ../include/delegates/detail/delegate_impl.hpp
  ../include/delegates/detail/factory.hpp
  ../include/delegates/detail/tuple_runtime.hpp
)
  
include_directories("${CMAKE_CURRENT_LIST_DIR}/../include")

add_executable(cpp-delegates-tests ${SOURCE_FILES} ${HEADER_FILES} ${DELEGATES_LIB_HEADER_FILES})
target_link_libraries(cpp-delegates-tests gtest_main)
#add_library(udev_dynamic_wrapper STATIC ${SOURCE_FILES})



#add_executable(foo foo.cc)
#target_link_libraries(cpp-delegates-tests GTest::gtest GTest::gtest_main)

#add_test(AllTestsInFoo foo)

install(TARGETS cpp-delegates-tests DESTINATION ../out)

