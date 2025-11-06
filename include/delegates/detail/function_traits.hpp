//
// Copyright (c) 2025, Alex Bobryshev <alexbobryshev555@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef DELEGATES_FUNCTION_TRAITS_HEADER
#define DELEGATES_FUNCTION_TRAITS_HEADER

#include "../delegates_conf.h"
#include <type_traits>
#include <functional>
#include <tuple>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {
namespace detail {

/// \brief    Extract function signature traits from callable types
template<typename T>
struct function_traits;

// Specialization for function types: R(Args...)
template<typename Result, typename... Args>
struct function_traits<Result(Args...)> {
    using result_type = Result;
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t args_count = sizeof...(Args);
    
    template<size_t I>
    using arg_type = std::tuple_element_t<I, args_tuple>;
};

// Specialization for function pointers: R(*)(Args...)
template<typename Result, typename... Args>
struct function_traits<Result(*)(Args...)> 
    : public function_traits<Result(Args...)> {};

// Specialization for std::function
template<typename Result, typename... Args>
struct function_traits<std::function<Result(Args...)>>
    : public function_traits<Result(Args...)> {};

// Specialization for member function pointers: R(Class::*)(Args...)
template<typename Class, typename Result, typename... Args>
struct function_traits<Result(Class::*)(Args...)> {
    using result_type = Result;
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t args_count = sizeof...(Args);
    using class_type = Class;
    
    template<size_t I>
    using arg_type = std::tuple_element_t<I, args_tuple>;
};

// Specialization for const member function pointers: R(Class::*)(Args...) const
template<typename Class, typename Result, typename... Args>
struct function_traits<Result(Class::*)(Args...) const> {
    using result_type = Result;
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t args_count = sizeof...(Args);
    using class_type = Class;
    
    template<size_t I>
    using arg_type = std::tuple_element_t<I, args_tuple>;
};

// Specialization for functors (lambdas, classes with operator())
// Extract signature from operator()
template<typename T>
struct function_traits 
    : public function_traits<decltype(&T::operator())> {};

// Helper aliases
template<typename T>
using function_result_t = typename function_traits<T>::result_type;

template<typename T>
using function_args_tuple_t = typename function_traits<T>::args_tuple;

// Helper function for C++14 compatibility (instead of variable template)
template<typename T>
constexpr size_t function_args_count() {
    return function_traits<T>::args_count;
}

} // namespace detail
} // namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif // DELEGATES_FUNCTION_TRAITS_HEADER

