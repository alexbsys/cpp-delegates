
#ifndef CALL_HELPER_HEADER
#define CALL_HELPER_HEADER

#include <tuple>
#include <functional>
#include <type_traits>
#include <utility>
#include <stdexcept>


namespace delegates {
namespace call_helper {

template <std::size_t... Ts>
struct index {};

template <std::size_t N, std::size_t... Ts>
struct gen_seq : gen_seq<N - 1, N - 1, Ts...> {};

template <std::size_t... Ts>
struct gen_seq<0, Ts...> : index<Ts...> {};

template<typename T>
T safe(const T& value) { return value; }

template< class T, class U >
static constexpr bool is_same_v = std::is_same<T, U>::value;

template< class T >
static constexpr bool is_copy_constructible_v = std::is_copy_constructible<T>::value;

template <typename TResult, typename... Args, std::size_t... Is>
static TResult DoFunctionCall(std::function<TResult(Args...)> f, typename std::tuple<Args...>& tup, index<Is...>) {
  return f(std::get<Is>(tup)...);
}

}//namespace call_helper
}//namespace delegates

#endif //CALL_HELPER_HEADER
