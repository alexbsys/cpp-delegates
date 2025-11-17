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

#ifndef DELEGATES_CALLABLE_TRAITS_HEADER
#define DELEGATES_CALLABLE_TRAITS_HEADER

#include "../delegates_conf.h"
#include "function_traits.hpp"
#include <type_traits>
#include <functional>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {
namespace detail {

// Tag types for callable category
struct lambda_tag {};
struct function_tag {};
struct method_tag {};
struct const_method_tag {};
struct unknown_tag {};

/// \brief    Determine category of callable type for tag dispatch
template<typename T>
struct callable_category {
private:
    // Test for lambda/functor (has operator())
    template<typename U>
    static auto test(int) -> decltype(&U::operator(), lambda_tag{});
    
    // Test for function pointer
    template<typename U>
    static function_tag test(long);
    
public:
    using type = decltype(test<T>(0));
};

// Specialization for function pointers
template<typename Result, typename... Args>
struct callable_category<Result(*)(Args...)> {
    using type = function_tag;
};

// Specialization for std::function
template<typename Result, typename... Args>
struct callable_category<std::function<Result(Args...)>> {
    using type = function_tag;
};

// Specialization for member function pointers
template<typename Class, typename Result, typename... Args>
struct callable_category<Result(Class::*)(Args...)> {
    using type = method_tag;
};

// Specialization for const member function pointers
template<typename Class, typename Result, typename... Args>
struct callable_category<Result(Class::*)(Args...) const> {
    using type = const_method_tag;
};

// Helper alias
template<typename T>
using callable_category_t = typename callable_category<T>::type;

} // namespace detail
} // namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif // DELEGATES_CALLABLE_TRAITS_HEADER

