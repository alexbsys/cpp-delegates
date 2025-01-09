
#ifndef DELEGATES_FACTORY_HEADER
#define DELEGATES_FACTORY_HEADER

#include "delegate_impl.hpp"
#include <memory>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {
namespace factory {

// raw pointers

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make_method_delegate(TClass* callee,
                                        TResult (TClass::*method)(TArgs...),
                                        TArgs... args) {
  return new MethodDelegate<TClass, TResult, TArgs...>(callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static IDelegate* make_method_delegate(TClass* callee,
                                       TResult (TClass::*method)(TArgs...),
                                       DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return new MethodDelegate<TClass, TResult, TArgs...>(callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make_const_method_delegate(const TClass* callee,
                                        TResult (TClass::*method)(TArgs...) const,
                                        TArgs... args) {
  return new ConstMethodDelegate<TClass, TResult, TArgs...>(callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static IDelegate* make_const_method_delegate(const TClass* callee,
                                             TResult (TClass::*method)(TArgs...) const,
                                             DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return new ConstMethodDelegate<TClass, TResult, TArgs...>(callee, method, std::move(params));
}

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make_function_delegate(TResult f(TArgs...), TArgs... args) {
  return new FunctionalDelegate<TResult, TArgs...>(std::function<TResult(TArgs...)>(f), DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult, typename... TArgs>
static IDelegate* make_function_delegate(TResult f(TArgs...), DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return new FunctionalDelegate<TResult, TArgs...>(std::function<TResult(TArgs...)>(f), std::move(params));
}

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make_function_delegate(std::function<TResult(TArgs...)> func, TArgs... args) {
  return new FunctionalDelegate<TResult, TArgs...>(func, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult, typename... TArgs>
static IDelegate* make_function_delegate(std::function<TResult(TArgs...)> func, DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return new FunctionalDelegate<TResult, TArgs...>(func, std::move(params));
}

template <typename TResult=void, typename... TArgs, typename F>
static IDelegate* make_lambda_delegate(F && lambda, DelegateArgs<TArgs...> && params = DelegateArgs<TArgs...>()) {
  return new LambdaDelegate<TResult, F, TArgs...>(std::move(lambda), std::move(params));
}

// class instance pointer is weak_ptr

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make_method_delegate(std::weak_ptr<TClass> callee,
                                       TResult (TClass::*method)(TArgs...),
                                       TArgs... args) {
  return new WeakMethodDelegate<TClass, TResult, TArgs...>(
    callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static IDelegate* make_method_delegate(std::weak_ptr<TClass> callee,
                                       TResult (TClass::*method)(TArgs...),
                                       DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return new WeakMethodDelegate<TClass, TResult, TArgs...>(
    callee, method, std::move(params));
}

// class instance pointer is shared ptr

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make_method_delegate(std::shared_ptr<TClass> callee,
                                       TResult (TClass::*method)(TArgs...),
                                       TArgs... args) {
  return new SharedMethodDelegate<TClass, TResult, TArgs...>(
    callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static IDelegate* make_method_delegate(std::shared_ptr<TClass> callee,
                                       TResult (TClass::*method)(TArgs...),
                                       DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return new SharedMethodDelegate<TClass, TResult, TArgs...>(
    callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make_const_method_delegate(std::shared_ptr<TClass> callee,
                                             TResult(TClass::* method)(TArgs...) const,
                                             TArgs... args) {
  return new SharedConstMethodDelegate<TClass, TResult, TArgs...>(
    callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static IDelegate* make_const_method_delegate(std::shared_ptr<TClass> callee,
                                             TResult(TClass::* method)(TArgs...) const,
                                             DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return new SharedConstMethodDelegate<TClass, TResult, TArgs...>(
    callee, method, std::move(params));
}

// shared

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  std::shared_ptr<TClass> callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return std::make_shared<SharedMethodDelegate<TClass, TResult, TArgs...> >(
    callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  std::shared_ptr<TClass> callee, 
  TResult (TClass::*method)(TArgs...), 
  DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return std::make_shared<SharedMethodDelegate<TClass, TResult, TArgs...> >(
    callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  TClass* callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return std::make_shared<MethodDelegate<TClass, TResult, TArgs...> >(callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared_method_delegate(
  TClass* callee, TResult (TClass::*method)(TArgs...), DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return std::make_shared<MethodDelegate<TClass, TResult, TArgs...> >(callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(
  const TClass* callee, TResult(TClass::* method)(TArgs...) const,  TArgs... args) {
  return std::make_shared<ConstMethodDelegate<TClass, TResult, TArgs...> >(callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(const TClass* callee, TResult(TClass::* method)(TArgs...) const,
  DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return std::make_shared<ConstMethodDelegate<TClass, TResult, TArgs...> >(callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(
  std::shared_ptr<TClass> callee, TResult(TClass::* method)(TArgs...) const, TArgs... args) {
  return std::make_shared<SharedConstMethodDelegate<TClass, TResult, TArgs...> >(
    callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared_const_method_delegate(std::shared_ptr<TClass> callee, TResult(TClass::* method)(TArgs...) const,
  DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return std::make_shared<SharedConstMethodDelegate<TClass, TResult, TArgs...> >(callee, method, std::move(params));
}


template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  TResult f(TArgs...), TArgs... args) {
  return std::make_shared<FunctionalDelegate<TResult, TArgs...> >(
    std::function<TResult(TArgs...)>(f), DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  TResult f(TArgs...), DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return std::make_shared<FunctionalDelegate<TResult, TArgs...> >(
    std::function<TResult(TArgs...)>(f), std::move(params));
}

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  std::function<TResult(TArgs...)> func, TArgs... args) {
  return std::make_shared<FunctionalDelegate<TResult, TArgs...> >(func, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared_function_delegate(
  std::function<TResult(TArgs...)> func, DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return std::make_shared<FunctionalDelegate<TResult, TArgs...> >(func, std::move(params));
}

template <typename TResult=void, typename... TArgs, typename F, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared_lambda_delegate(F && lambda, TArgs... args) {
  return std::make_shared<LambdaDelegate<TResult, F, TArgs...> >(std::move(lambda), DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult=void, typename... TArgs, typename F>
static std::shared_ptr<IDelegate> make_shared_lambda_delegate(F && lambda, DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return std::make_shared<LambdaDelegate<TResult, F, TArgs...> >(std::move(lambda), std::move(params));
}

// unique

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  std::shared_ptr<TClass> callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return std::make_unique<SharedMethodDelegate<TClass, TResult, TArgs...> >(
    callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  std::shared_ptr<TClass> callee, TResult (TClass::*method)(TArgs...), DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return std::make_unique<SharedMethodDelegate<TClass, TResult, TArgs...> >(
    callee, method, std::move(params));
}

template <class TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  TClass* callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return std::make_unique<MethodDelegate<TClass, TResult, TArgs...> >(callee, method, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <class TClass, typename TResult, typename... TArgs>
static std::unique_ptr<IDelegate> make_unique_method_delegate(
  TClass* callee, TResult (TClass::*method)(TArgs...), DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return std::make_unique<MethodDelegate<TClass, TResult, TArgs...> >(callee, method, std::move(params));
}

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  TResult f(TArgs...), TArgs... args) {
  return std::make_unique<FunctionalDelegate<TResult, TArgs...> >(
    std::function<TResult(TArgs...)>(f), DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult, typename... TArgs>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  TResult f(TArgs...), DelegateArgs<TArgs...> && params = DelegateArgs<TArgs...>()) {
  return std::make_unique<FunctionalDelegate<TResult, TArgs...> >(
    std::function<TResult(TArgs...)>(f), std::move(params));
}

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  std::function<TResult(TArgs...)> func, TArgs... args) {
  return std::make_unique<FunctionalDelegate<TResult, TArgs...> >(func, DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult, typename... TArgs>
static std::unique_ptr<IDelegate> make_unique_function_delegate(
  std::function<TResult(TArgs...)> func, DelegateArgs<TArgs...> && params = DelegateArgs<TArgs...>()) {
  return std::make_unique<FunctionalDelegate<TResult, TArgs...> >(func, std::move(params));
}

template <typename TResult=void, typename... TArgs, typename F, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique_lambda_delegate(F && lambda, TArgs... args) {
  return std::make_unique<LambdaDelegate<TResult, F, TArgs...> >(std::move(lambda), DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult=void, typename... TArgs, typename F>
static std::unique_ptr<IDelegate> make_unique_lambda_delegate(F && lambda, DelegateArgs<TArgs...> && params = DelegateArgs<TArgs...>()) {
  return std::make_unique<LambdaDelegate<TResult, F, TArgs...> >(std::move(lambda), std::move(params));
}

// autodetect raw

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make(TResult f(TArgs...), TArgs... args) {
  return make_function_delegate(f,std::forward<TArgs>(args)...);
}

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make(std::function<TResult(TArgs...)> func, TArgs... args) {
  return make_function_delegate(func,std::forward<TArgs>(args)...);
}

template <typename TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make(TClass* callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return make_method_delegate(callee, method, std::forward<TArgs>(args)...);
}

template <typename TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make(std::shared_ptr<TClass> callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return make_method_delegate<TClass, TResult, TArgs...>(callee, method, std::forward<TArgs>(args)...);
}

template <typename TResult=void, typename... TArgs, typename F, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static IDelegate* make(F && lambda, TArgs&&... args) {
  return make_lambda_delegate<TResult, TArgs...>(std::move(lambda), 
    DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult=void, typename... TArgs, typename F>
static IDelegate* make(F && lambda, DelegateArgs<TArgs...> && params = DelegateArgs<TArgs...>()) {
  return make_lambda_delegate<TResult, TArgs...>(std::move(lambda), std::move(params));
}

template <typename TResult=void, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static ISignal* make_signal(TArgs&&... args) {
  return new detail::SignalBase<TResult, TArgs...>(DelegateArgs<TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult=void, typename... TArgs>
static ISignal* make_signal(DelegateArgs<TArgs...> && params = DelegateArgs<TArgs...>()) {
  return new detail::SignalBase<TResult, TArgs...>(std::move(params));
}

// autodetect shared

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared(TResult f(TArgs...), TArgs... args) {
  return make_shared_function_delegate(f,std::forward<TArgs>(args)...);
}

template <typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared(TResult f(TArgs...), DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return make_shared_function_delegate(f,std::move(params));
}

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared(std::function<TResult(TArgs...)> func, TArgs... args) {
  return make_shared_function_delegate(func,std::forward<TArgs>(args)...);
}

template <typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared(std::function<TResult(TArgs...)> func, DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return make_shared_function_delegate(func,std::move(params));
}

template <typename TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared(TClass* callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return make_shared_method_delegate(callee, method, std::forward<TArgs>(args)...);
}

template <typename TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared(std::shared_ptr<TClass> callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return make_shared_method_delegate<TClass, TResult, TArgs...>(callee, method, std::forward<TArgs>(args)...);
}

template <typename TClass, typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared(std::shared_ptr<TClass> callee, TResult(TClass::* method)(TArgs...), DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return make_shared_method_delegate<TClass, TResult, TArgs...>(callee, method, std::move(params));
}

template <typename TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared(const TClass* callee, TResult(TClass::* method)(TArgs...) const, TArgs... args) {
  return make_shared_const_method_delegate<TClass, TResult, TArgs...>(callee, method, std::forward<TArgs>(args)...);
}

template <typename TClass, typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared(const TClass* callee, TResult(TClass::* method)(TArgs...) const, DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return make_shared_const_method_delegate<TClass, TResult, TArgs...>(callee, method, std::move(params));
}

template <typename TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared(std::shared_ptr<TClass> callee, TResult(TClass::* method)(TArgs...) const, TArgs... args) {
  return make_shared_const_method_delegate<TClass, TResult, TArgs...>(callee, method, std::forward<TArgs>(args)...);
}

template <typename TClass, typename TResult, typename... TArgs>
static std::shared_ptr<IDelegate> make_shared(std::shared_ptr<TClass> callee, TResult(TClass::* method)(TArgs...) const, DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return make_shared_const_method_delegate<TClass, TResult, TArgs...>(callee, method, std::move(params));
}

template <typename TResult=void, typename... TArgs, typename F, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::shared_ptr<IDelegate> make_shared(F && lambda, TArgs&&... args) {
  return make_shared_lambda_delegate<TResult, TArgs...>(std::move(lambda), std::forward<TArgs>(args)...);
}

template <typename TResult=void, typename... TArgs, typename F>
static std::shared_ptr<IDelegate> make_shared(F&& lambda, DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) {
  return make_shared_lambda_delegate<TResult, TArgs...>(std::move(lambda), std::move(params));
}

template <typename TResult=void, typename... TArgs>
static std::shared_ptr<ISignal> make_shared_signal(TArgs&&... args) {
  return std::shared_ptr<ISignal>(make_signal<TResult,TArgs...>(std::forward<TArgs>(args)...));
}

template <typename TResult=void, typename... TArgs>
static std::shared_ptr<ISignal> make_shared_signal(DelegateArgs<TArgs...> && params = DelegateArgs<TArgs...>()) {
  return std::shared_ptr<ISignal>(make_signal<TResult,TArgs...>(std::move(params)));
}

// autodetect unique

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique(TResult f(TArgs...), TArgs... args) {
  return make_unique_function_delegate(f,std::forward<TArgs>(args)...);
}

template <typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique(std::function<TResult(TArgs...)> func, TArgs... args) {
  return make_unique_function_delegate(func,std::forward<TArgs>(args)...);
}

template <typename TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique(TClass* callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return make_unique_method_delegate(callee, method, std::forward<TArgs>(args)...);
}

template <typename TClass, typename TResult, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique(std::shared_ptr<TClass> callee, TResult (TClass::*method)(TArgs...), TArgs... args) {
  return make_unique_method_delegate<TClass, TResult, TArgs...>(callee, method, std::forward<TArgs>(args)...);
}

template <typename TResult=void, typename... TArgs, typename F, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<IDelegate> make_unique(F && lambda, TArgs&&... args) {
  return make_unique_lambda_delegate<TResult, TArgs...>(std::move(lambda), std::forward<TArgs>(args)...);
}

template <typename TResult=void, typename... TArgs, typename F>
static std::unique_ptr<IDelegate> make_unique(F && lambda, DelegateArgs<TArgs...> && params = DelegateArgs<TArgs...>()) {
  return make_unique_lambda_delegate<TResult, TArgs...>(std::move(lambda), std::move(params));
}

template <typename TResult=void, typename... TArgs, bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
static std::unique_ptr<ISignal> make_unique_signal(TArgs&&... args) {
  return std::make_unique<detail::SignalBase<TResult,TArgs...> >(std::forward<TArgs>(args)...);
}

template <typename TResult=void, typename... TArgs>
static std::unique_ptr<ISignal> make_unique_signal(DelegateArgs<TArgs...> && params = DelegateArgs<TArgs...>()) {
  return std::make_unique<detail::SignalBase<TResult,TArgs...> >(std::move(params));
}

}// namespace factory
}//namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif //DELEGATES_FACTORY_HEADER
