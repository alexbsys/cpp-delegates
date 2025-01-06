
#ifndef DELEGATES_FACTORY_HEADER
#define DELEGATES_FACTORY_HEADER

#include "delegate_impl.hpp"

#include <memory>

namespace delegates {


namespace detail {

struct any_argument {
    template <typename T>
    operator T && () const;
};

template <typename Lambda, typename Is, typename = void>
struct can_accept_impl : std::false_type
{};

template <typename Lambda, std::size_t ...Is>
struct can_accept_impl <Lambda, std::index_sequence<Is...>, decltype(std::declval<Lambda>()(((void)Is, any_argument{})...), void())> : std::true_type
{};

template <typename Lambda, std::size_t N>
struct can_accept : can_accept_impl<Lambda, std::make_index_sequence<N>>
{};

template <typename Lambda, std::size_t N, size_t Max, typename = void>
struct lambda_details_maximum
{
    static constexpr size_t maximum_argument_count = N - 1;
    static constexpr bool is_variadic = false;
};

template <typename Lambda, std::size_t N, size_t Max>
struct lambda_details_maximum<Lambda, N, Max, std::enable_if_t<can_accept<Lambda, N>::value && (N <= Max)>> : lambda_details_maximum<Lambda, N + 1, Max>
{};

template <typename Lambda, std::size_t N, size_t Max>
struct lambda_details_maximum<Lambda, N, Max, std::enable_if_t<can_accept<Lambda, N>::value && (N > Max)>>
{
    static constexpr bool is_variadic = true;
};

template <typename Lambda, std::size_t N, size_t Max, typename = void>
struct lambda_details_minimum : lambda_details_minimum<Lambda, N + 1, Max>
{
    static_assert(N <= Max, "Argument limit reached");
};

template <typename Lambda, std::size_t N, size_t Max>
struct lambda_details_minimum<Lambda, N, Max, std::enable_if_t<can_accept<Lambda, N>::value>> : lambda_details_maximum<Lambda, N, Max>
{
    static constexpr size_t minimum_argument_count = N;
};

template <typename Lambda, size_t Max = 50>
struct lambda_details : lambda_details_minimum<Lambda, 0, Max>
{};

}//namespace detail


namespace factory {





// raw

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_method_delegate(TClass* callee,
                                        TResult (TClass::*method)(Ts...),
                                        Ts... args) {
  return new MethodDelegate<TClass, TResult, Ts...>(callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_method_delegate(TClass* callee,
                                       TResult (TClass::*method)(Ts...),
                                       DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return new MethodDelegate<TClass, TResult, Ts...>(callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_const_method_delegate(const TClass* callee,
                                        TResult (TClass::*method)(Ts...) const,
                                        Ts... args) {
  return new ConstMethodDelegate<TClass, TResult, Ts...>(callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_const_method_delegate(const TClass* callee,
                                             TResult (TClass::*method)(Ts...) const,
                                             DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return new ConstMethodDelegate<TClass, TResult, Ts...>(callee, method, std::move(params));
}

template <typename TResult, typename... Ts>
static IDelegate* make_function_delegate(TResult f(Ts...), Ts... args) {
  return new FunctionalDelegate<TResult, Ts...>(std::function<TResult(Ts...)>(f), std::forward<Ts>(args)...);
}

/*template <typename TResult, typename... Ts>
static IDelegate* make_function_delegate(TResult f(Ts...), std::nullptr_t) {
  return new FunctionalDelegate<TResult, Ts...>(std::function<TResult(Ts...)>(f), std::nullptr_t{});
}*/

template <typename TResult, typename... Ts>
static IDelegate* make_function_delegate(TResult f(Ts...), DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return new FunctionalDelegate<TResult, Ts...>(std::function<TResult(Ts...)>(f), std::move(params));
}

//template <typename TResult>
//static IDelegate* make_function_delegate(TResult f()) {
//  return new FunctionalDelegate<TResult>(std::function<TResult()>(f), std::nullptr_t{});
//}

template <typename TResult, typename... Ts>
static IDelegate* make_function_delegate(std::function<TResult(Ts...)> func, Ts... args) {
  return new FunctionalDelegate<TResult, Ts...>(func, std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static IDelegate* make_function_delegate(std::function<TResult(Ts...)> func, DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return new FunctionalDelegate<TResult, Ts...>(func, std::move(params));
}

//template <typename TResult>
//static IDelegate* make_function_delegate(std::function<TResult()> func) {
//  return new FunctionalDelegate<TResult>(func, std::nullptr_t{});
//}

template <typename TResult=void, typename... Ts, typename F>
static IDelegate* make_lambda_delegate(F && lambda, DelegateArgs<Ts...> && params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return new LambdaDelegate<TResult, F, Ts...>(std::move(lambda), std::move(params) /* std::forward<Ts>(args)...*/);
}

//template <typename TResult = void, typename... Ts, typename F>
//static IDelegate* make_lambda_delegate(F && lambda, Ts... args) {
//  return new LambdaDelegate<TResult, F, Ts...>(std::move(lambda), std::move(DelegateArgs<Ts...>(args...)));
//}

//template <typename TResult = void, typename F>
//static IDelegate* make_lambda_delegate(F&& lambda) {
//  return new LambdaDelegate<TResult, F>(std::move(lambda), std::nullptr_t{});
//}

// class instance pointer is weak_ptr

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_method_delegate(std::weak_ptr<TClass> callee,
                                       TResult (TClass::*method)(Ts...),
                                       Ts... args) {
  return new WeakMethodDelegate<TClass, TResult, Ts...>(
    callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_method_delegate(std::weak_ptr<TClass> callee,
                                       TResult (TClass::*method)(Ts...),
                                       DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return new WeakMethodDelegate<TClass, TResult, Ts...>(
    callee, method, std::move(params));
}

// class instance pointer is shared ptr

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_method_delegate(std::shared_ptr<TClass> callee,
                                       TResult (TClass::*method)(Ts...),
                                       Ts... args) {
  return new SharedMethodDelegate<TClass, TResult, Ts...>(
    callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_method_delegate(std::shared_ptr<TClass> callee,
                                       TResult (TClass::*method)(Ts...),
                                       DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return new SharedMethodDelegate<TClass, TResult, Ts...>(
    callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_const_method_delegate(std::shared_ptr<TClass> callee,
                                             TResult(TClass::* method)(Ts...) const,
                                             Ts... args) {
  return new SharedConstMethodDelegate<TClass, TResult, Ts...>(
    callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static IDelegate* make_const_method_delegate(std::shared_ptr<TClass> callee,
                                             TResult(TClass::* method)(Ts...) const,
                                             DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return new SharedConstMethodDelegate<TClass, TResult, Ts...>(
    callee, method, std::move(params));
}

// shared

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  std::shared_ptr<TClass> callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return std::make_shared<SharedMethodDelegate<TClass, TResult, Ts...> >(
    callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  std::shared_ptr<TClass> callee, TResult (TClass::*method)(Ts...), DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_shared<SharedMethodDelegate<TClass, TResult, Ts...> >(
    callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  TClass* callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return std::make_shared<MethodDelegate<TClass, TResult, Ts...> >(callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  TClass* callee, TResult (TClass::*method)(Ts...), DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_shared<MethodDelegate<TClass, TResult, Ts...> >(callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(
  const TClass* callee, TResult(TClass::* method)(Ts...) const,  Ts... args) {
  return std::make_shared<ConstMethodDelegate<TClass, TResult, Ts...> >(callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(const TClass* callee, TResult(TClass::* method)(Ts...) const,
  DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_shared<ConstMethodDelegate<TClass, TResult, Ts...> >(callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(
  std::shared_ptr<TClass> callee, TResult(TClass::* method)(Ts...) const, Ts... args) {
  return std::make_shared<SharedConstMethodDelegate<TClass, TResult, Ts...> >(callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(std::shared_ptr<TClass> callee, TResult(TClass::* method)(Ts...) const,
  DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_shared<SharedConstMethodDelegate<TClass, TResult, Ts...> >(callee, method, std::move(params));
}


template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  TResult f(Ts...), Ts... args) {
  return std::make_shared<FunctionalDelegate<TResult, Ts...> >(
    std::function<TResult(Ts...)>(f), std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  TResult f(Ts...), DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_shared<FunctionalDelegate<TResult, Ts...> >(
    std::function<TResult(Ts...)>(f), std::move(params));
}

template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  std::function<TResult(Ts...)> func, Ts... args) {
  return std::make_shared<FunctionalDelegate<TResult, Ts...> >(func, std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  std::function<TResult(Ts...)> func, DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_shared<FunctionalDelegate<TResult, Ts...> >(func, std::move(params));
}

template <typename TResult=void, typename... Ts, typename F>
static std::shared_ptr<IDelegate> make_shared_lambda_delegate(F && lambda, Ts... args) {
  return std::make_shared<LambdaDelegate<TResult, F, Ts...> >(std::move(lambda), std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static std::shared_ptr<IDelegate> make_shared_lambda_delegate(F && lambda, DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_shared<LambdaDelegate<TResult, F, Ts...> >(std::move(lambda), std::move(params));
}

// unique

template <class TClass, typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  std::shared_ptr<TClass> callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return std::make_unique<SharedMethodDelegate<TClass, TResult, Ts...> >(
    callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  std::shared_ptr<TClass> callee, TResult (TClass::*method)(Ts...), DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_unique<SharedMethodDelegate<TClass, TResult, Ts...> >(
    callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  TClass* callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return std::make_unique<MethodDelegate<TClass, TResult, Ts...> >(callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  TClass* callee, TResult (TClass::*method)(Ts...), DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_unique<MethodDelegate<TClass, TResult, Ts...> >(callee, method, std::move(params));
}

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  TResult f(Ts...), Ts... args) {
  return std::make_unique<FunctionalDelegate<TResult, Ts...> >(
    std::function<TResult(Ts...)>(f), std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  TResult f(Ts...), DelegateArgs<Ts...> && params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_unique<FunctionalDelegate<TResult, Ts...> >(
    std::function<TResult(Ts...)>(f), std::move(params));
}

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  std::function<TResult(Ts...)> func, Ts... args) {
  return std::make_unique<FunctionalDelegate<TResult, Ts...> >(func, std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  std::function<TResult(Ts...)> func, DelegateArgs<Ts...> && params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_unique<FunctionalDelegate<TResult, Ts...> >(func, std::move(params));
}

template <typename TResult=void, typename... Ts, typename F>
static std::unique_ptr<IDelegate> make_unique_lambda_delegate(F && lambda, Ts... args) {
  return std::make_unique<LambdaDelegate<TResult, F, Ts...> >(std::move(lambda), std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static std::unique_ptr<IDelegate> make_unique_lambda_delegate(F && lambda, DelegateArgs<Ts...> && params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_unique<LambdaDelegate<TResult, F, Ts...> >(std::move(lambda), std::move(params));
}

// autodetect raw

template <typename TResult, typename... Ts>
static IDelegate* make(TResult f(Ts...), Ts... args) {
  return make_function_delegate(f,std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static IDelegate* make(std::function<TResult(Ts...)> func, Ts... args) {
  return make_function_delegate(func,std::forward<Ts>(args)...);
}

template <typename TClass, typename TResult, typename... Ts>
static IDelegate* make(TClass* callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return make_method_delegate(callee, method, std::forward<Ts>(args)...);
}

template <typename TClass, typename TResult, typename... Ts>
static IDelegate* make(std::shared_ptr<TClass> callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return make_method_delegate<TClass, TResult, Ts...>(callee, method, std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static IDelegate* make(F && lambda, Ts&&... args) {
  return make_lambda_delegate<TResult, Ts...>(std::move(lambda), 
    DelegateArgs<Ts...>(std::forward<Ts>(args)...));
}

template <typename TResult=void, typename... Ts, typename F>
static IDelegate* make(F && lambda, DelegateArgs<Ts...> && params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return make_lambda_delegate<TResult, Ts...>(std::move(lambda), std::move(params));
}

template <typename TResult=void, typename... Ts>
static ISignal* make_signal(Ts&&... args) {
  return new detail::SignalBase<TResult, Ts...>(std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts>
static ISignal* make_signal(DelegateArgs<Ts...> && params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return new detail::SignalBase<TResult, Ts...>(std::move(params));
}

// autodetect shared

template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared(TResult f(Ts...), Ts... args) {
  return make_shared_function_delegate(f,std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared(std::function<TResult(Ts...)> func, Ts... args) {
  return make_shared_function_delegate(func,std::forward<Ts>(args)...);
}

template <typename TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared(TClass* callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return make_shared_method_delegate(callee, method, std::forward<Ts>(args)...);
}

template <typename TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared(std::shared_ptr<TClass> callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return make_shared_method_delegate<TClass, TResult, Ts...>(callee, method, std::forward<Ts>(args)...);
}

template <typename TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared(std::shared_ptr<TClass> callee, TResult(TClass::* method)(Ts...), DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return make_shared_method_delegate<TClass, TResult, Ts...>(callee, method, std::move(params));
}

/*
template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(
  const TClass* callee, TResult(TClass::* method)(Ts...) const, Ts... args) {
  return std::make_shared<ConstMethodDelegate<TClass, TResult, Ts...> >(callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(const TClass* callee, TResult(TClass::* method)(Ts...) const,
  DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_shared<ConstMethodDelegate<TClass, TResult, Ts...> >(callee, method, std::move(params));
}
*/


template <typename TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared(const TClass* callee, TResult(TClass::* method)(Ts...) const, Ts... args) {
  return make_shared_const_method_delegate<TClass, TResult, Ts...>(callee, method, std::forward<Ts>(args)...);
}

template <typename TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared(const TClass* callee, TResult(TClass::* method)(Ts...) const, DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return make_shared_const_method_delegate<TClass, TResult, Ts...>(callee, method, std::move(params));
}

template <typename TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared(std::shared_ptr<TClass> callee, TResult(TClass::* method)(Ts...) const, Ts... args) {
  return make_shared_const_method_delegate<TClass, TResult, Ts...>(callee, method, std::forward<Ts>(args)...);
}

template <typename TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared(std::shared_ptr<TClass> callee, TResult(TClass::* method)(Ts...) const, DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return make_shared_const_method_delegate<TClass, TResult, Ts...>(callee, method, std::move(params));
}



template <typename TResult=void, typename... Ts, typename F>
static std::shared_ptr<IDelegate> make_shared(F && lambda, Ts&&... args) {
  return make_shared_lambda_delegate<TResult, Ts...>(std::move(lambda), std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static std::shared_ptr<IDelegate> make_shared(F&& lambda, DelegateArgs<Ts...>&& params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return make_shared_lambda_delegate<TResult, Ts...>(std::move(lambda), std::move(params));
}

template <typename TResult=void, typename... Ts>
static std::shared_ptr<ISignal> make_shared_signal(Ts&&... args) {
  return std::shared_ptr<ISignal>(make_signal<TResult,Ts...>(std::forward<Ts>(args)...));
}

template <typename TResult=void, typename... Ts>
static std::shared_ptr<ISignal> make_shared_signal(DelegateArgs<Ts...> && params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::shared_ptr<ISignal>(make_signal<TResult,Ts...>(std::move(params)));
}


// autodetect unique

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique(TResult f(Ts...), Ts... args) {
  return make_unique_function_delegate(f,std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique(std::function<TResult(Ts...)> func, Ts... args) {
  return make_unique_function_delegate(func,std::forward<Ts>(args)...);
}

template <typename TClass, typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique(TClass* callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return make_unique_method_delegate(callee, method, std::forward<Ts>(args)...);
}

template <typename TClass, typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique(std::shared_ptr<TClass> callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return make_unique_method_delegate<TClass, TResult, Ts...>(callee, method, std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static std::unique_ptr<IDelegate> make_unique(F && lambda, Ts&&... args) {
  return make_unique_lambda_delegate<TResult, Ts...>(std::move(lambda), std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static std::unique_ptr<IDelegate> make_unique(F && lambda, DelegateArgs<Ts...> && params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return make_unique_lambda_delegate<TResult, Ts...>(std::move(lambda), std::nullptr_t{});
}

template <typename TResult=void, typename... Ts>
static std::unique_ptr<ISignal> make_unique_signal(Ts&&... args) {
  return std::make_unique<detail::SignalBase<TResult,Ts...> >(std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts>
static std::unique_ptr<ISignal> make_unique_signal(DelegateArgs<Ts...> && params = DelegateArgs<Ts...>(std::nullptr_t{})) {
  return std::make_unique<detail::SignalBase<TResult,Ts...> >(std::move(params));
}

}// namespace factory
}//namespace delegates

#endif //DELEGATES_FACTORY_HEADER
