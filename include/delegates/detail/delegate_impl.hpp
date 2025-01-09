
#ifndef DELEGATES_DELEGATE_IMPL_HEADER
#define DELEGATES_DELEGATE_IMPL_HEADER

#include <functional>
#include <tuple>
#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <cstddef>

#if DELEGATES_STRICT
#include <stdexcept>
#endif //DELEGATES_STRICT

#if DELEGATES_TRACE
#include <iostream>
#endif //DELEGATES_TRACE

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
  
  bool call(IDelegateArgs* args) override { 
    if (!args || args->size() != params_.size()) {
#if DELEGATES_TRACE
      std::cerr << "Null or wrong arguments count provided to call()" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Null or wrong arguments count provided to call()");
#endif //DELEGATES_STRICT

      return false;
    }
    for (size_t i = 0; i < params_.size(); i++) {
      if (args->hash_code(i) != params_.hash_code(i)) {
#if DELEGATES_TRACE
        std::cerr << "Wrong argument type provided to call, argument number " << i << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
        throw std::runtime_error("Wrong argument type provided to call");
#endif //DELEGATES_STRICT

        return false;
      }
    }

    return perform_call(result_, *static_cast<DelegateArgs<TArgs...>*>(args)); 
  }
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

  void get_all(std::vector<IDelegate*>& delegates) const override {
    delegates.clear();

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& c : calls_)
      delegates.push_back(c.call_);

    for (const auto& c : shared_calls_)
      delegates.push_back(c.call_.get());
  }

  virtual bool call() override {
    return call(nullptr);
  }

  virtual bool call(IDelegateArgs* args) override {
    std::list<SharedDelegateType> shared_calls;
    std::list<DelegateType> calls;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      shared_calls = shared_calls_;
      calls = calls_;
    }

    bool result = true;

    for(const auto& c : calls)
      result &= perform_call(c.call_, args, c.args_mode_);

    for(const auto& c : shared_calls)
      result &= perform_call(c.call_.get(), args, c.args_mode_);

    return result;
  }

  IDelegateResult* result() override { return static_cast<IDelegateResult*>(&result_); }
  IDelegateArgs* args() override { return static_cast<IDelegateArgs*>(&params_); }

  virtual void add(
    IDelegate* call,
    const std::string& tag = std::string(),
    DelegateArgsMode args_mode = kDelegateArgsMode_Auto,
    std::function<void(IDelegate*)> deleter = [](IDelegate*){}) override {
    
    if (!call) {
#if DELEGATES_TRACE
      std::cerr << "Null delegate provided to remove()" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Null delegate provided to remove()");
#endif //DELEGATES_STRICT
      return;
    }
    
    DelegateType c;
    c.call_ = call;
    c.deleter_ = deleter;
    c.tag_ = tag;
    c.args_mode_ = args_mode;

    std::lock_guard<std::mutex> lock(mutex_);
    calls_.push_back(c);
  }

  virtual void add(std::shared_ptr<IDelegate> call, const std::string& tag = std::string(), DelegateArgsMode args_mode = kDelegateArgsMode_Auto) override {
    if (!call) {
#if DELEGATES_TRACE
      std::cerr << "Null delegate provided to add()" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Null delegate provided to add()");
#endif //DELEGATES_STRICT
      return;
    }

    SharedDelegateType c;
    c.call_ = call;
    c.tag_ = tag;
    c.args_mode_ = args_mode;

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
    if (!call) {
#if DELEGATES_TRACE
      std::cerr << "Null delegate provided to remove()" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Null delegate provided to remove()");
#endif //DELEGATES_STRICT
      return;
    }

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
    if (!call) {
#if DELEGATES_TRACE
      std::cerr << "Null delegate provided to remove()" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Null delegate provided to remove()");
#endif //DELEGATES_STRICT
      return;
    }
    
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
  // check the arguments are correspond between signal and delegate when args_mode == kDelegateArgsMode_UseSignalArgs
  static bool check_delegate_arguments_correspond_to_signal(IDelegate* call, IDelegateArgs* args) {
    if (call->args()->size() == 0)
      return true;

    if (args->size() != call->args()->size())
      return false;

    for (size_t i = 0; i < args->size(); i++) {
      if (args->hash_code(i) != call->args()->hash_code(i))
        return false;
    }

    return true;
  }

  // Execute call: check types, pass args to call, execute call, move result
  bool perform_call(IDelegate* call, IDelegateArgs* args, DelegateArgsMode args_mode) {
    using result_noref = typename std::decay<TResult>::type;

    if (call->result()->hash_code() != typeid(TResult).hash_code() && call->result()->hash_code() != typeid(void).hash_code()) {
#if DELEGATES_TRACE
      std::cerr << "[DELEGATE ERROR] Cannot perform call for delegate because return type is incompatible";
      std::cerr << "  delegate result type: " << typeid(TResult).name() << ", hash code " << typeid(TResult).hash_code();
      std::cerr << ", call result hash code " << call->result()->hash_code() << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Cannot perform call for delegate because return type is incompatible");
#endif // DELEGATES_STRICT
      return false;
    }

    bool ret = false;

    if (args_mode == kDelegateArgsMode_UseDelegateOwnArgs) {
      ret = call->call();
    }
    else {
      IDelegateArgs* pargs = args ? args : &params_;
      bool use_args = check_delegate_arguments_correspond_to_signal(call, pargs);

      if (args_mode == kDelegateArgsMode_Auto) {
        if (!use_args)
          ret = call->call();
        else {
          args_mode = kDelegateArgsMode_UseSignalArgs;
        }
      }

      if (use_args && args_mode == kDelegateArgsMode_UseSignalArgs) {
        if (call->args()->size() == 0)
          ret = call->call();
        else
          ret = call->call(pargs);
      }
    }

    if (!ret) {
#if DELEGATES_TRACE
      std::cerr << "Call was not performed" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Call was not performed");
#endif //DELEGATES_STRICT
    }

    if (!ret || call->result()->hash_code() == typeid(void).hash_code())
      return ret;

    return MoveDelegateResult<TResult>{}(call->result(), result());
  }

  struct SharedDelegateType {
    std::shared_ptr<IDelegate> call_;
    std::string tag_;
    DelegateArgsMode args_mode_ = kDelegateArgsMode_Auto;
  };

  struct DelegateType {
    IDelegate* call_ = nullptr;
    std::function<void(IDelegate*)> deleter_ = [](IDelegate*){};
    std::string tag_;
    DelegateArgsMode args_mode_ = kDelegateArgsMode_Auto;
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
    if (!callee) {
#if DELEGATES_TRACE
      std::cerr << "Delegate call failed: Class pointer is null" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Delegate call failed: Class pointer is null");
#endif //DELEGATES_STRICT

      return false;
    }

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
    if (!callee) {
#if DELEGATES_TRACE
      std::cerr << "Delegate call failed: Class pointer is null" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Delegate call failed: Class pointer is null");
#endif //DELEGATES_STRICT

      return false;
    }

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
    if (!callee) {
#if DELEGATES_TRACE
      std::cerr << "Delegate call failed: Class pointer is null" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Delegate call failed: Class pointer is null");
#endif //DELEGATES_STRICT

      return false;
    }

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
    if (!callee) {
#if DELEGATES_TRACE
      std::cerr << "Delegate call failed: Class pointer is null" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Delegate call failed: Class pointer is null");
#endif //DELEGATES_STRICT
      return false;
    }

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
  TResult perform_function_call(typename std::tuple<TArgs&...>& tup, std::index_sequence<Is...>) {
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
  void perform_function_call(typename std::tuple<TArgs&...>& tup, std::index_sequence<Is...>) {
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
    if (!callee) {
#if DELEGATES_TRACE
      std::cerr << "WARNING: Delegate was not called: weak pointer is null" << std::endl;
#endif //DELEGATES_TRACE
      return false;
    }
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
    if (!callee) {
#if DELEGATES_TRACE
      std::cerr << "WARNING: Delegate was not called: weak pointer is null" << std::endl;
#endif //DELEGATES_TRACE

      return false;
    }
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
