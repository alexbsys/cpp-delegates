
#ifndef DELEGATES_FACTORY_HEADER
#define DELEGATES_FACTORY_HEADER

#include "delegate_impl.hpp"

#include <memory>

namespace delegates {
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
                                       std::nullptr_t) {
  return new MethodDelegate<TClass, TResult, Ts...>(callee, method, std::nullptr_t{});
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
                                             std::nullptr_t) {
  return new ConstMethodDelegate<TClass, TResult, Ts...>(callee, method, std::nullptr_t{});
}

template <typename TResult, typename... Ts>
static IDelegate* make_function_delegate(TResult f(Ts...), Ts... args) {
  return new FunctionalDelegate<TResult, Ts...>(std::function<TResult(Ts...)>(f), std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static IDelegate* make_function_delegate(TResult f(Ts...), std::nullptr_t) {
  return new FunctionalDelegate<TResult, Ts...>(std::function<TResult(Ts...)>(f), std::nullptr_t{});
}

template <typename TResult, typename... Ts>
static IDelegate* make_function_delegate(std::function<TResult(Ts...)> func, Ts... args) {
  return new FunctionalDelegate<TResult, Ts...>(func, std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static IDelegate* make_function_delegate(std::function<TResult(Ts...)> func, std::nullptr_t) {
  return new FunctionalDelegate<TResult, Ts...>(func, std::nullptr_t{});
}

template <typename TResult=void, typename... Ts, typename F>
static IDelegate* make_lambda_delegate(F && lambda, Ts... args) {
  return new LambdaDelegate<TResult, F, Ts...>(std::move(lambda), std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static IDelegate* make_lambda_delegate(F && lambda, std::nullptr_t) {
  return new LambdaDelegate<TResult, F, Ts...>(std::move(lambda), std::nullptr_t{});
}

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
                                       std::nullptr_t) {
  return new WeakMethodDelegate<TClass, TResult, Ts...>(
    callee, method, std::nullptr_t{});
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
                                       std::nullptr_t) {
  return new SharedMethodDelegate<TClass, TResult, Ts...>(
    callee, method, std::nullptr_t{});
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
                                             std::nullptr_t) {
  return new SharedConstMethodDelegate<TClass, TResult, Ts...>(
    callee, method, std::nullptr_t{});
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
  std::shared_ptr<TClass> callee, TResult (TClass::*method)(Ts...), std::nullptr_t) {
  return std::make_shared<SharedMethodDelegate<TClass, TResult, Ts...> >(
    callee, method, std::nullptr_t{});
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  TClass* callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return std::make_shared<MethodDelegate<TClass, TResult, Ts...> >(callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  TClass* callee, TResult (TClass::*method)(Ts...), std::nullptr_t) {
  return std::make_shared<MethodDelegate<TClass, TResult, Ts...> >(callee, method, std::nullptr_t{});
}

template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  TResult f(Ts...), Ts... args) {
  return std::make_shared<FunctionalDelegate<TResult, Ts...> >(
    std::function<TResult(Ts...)>(f), std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  TResult f(Ts...), std::nullptr_t) {
  return std::make_shared<FunctionalDelegate<TResult, Ts...> >(
    std::function<TResult(Ts...)>(f), std::nullptr_t{});
}

template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  std::function<TResult(Ts...)> func, Ts... args) {
  return std::make_shared<FunctionalDelegate<TResult, Ts...> >(func, std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  std::function<TResult(Ts...)> func, std::nullptr_t) {
  return std::make_shared<FunctionalDelegate<TResult, Ts...> >(func, std::nullptr_t{});
}

template <typename TResult=void, typename... Ts, typename F>
static std::shared_ptr<IDelegate> make_shared_lambda_delegate(F && lambda, Ts... args) {
  return std::make_shared<LambdaDelegate<TResult, F, Ts...> >(std::move(lambda), std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static std::shared_ptr<IDelegate> make_shared_lambda_delegate(F && lambda, std::nullptr_t) {
  return std::make_shared<LambdaDelegate<TResult, F, Ts...> >(std::move(lambda), std::nullptr_t{});
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
  std::shared_ptr<TClass> callee, TResult (TClass::*method)(Ts...), std::nullptr_t) {
  return std::make_unique<SharedMethodDelegate<TClass, TResult, Ts...> >(
    callee, method, std::nullptr_t{});
}

template <class TClass, typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  TClass* callee, TResult (TClass::*method)(Ts...), Ts... args) {
  return std::make_unique<MethodDelegate<TClass, TResult, Ts...> >(callee, method, std::forward<Ts>(args)...);
}

template <class TClass, typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  TClass* callee, TResult (TClass::*method)(Ts...), std::nullptr_t) {
  return std::make_unique<MethodDelegate<TClass, TResult, Ts...> >(callee, method, std::nullptr_t{});
}

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  TResult f(Ts...), Ts... args) {
  return std::make_unique<FunctionalDelegate<TResult, Ts...> >(
    std::function<TResult(Ts...)>(f), std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  TResult f(Ts...), std::nullptr_t) {
  return std::make_unique<FunctionalDelegate<TResult, Ts...> >(
    std::function<TResult(Ts...)>(f), std::nullptr_t{});
}

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  std::function<TResult(Ts...)> func, Ts... args) {
  return std::make_unique<FunctionalDelegate<TResult, Ts...> >(func, std::forward<Ts>(args)...);
}

template <typename TResult, typename... Ts>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  std::function<TResult(Ts...)> func, std::nullptr_t) {
  return std::make_unique<FunctionalDelegate<TResult, Ts...> >(func, std::nullptr_t{});
}

template <typename TResult=void, typename... Ts, typename F>
static std::unique_ptr<IDelegate> make_unique_lambda_delegate(F && lambda, Ts... args) {
  return std::make_unique<LambdaDelegate<TResult, F, Ts...> >(std::move(lambda), std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static std::unique_ptr<IDelegate> make_unique_lambda_delegate(F && lambda, std::nullptr_t) {
  return std::make_unique<LambdaDelegate<TResult, F, Ts...> >(std::move(lambda), std::nullptr_t{});
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
  return make_lambda_delegate<TResult, Ts...>(std::move(lambda), std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static IDelegate* make(F && lambda, std::nullptr_t) {
  return make_lambda_delegate<TResult, Ts...>(std::move(lambda), std::nullptr_t{});
}

template <typename TResult=void, typename... Ts>
static ISignal* make_multidelegate(Ts&&... args) {
  return new detail::SignalBase<TResult, Ts...>(std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts>
static ISignal* make_multidelegate(std::nullptr_t) {
  return new detail::SignalBase<TResult, Ts...>(std::nullptr_t{});
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

template <typename TResult=void, typename... Ts, typename F>
static std::shared_ptr<IDelegate> make_shared(F && lambda, Ts&&... args) {
  return make_shared_lambda_delegate<TResult, Ts...>(std::move(lambda), std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts, typename F>
static std::shared_ptr<IDelegate> make_shared(F && lambda, std::nullptr_t) {
  return make_shared_lambda_delegate<TResult, Ts...>(std::move(lambda), std::nullptr_t{});
}

template <typename TResult=void, typename... Ts>
static std::shared_ptr<ISignal> make_shared_multidelegate(Ts&&... args) {
  return std::shared_ptr<ISignal>(make_multidelegate<TResult,Ts...>(std::forward<Ts>(args)...));
}

template <typename TResult=void, typename... Ts>
static std::shared_ptr<ISignal> make_shared_multidelegate(std::nullptr_t) {
  return std::shared_ptr<ISignal>(make_multidelegate<TResult,Ts...>(std::nullptr_t{}));
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
static std::unique_ptr<IDelegate> make_unique(F && lambda, std::nullptr_t) {
  return make_unique_lambda_delegate<TResult, Ts...>(std::move(lambda), std::nullptr_t{});
}

template <typename TResult=void, typename... Ts>
static std::unique_ptr<ISignal> make_unique_multidelegate(Ts&&... args) {
  return std::make_unique<detail::SignalBase<TResult,Ts...> >(std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts>
static std::unique_ptr<ISignal> make_unique_multidelegate(std::nullptr_t) {
  return std::make_unique<detail::SignalBase<TResult,Ts...> >(std::nullptr_t{});
}

}// namespace factory
}//namespace delegates

#endif //DELEGATES_FACTORY_HEADER
