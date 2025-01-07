
#ifndef DELEGATES_DELEGATE_IMPL_HEADER
#define DELEGATES_DELEGATE_IMPL_HEADER

#include <functional>
#include <tuple>
#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <cstddef>

#include "../i_delegate.h"
#include "tuple_runtime.hpp"
#include "delegate_result_impl.hpp"
#include "delegate_args_impl.hpp"

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {

namespace detail {

/// \brief    Delegate base implementation. Void or non-void return types are supported
template<typename TResult, typename... TArgs>
struct DelegateBase
  : public virtual IDelegate {
  DelegateBase(DelegateArgs<TArgs...> && params) : params_(std::move(params)) {}
  ~DelegateBase() = default;

  bool call() override { return perform_call(result_, params_); }
  IDelegateResult* result() override { return static_cast<IDelegateResult*>(&result_); }
  IDelegateArgs* args() override { return static_cast<IDelegateArgs*>(&params_); }

protected:
  // perform_call must be implemented by nested classes
  virtual bool perform_call(DelegateResult<TResult>& result, DelegateArgs<TArgs...>& args) = 0;
private:
  DelegateResult<TResult> result_;
  DelegateArgs<TArgs...> params_;
};


/// \brief    SignalBase implementation
template<typename TResult, typename... TArgs>
class SignalBase : public virtual ISignal {
  SignalBase(const SignalBase&) {}
  SignalBase& operator=(const SignalBase&) { return *this;  }

public:
  SignalBase(DelegateArgs<TArgs...>&& params) : params_(std::move(params)) {}
  ~SignalBase() override { remove_all(); }

  virtual bool call() override {
    bool result = true;

    for(const auto& c : calls_)
      result &= perform_call(c.call_);

    for(const auto& c : shared_calls_)
      result &= perform_call(c.call_.get());

    return result;
  }

  IDelegateResult* result() override { return static_cast<IDelegateResult*>(&result_); }
  IDelegateArgs* args() override { return static_cast<IDelegateArgs*>(&params_); }

  virtual void add(
    IDelegate* call,
    const std::string& tag = std::string(),
    std::function<void(IDelegate*)> deleter = [](IDelegate*){}) override {
    DelegateType c;
    c.call_ = call;
    c.deleter_ = deleter;
    c.tag_ = tag;

    std::lock_guard<std::mutex> lock(mutex_);
    calls_.push_back(c);
  }

  virtual void add(std::shared_ptr<IDelegate> call, const std::string& tag = std::string()) override {
    SharedDelegateType c;
    c.call_ = call;
    c.tag_ = tag;

    std::lock_guard<std::mutex> lock(mutex_);
    shared_calls_.push_back(c);
  }

  virtual void remove(const std::string& tag) override {
    std::lock_guard<std::mutex> lock(mutex_);
    calls_.remove_if([&tag](const DelegateType& c) {
      if (c.tag_ == tag) {
        if (c.deleter_)
          c.deleter_(c.call_);
        return true;
      }
      return false;
    });

    shared_calls_.remove_if([&tag](const SharedDelegateType& c) {
      return c.tag_ == tag;
    });
  }

  virtual void remove(IDelegate* call) override {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!call) return;

    calls_.remove_if([&call](const DelegateType& c) {
      if (c.call_ == call) {
        if (c.deleter_)
          c.deleter_(c.call_);
        return true;
      }
      return false;
    });
  }

  virtual void remove(std::shared_ptr<IDelegate> call) override {
    std::lock_guard<std::mutex> lock(mutex_);
    shared_calls_.remove_if([&call](const SharedDelegateType& c) { return c.call_ == call; });
  }

  virtual void remove_all() override {
    std::lock_guard<std::mutex> lock(mutex_);
    calls_.remove_if([](DelegateType& c) {
      if (c.call_ && c.deleter_) c.deleter_(c.call_);
      c.call_ = nullptr;
      return true;
    });

    shared_calls_.clear();
  }

 private:
  // Copy arguments pointers without deleters. 
  // They are accessible only during source arguments list is alive. 
  // Destination container must be released before source
  static bool args_weak_copy(IDelegateArgs* from, IDelegateArgs* to) {
    if (!from || !to || from->size() != to->size())  return false;

    for (size_t i=0; i<from->size(); i++) {
      if (from->hash_code(i) != to->hash_code(i))
        return false;
    }

    for (size_t i=0; i<from->size(); i++) {
      void* pv = from->get_ptr(i);
      if (!to->set_ptr(i, pv, from->hash_code(i)))
        return false;
    }

    return true;
  }

  // Execute call: check types, create arguments weak copy, pass them to call, execute call, release arguments, copy result
  bool perform_call(IDelegate* call) {
    using result_noref = typename std::decay<TResult>::type;
    std::lock_guard<std::mutex> lock(mutex_);

    if (call->result()->hash_code() != typeid(TResult).hash_code())
      return false;

    if (!args_weak_copy(&params_, call->args()))
      return false;

    bool ret = false;

    try {
      ret = call->call();
    } catch (...) {
      call->args()->clear(); // clear arguments weak copy
      throw;
    }

    return ret ? MoveDelegateResult<TResult>{}(call->result(), result()) : false;
  }

  struct SharedDelegateType {
    std::shared_ptr<IDelegate> call_;
    std::string tag_;
  };

  struct DelegateType {
    IDelegate* call_ = nullptr;
    std::function<void(IDelegate*)> deleter_ = [](IDelegate*){};
    std::string tag_;
  };

  DelegateResult<TResult> result_;
  DelegateArgs<TArgs...> params_;
  std::list<SharedDelegateType> shared_calls_;
  std::list<DelegateType> calls_;
  mutable std::mutex mutex_;
};

}//namespace detail


using namespace detail;

/// \brief    Class method deferred call template for all result types but void
template <class TClass, typename TResult, typename... TArgs>
class MethodDelegate 
  : public detail::DelegateBase<TResult, TArgs...> {
 private:
  typedef TResult (TClass::*MethodType)(TArgs...);
  TClass* callee_;
  MethodType method_;

 public:
  MethodDelegate(TClass* owner, TResult (TClass::* member)(TArgs...), DelegateArgs<TArgs...>&& params)
    :detail::DelegateBase<TResult, TArgs...>(std::move(params))
    , callee_(owner)
    , method_(member) {}

  ~MethodDelegate() = default;

 private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<TArgs...>& args) override {
    return perform_call(result, args, std::make_index_sequence<sizeof...(TArgs)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<TArgs...>& args, std::index_sequence<Is...>) {
    auto callee = callee_;
    if (!callee) return false;

    result.set((callee->*(method_))(std::get<Is>(args.get_tuple())...));
    return true;
  }
};


/// \brief    Class const method deferred call template for all result types but void
template <class TClass, typename TResult, typename... TArgs>
class ConstMethodDelegate
  : public detail::DelegateBase<TResult, TArgs...> {
 public:
  ConstMethodDelegate(const TClass* owner, TResult(TClass::* member)(TArgs...) const, DelegateArgs<TArgs...>&& params)
    :detail::DelegateBase<TResult, TArgs...>(std::move(params))
    , callee_(owner)
    , method_(member) {}

  ~ConstMethodDelegate() = default;

 private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<TArgs...>& args) override {
    return perform_call(result, args, std::make_index_sequence<sizeof...(TArgs)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<TArgs...>& args, std::index_sequence<Is...>) {
    auto callee = callee_;
    if (!callee)
      return false;

    result.set((callee->*(method_))(std::get<Is>(args.get_tuple())...));
    return true;
  }

  typedef TResult (TClass::*MethodType)(TArgs...) const;
  const TClass* callee_;
  MethodType method_;
};


/// \brief    Class const method deferred call template for void result type
template <class TClass, typename... TArgs>
class ConstMethodDelegate<TClass, void, TArgs...>
  : public detail::DelegateBase<void, TArgs...> {
public:
  ConstMethodDelegate(const TClass* owner, void(TClass::* member)(TArgs...) const, DelegateArgs<TArgs...>&& params)
    :detail::DelegateBase<void, TArgs...>(std::move(params))
    , callee_(owner)
    , method_(member) {}

  ~ConstMethodDelegate() = default;

private:
  bool perform_call(DelegateResult<void>&, DelegateArgs<TArgs...>& args) override {
    return perform_call(args, std::make_index_sequence<sizeof...(TArgs)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateArgs<TArgs...>& args, std::index_sequence<Is...>) {
    auto callee = callee_;
    if (!callee)
      return false;

    (callee->*(method_))(std::get<Is>(args.get_tuple())...);
    return true;
  }

  typedef void(TClass::* MethodType)(TArgs...) const;
  const TClass* callee_;
  MethodType method_;
};


/// \brief    Class method deferred call for 'void' return type
template <class TClass, typename... TArgs>
class MethodDelegate<TClass, void, TArgs...>
  : public detail::DelegateBase<void, TArgs...> {
 public:
  MethodDelegate(TClass* owner, void (TClass::* member)(TArgs...), DelegateArgs<TArgs...>&& params)
    :detail::DelegateBase<void, TArgs...>(std::move(params))
    , callee_(owner)
    , method_(member) {}

  ~MethodDelegate() = default;

 private:
  bool perform_call(DelegateResult<void>&, DelegateArgs<TArgs...>& args) override {
    return perform_call(args, std::make_index_sequence<sizeof...(TArgs)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateArgs<TArgs...>& args, std::index_sequence<Is...>) {
    auto callee = callee_;
    if (!callee) return false;

    (callee->*(method_))(std::get<Is>(args.get_tuple())...);
    return true;
  }

  typedef void (TClass::*MethodType)(TArgs...);
  TClass* callee_;
  MethodType method_;
};


/// \brief    Functional delegate implementation for all result types but void
template <typename TResult, typename... TArgs>
class FunctionalDelegate
  : public detail::DelegateBase<TResult, TArgs...> {
public:
  FunctionalDelegate(std::function<TResult(TArgs...)> func, DelegateArgs<TArgs...>&& params)
    :func_(func)
    ,detail::DelegateBase<TResult, TArgs...>(std::move(params)) {
  }

  ~FunctionalDelegate() override = default;
private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<TArgs...>& args) override {
    return perform_call(result, args, std::make_index_sequence<sizeof...(TArgs)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<TArgs...>& args, std::index_sequence<Is...>) {
    result.set(func_(std::get<Is>(args.get_tuple())...));
    return true;
  }

  std::function<TResult(TArgs...)> func_;
};


/// \brief    Funnctional delegate implementation for void result type
template <typename... TArgs>
class FunctionalDelegate<void, TArgs...>
  : public detail::DelegateBase<void,TArgs...> {
 public:
  FunctionalDelegate(std::function<void(TArgs...)> func, DelegateArgs<TArgs...>&& params)
    :func_(func)
    , detail::DelegateBase<void, TArgs...>(std::move(params)) {
  }

  ~FunctionalDelegate() = default;
private:
  bool perform_call(DelegateResult<void>& result, DelegateArgs<TArgs...>& args) override {
    perform_call(args, std::make_index_sequence<sizeof...(TArgs)>{});
    return true;
  }

  template<std::size_t... Is>
  void perform_call(DelegateArgs<TArgs...>& args, std::index_sequence<Is...>) {
    func_(std::get<Is>(args.get_tuple())...);
  }

  std::function<void(TArgs...)> func_;
};


/// \brief    Lambda delegate implementation for non-void result types
template <typename TResult, typename F, typename... TArgs>
class LambdaDelegate
  : public detail::DelegateBase<TResult,TArgs...> {
public:
  LambdaDelegate(F&& lambda, DelegateArgs<TArgs...> && params)
    :func_(std::function<TResult(TArgs...)>(std::move(lambda)))
    , detail::DelegateBase<TResult, TArgs...>(std::move(params)) {
  }

  ~LambdaDelegate() override = default;

private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<TArgs...>& args) override {
    result.set(perform_function_call(args.get_tuple(), std::make_index_sequence<sizeof...(TArgs)>{}));
    return true;
  }

  template <std::size_t... Is>
  TResult perform_function_call(typename std::tuple<TArgs...>& tup, std::index_sequence<Is...>) {
    return func_(std::get<Is>(tup)...);
  }

  std::function<TResult(TArgs...)> func_;
};


/// \brief    Lambda delegate implementation for void result type
template <typename F, typename... TArgs>
class LambdaDelegate<void, F, TArgs...>
  : public detail::DelegateBase<void, TArgs...> {
public:
  LambdaDelegate(F&& lambda, DelegateArgs<TArgs...>&& params)
    :func_(std::function<void(TArgs...)>(std::move(lambda)))
    , detail::DelegateBase<void, TArgs...>(std::move(params)) {
  }

  ~LambdaDelegate() override = default;

private:
  bool perform_call(DelegateResult<void>&, DelegateArgs<TArgs...>& args) override {
    perform_function_call(args.get_tuple(), std::make_index_sequence<sizeof...(TArgs)>{});
    return true;
  }

  template <std::size_t... Is>
  void perform_function_call(typename std::tuple<TArgs...>& tup, std::index_sequence<Is...>) {
    func_(std::get<Is>(tup)...);
  }

  std::function<void(TArgs...)> func_;
};


/// \brief    Delegate for class method call by shared pointer implementation. Both void and non-void result types are supported
template <class TClass, typename TResult, typename... TArgs>
class SharedMethodDelegate
  : public MethodDelegate<TClass, TResult, TArgs...> {
 public:
  SharedMethodDelegate(std::shared_ptr<TClass> callee,
    TResult(TClass::* method)(TArgs...), DelegateArgs<TArgs...> && params)
    :MethodDelegate<TClass, TResult, TArgs...>(callee.get(), method, std::move(params))
    , callee_ptr_(callee) {}

  ~SharedMethodDelegate() = default;
 private:
  std::shared_ptr<TClass> callee_ptr_;
};


/// \brief    Delegate for class method call by shared pointer implementation. Both void and non-void result types are supported
template <class TClass, typename TResult, typename... TArgs>
class SharedConstMethodDelegate
  : public ConstMethodDelegate<TClass, TResult, TArgs...> {
public:
  SharedConstMethodDelegate(std::shared_ptr<TClass> callee,
    TResult(TClass::* method)(TArgs...) const, DelegateArgs<TArgs...> && params)
    :ConstMethodDelegate<TClass, TResult, TArgs...>(callee.get(), method, std::move(params))
    , callee_ptr_(callee) {}

  ~SharedConstMethodDelegate() = default;
private:
  std::shared_ptr<TClass> callee_ptr_;
};


/// \brief    Delegate for class method call by shared pointer implementation. Non-void result types are supported
template <class TClass, typename TResult, typename... TArgs>
class WeakMethodDelegate
  : public detail::DelegateBase<TResult,TArgs...> {
 public:
  WeakMethodDelegate(std::weak_ptr<TClass> callee, TResult(TClass::* method)(TArgs...), DelegateArgs<TArgs...> && params)
    :detail::DelegateBase<TResult, TArgs...>(std::move(params))
    ,callee_(callee)
    ,method_(method) {}

  ~WeakMethodDelegate() = default;
 private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<TArgs...>& args) override {
    return perform_call(result, args.get_tuple(), std::make_index_sequence<sizeof...(TArgs)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, std::tuple<TArgs...>& tup, std::index_sequence<Is...>) {
    auto callee = callee_.lock();
    if (!callee)
      return false;
    result.set((callee.get()->*(method_))(std::get<Is>(tup)...));
    return true;
  }

  typedef TResult (TClass::*MethodType)(TArgs...);
  std::weak_ptr<TClass> callee_;
  MethodType method_;
};


/// \brief    Delegate for class method call by shared pointer implementation. For void result type
template <class TClass, typename... TArgs>
class WeakMethodDelegate<TClass,void,TArgs...>
  : public detail::DelegateBase<void,TArgs...> {
 public:
  WeakMethodDelegate(std::weak_ptr<TClass> callee, void(TClass::*method)(TArgs...), DelegateArgs<TArgs...>&& params)
    :detail::DelegateBase<void,TArgs...>(std::move(params))
    ,callee_(callee)
    ,method_(method) {}

  ~WeakMethodDelegate() = default;
 private:
  template <std::size_t... Is>
  bool perform_call(std::tuple<TArgs...>& tup, std::index_sequence<Is...>) {
    auto callee = callee_.lock();
    if (!callee)
      return false;
    (callee.get()->*(method_))(std::get<Is>(tup)...);
    return true;
  }

  bool perform_call(DelegateResult<void>& result, DelegateArgs<TArgs...>& args) override {
    return perform_call(args.get_tuple(), std::make_index_sequence<sizeof...(TArgs)>{});
  }

  typedef void (TClass::*MethodType)(TArgs...);
  std::weak_ptr<TClass> callee_;
  MethodType method_;
};

}//namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif  // DELEGATES_DELEGATE_IMPL_HEADER
