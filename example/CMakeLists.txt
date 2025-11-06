
cmake_minimum_required(VERSION 3.10)

project(cpp-delegates-example VERSION 0.0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14 CACHE STRING "v")
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #Optional
set (SOURCE_FILES
  example_main.cc
)

set (DELEGATES_LIB_HEADER_FILES
  ../include/delegates/delegates.hpp
  ../include/delegates/delegates_conf.h  
  ../include/delegates/i_delegate.h
  ../include/delegates/detail/signal.hpp
  ../include/delegates/detail/delegate_args_impl.hpp
  ../include/delegates/detail/delegate_result_impl.hpp
  ../include/delegates/detail/delegate_impl.hpp
  ../include/delegates/detail/factory.hpp
  ../include/delegates/detail/tuple_runtime.hpp
  ../include/delegates/detail/function_traits.hpp
  ../include/delegates/detail/callable_traits.hpp
  ../include/delegates/typed_delegate.hpp
  ../include/delegates/serialization/iserializer.h
  ../include/delegates/serialization/iserializer_impl.hpp
)

include_directories("${CMAKE_CURRENT_LIST_DIR}/../include")

add_executable(cpp-delegates-example ${SOURCE_FILES} ${DELEGATES_LIB_HEADER_FILES})
target_link_libraries(cpp-delegates-example cpp-delegates)

if(MSVC)
    target_compile_options(cpp-delegates-example PRIVATE /bigobj)
endif()
install(TARGETS cpp-delegates-example DESTINATION ../out)
