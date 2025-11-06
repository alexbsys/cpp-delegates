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

#ifndef DELEGATES_FACTORY_HEADER
#define DELEGATES_FACTORY_HEADER

#include "delegate_impl.hpp"
#include "function_traits.hpp"
#include "callable_traits.hpp"
#include "../typed_delegate.hpp"
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

// ============================================================================
// New unified API with automatic type deduction
// ============================================================================

/// \brief    Create delegate with automatic type deduction from callable
///           Returns TypedDelegate for convenient direct calls
template<typename F>
auto make_delegate_auto(F&& callable) {
    using traits = detail::function_traits<std::decay_t<F>>;
    return make_delegate_auto_impl(
        detail::callable_category_t<std::decay_t<F>>{},
        std::forward<F>(callable),
        std::make_index_sequence<traits::args_count>{}
    );
}

// Implementation with tag dispatch
template<typename F, size_t... Is>
auto make_delegate_auto_impl(detail::lambda_tag, F&& lambda, 
                              std::index_sequence<Is...>) {
    using traits = detail::function_traits<std::decay_t<F>>;
    IDelegate* raw = make_lambda_delegate<
        traits::result_type, 
        traits::template arg_type<Is>...
    >(std::forward<F>(lambda));
    return TypedDelegate<traits::result_type, traits::template arg_type<Is>...>(raw, true);
}

template<typename F, size_t... Is>
auto make_delegate_auto_impl(detail::function_tag, F&& func,
                              std::index_sequence<Is...>) {
    using traits = detail::function_traits<std::decay_t<F>>;
    IDelegate* raw = make_function_delegate<
        traits::result_type,
        traits::template arg_type<Is>...
    >(std::forward<F>(func));
    return TypedDelegate<traits::result_type, traits::template arg_type<Is>...>(raw, true);
}

/// \brief    Create typed delegate with explicit type specification
///           Use this when you need control over reference types
template<typename Result, typename... Args, typename F>
TypedDelegate<Result, Args...> make_delegate(F&& callable) {
    IDelegate* raw = make_delegate_impl<Result, Args...>(
        detail::callable_category_t<std::decay_t<F>>{},
        std::forward<F>(callable)
    );
    return TypedDelegate<Result, Args...>(raw, true);
}

// Implementation with tag dispatch for explicit types
template<typename Result, typename... Args, typename F>
IDelegate* make_delegate_impl(detail::lambda_tag, F&& lambda) {
    return make_lambda_delegate<Result, Args...>(std::forward<F>(lambda));
}

template<typename Result, typename... Args, typename F>
IDelegate* make_delegate_impl(detail::function_tag, F&& func) {
    return make_function_delegate<Result, Args...>(std::forward<F>(func));
}

/// \brief    Create typed delegate from method pointer
template<typename Class, typename Result, typename... Args>
TypedDelegate<Result, Args...> make_delegate(Class* obj, Result(Class::*method)(Args...)) {
    IDelegate* raw = make_method_delegate<Class, Result, Args...>(obj, method);
    return TypedDelegate<Result, Args...>(raw, true);
}

template<typename Class, typename Result, typename... Args>
TypedDelegate<Result, Args...> make_delegate(std::shared_ptr<Class> obj, Result(Class::*method)(Args...)) {
    IDelegate* raw = make_method_delegate<Class, Result, Args...>(obj, method);
    return TypedDelegate<Result, Args...>(raw, true);
}

template<typename Class, typename Result, typename... Args>
TypedDelegate<Result, Args...> make_delegate(const Class* obj, Result(Class::*method)(Args...) const) {
    IDelegate* raw = make_const_method_delegate<Class, Result, Args...>(obj, method);
    return TypedDelegate<Result, Args...>(raw, true);
}

template<typename Class, typename Result, typename... Args>
TypedDelegate<Result, Args...> make_delegate(std::shared_ptr<Class> obj, Result(Class::*method)(Args...) const) {
    IDelegate* raw = make_const_method_delegate<Class, Result, Args...>(obj, method);
    return TypedDelegate<Result, Args...>(raw, true);
}

/// \brief    Create shared typed delegate with automatic type deduction
template<typename F>
auto make_delegate_shared_auto(F&& callable) {
    using traits = detail::function_traits<std::decay_t<F>>;
    return make_delegate_shared_auto_impl(
        detail::callable_category_t<std::decay_t<F>>{},
        std::forward<F>(callable),
        std::make_index_sequence<traits::args_count>{}
    );
}

template<typename F, size_t... Is>
auto make_delegate_shared_auto_impl(detail::lambda_tag, F&& lambda,
                                     std::index_sequence<Is...>) {
    using traits = detail::function_traits<std::decay_t<F>>;
    auto raw = make_shared_lambda_delegate<
        traits::result_type,
        traits::template arg_type<Is>...
    >(std::forward<F>(lambda));
    return std::make_shared<TypedDelegate<traits::result_type, traits::template arg_type<Is>...>>(raw);
}

template<typename F, size_t... Is>
auto make_delegate_shared_auto_impl(detail::function_tag, F&& func,
                                    std::index_sequence<Is...>) {
    using traits = detail::function_traits<std::decay_t<F>>;
    auto raw = make_shared_function_delegate<
        traits::result_type,
        traits::template arg_type<Is>...
    >(std::forward<F>(func));
    return std::make_shared<TypedDelegate<traits::result_type, traits::template arg_type<Is>...>>(raw);
}

/// \brief    Create shared typed delegate with explicit types
template<typename Result, typename... Args, typename F>
std::shared_ptr<TypedDelegate<Result, Args...>> make_delegate_shared(F&& callable) {
    auto raw = make_shared_delegate_impl<Result, Args...>(
        detail::callable_category_t<std::decay_t<F>>{},
        std::forward<F>(callable)
    );
    return std::make_shared<TypedDelegate<Result, Args...>>(raw);
}

template<typename Result, typename... Args, typename F>
std::shared_ptr<IDelegate> make_shared_delegate_impl(detail::lambda_tag, F&& lambda) {
    return make_shared_lambda_delegate<Result, Args...>(std::forward<F>(lambda));
}

template<typename Result, typename... Args, typename F>
std::shared_ptr<IDelegate> make_shared_delegate_impl(detail::function_tag, F&& func) {
    return make_shared_function_delegate<Result, Args...>(std::forward<F>(func));
}

/// \brief    Create unique typed delegate with automatic type deduction
template<typename F>
auto make_delegate_unique_auto(F&& callable) {
    using traits = detail::function_traits<std::decay_t<F>>;
    return make_delegate_unique_auto_impl(
        detail::callable_category_t<std::decay_t<F>>{},
        std::forward<F>(callable),
        std::make_index_sequence<traits::args_count>{}
    );
}

template<typename F, size_t... Is>
auto make_delegate_unique_auto_impl(detail::lambda_tag, F&& lambda,
                                    std::index_sequence<Is...>) {
    using traits = detail::function_traits<std::decay_t<F>>;
    auto raw = make_unique_lambda_delegate<
        traits::result_type,
        traits::template arg_type<Is>...
    >(std::forward<F>(lambda));
    return TypedDelegate<traits::result_type, traits::template arg_type<Is>...>(std::move(raw));
}

template<typename F, size_t... Is>
auto make_delegate_unique_auto_impl(detail::function_tag, F&& func,
                                    std::index_sequence<Is...>) {
    using traits = detail::function_traits<std::decay_t<F>>;
    auto raw = make_unique_function_delegate<
        traits::result_type,
        traits::template arg_type<Is>...
    >(std::forward<F>(func));
    return TypedDelegate<traits::result_type, traits::template arg_type<Is>...>(std::move(raw));
}

/// \brief    Create unique typed delegate with explicit types
template<typename Result, typename... Args, typename F>
TypedDelegate<Result, Args...> make_delegate_unique(F&& callable) {
    auto raw = make_unique_delegate_impl<Result, Args...>(
        detail::callable_category_t<std::decay_t<F>>{},
        std::forward<F>(callable)
    );
    return TypedDelegate<Result, Args...>(std::move(raw));
}

template<typename Result, typename... Args, typename F>
std::unique_ptr<IDelegate> make_unique_delegate_impl(detail::lambda_tag, F&& lambda) {
    return make_unique_lambda_delegate<Result, Args...>(std::forward<F>(lambda));
}

template<typename Result, typename... Args, typename F>
std::unique_ptr<IDelegate> make_unique_delegate_impl(detail::function_tag, F&& func) {
    return make_unique_function_delegate<Result, Args...>(std::forward<F>(func));
}

}// namespace factory
}//namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif //DELEGATES_FACTORY_HEADER
