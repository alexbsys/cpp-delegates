#ifndef DEFERRED_CALL_HEADER
#define DEFERRED_CALL_HEADER

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

namespace delegates {

namespace detail {

/// \brief    Delegate base implementation. Void or non-void return types are supported
template<typename TResult, typename... Args>
struct DelegateBase
  : public virtual IDelegate {
  DelegateBase(Args&&... args) : params_(std::forward<Args>(args)...) {}
  DelegateBase(std::nullptr_t) : params_(std::nullptr_t{}) {}
  ~DelegateBase() = default;

  bool call() override { return perform_call(result_, params_); }
  IDelegateResult* result() override { return static_cast<IDelegateResult*>(&result_); }
  IDelegateArgs* args() override { return static_cast<IDelegateArgs*>(&params_); }

protected:
  // perform_call must be implemented by nested classes
  virtual bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Args...>& args) = 0;
private:
  DelegateResult<TResult> result_;
  DelegateArgs<Args...> params_;
};


/// \brief    SignalBase implementation
template<typename TResult, typename... Args>
struct SignalBase : public virtual ISignal {
  SignalBase(Args&&... args) : params_(std::forward<Args>(args)...) {}
  SignalBase(std::nullptr_t) : params_(std::nullptr_t{}) {}
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
  DelegateArgs<Args...> params_;
  std::list<SharedDelegateType> shared_calls_;
  std::list<DelegateType> calls_;
  mutable std::mutex mutex_;
};

}//namespace detail

using namespace detail;

/// \brief    Class method deferred call template for all result types but void
template <class TClass, typename TResult, typename... Ts>
class MethodDelegate 
  : public detail::DelegateBase<TResult, Ts...> {
 private:
  typedef TResult (TClass::*MethodType)(Ts...);
  TClass* callee_;
  MethodType mem_fn_;

 public:
  MethodDelegate(TClass* callee, TResult (TClass::*method)(Ts...), Ts&&... args)
    :detail::DelegateBase<TResult, Ts...>(std::forward<Ts>(args)...)
     ,callee_(callee)
     ,mem_fn_(method) {}

  MethodDelegate(TClass* callee, TResult (TClass::*method)(Ts...), std::nullptr_t)
    :detail::DelegateBase<TResult, Ts...>(std::nullptr_t{})
      ,callee_(callee)
      ,mem_fn_(method) {}

  ~MethodDelegate() = default;

 private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args) override {
    return perform_call(result, args, std::make_index_sequence<sizeof...(Ts)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args, std::index_sequence<Is...>) {
    auto callee = callee_;
    if (!callee) return false;

    result.set((callee->*(mem_fn_))(std::get<Is>(args.get_tuple())...));
    return true;
  }
};


/// \brief    Class const method deferred call template for all result types but void
template <class TClass, typename TResult, typename... Ts>
class ConstMethodDelegate
  : public detail::DelegateBase<TResult, Ts...> {
 public:
  ConstMethodDelegate(const TClass* owner, TResult (TClass::*member)(Ts...) const, Ts&&... args)
     :detail::DelegateBase<TResult, Ts...>(std::forward<Ts>(args)...)
     ,callee_(owner)
     ,mem_fn_(member) {}

  ConstMethodDelegate(const TClass* owner, TResult (TClass::*member)(Ts...) const, std::nullptr_t)
     :detail::DelegateBase<TResult, Ts...>(std::nullptr_t{})
     ,callee_(owner)
     ,mem_fn_(member) {}

  ~ConstMethodDelegate() = default;

 private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args) override {
    return perform_call(result, args, std::make_index_sequence<sizeof...(Ts)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args, std::index_sequence<Is...>) {
    auto callee = callee_;
    if (!callee)
      return false;

    result.set((callee->*(mem_fn_))(std::get<Is>(args.get_tuple())...));
    return true;
  }

  typedef TResult (TClass::*MethodType)(Ts...) const;
  const TClass* callee_;
  MethodType mem_fn_;
};

/// \brief    Class method deferred call for 'void' return type
template <class TClass, typename... Ts>
class MethodDelegate<TClass, void, Ts...>
  : public detail::DelegateBase<void, Ts...> {
 public:
  MethodDelegate(TClass* owner, void (TClass::*member)(Ts...), Ts&&... args)
    :detail::DelegateBase<void, Ts...>(std::forward<Ts>(args)...)
    ,callee_(owner)
    ,mem_fn_(member) {}

  MethodDelegate(TClass* owner, void (TClass::*member)(Ts...), std::nullptr_t)
    :detail::DelegateBase<void, Ts...>(std::nullptr_t{})
    ,callee_(owner)
    ,mem_fn_(member) {}

  ~MethodDelegate() = default;

 private:
  bool perform_call(DelegateResult<void>&, DelegateArgs<Ts...>& args) override {
    return perform_call(args, std::make_index_sequence<sizeof...(Ts)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateArgs<Ts...>& args, std::index_sequence<Is...>) {
    auto callee = callee_;
    if (!callee) return false;

    (callee->*(mem_fn_))(std::get<Is>(args.get_tuple())...);
    return true;
  }

  typedef void (TClass::*MethodType)(Ts...);
  TClass* callee_;
  MethodType mem_fn_;
};

/// \brief    Functional delegate implementation for all result types but void
template <typename TResult, typename... Ts>
class FunctionalDelegate
  : public detail::DelegateBase<TResult, Ts...> {
public:
  explicit FunctionalDelegate(std::function<TResult(Ts...)> func, Ts&&... args)
    :detail::DelegateBase<TResult,Ts...>(std::forward<Ts>(args)...)
    ,func_(func) {}

  FunctionalDelegate(std::function<TResult(Ts...)> func, std::nullptr_t)
    :detail::DelegateBase<TResult,Ts...>(std::nullptr_t{})
    ,func_(func) {}

  ~FunctionalDelegate() override = default;
private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args) override {
    return perform_call(result, args, std::make_index_sequence<sizeof...(Ts)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args, std::index_sequence<Is...>) {
    result.set<TResult>(func_(std::get<Is>(args.get_tuple())...));
    return true;
  }

  std::function<TResult(Ts...)> func_;
};

/// \brief    Funnctional delegate implementation for void result type
template <typename... Ts>
class FunctionalDelegate<void, Ts...>
  : public detail::DelegateBase<void,Ts...> {
 public:
  explicit FunctionalDelegate(std::function<void(Ts...)> func, Ts&&... args)
    :detail::DelegateBase<void,Ts...>(std::forward<Ts>(args)...)
    ,func_(func) {}

  FunctionalDelegate(std::function<void(Ts...)> func, std::nullptr_t)
    :detail::DelegateBase<void,Ts...>(std::nullptr_t{})
    ,func_(func) {}

  ~FunctionalDelegate() = default;
private:
  bool perform_call(DelegateResult<void>& result, DelegateArgs<Ts...>& args) override {
    perform_call(args, std::make_index_sequence<sizeof...(Ts)>{});
    return true;
  }

  template<std::size_t... Is>
  void perform_call(DelegateArgs<Ts...>& args, std::index_sequence<Is...>) {
    func_(std::get<Is>(args.get_tuple())...);
  }

  std::function<void(Ts...)> func_;
};

/// \brief    Lambda delegate implementation for non-void result types
template <typename TResult, typename F, typename... Ts>
class LambdaDelegate
  : public detail::DelegateBase<TResult,Ts...> {
public:
  explicit LambdaDelegate(F && lambda, Ts&&... args)
   :func_(std::function<TResult(Ts...)>(std::move(lambda)))
   ,detail::DelegateBase<TResult,Ts...>(std::forward<Ts>(args)...) {
  }

  LambdaDelegate(F && lambda, std::nullptr_t)
   :func_(std::function<TResult(Ts...)>(std::move(lambda)))
   ,detail::DelegateBase<TResult,Ts...>(std::nullptr_t{}) {
  }

  ~LambdaDelegate() override = default;

private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args) override {
    result.set<TResult>(perform_function_call(args.get_tuple(), std::make_index_sequence<sizeof...(Ts)>{}));
    return true;
  }

  template <std::size_t... Is>
  TResult perform_function_call(typename std::tuple<Ts...>& tup, std::index_sequence<Is...>) {
    return func_(std::get<Is>(tup)...);
  }

  std::function<TResult(Ts...)> func_;
};

/// \brief    Lambda delegate implementation for void result type
template <typename F, typename... Ts>
class LambdaDelegate<void, F, Ts...>
  : public detail::DelegateBase<void, Ts...> {
public:
  explicit LambdaDelegate(F&& lambda, Ts&&... args)
    :func_(std::function<void(Ts...)>(std::move(lambda)))
    , detail::DelegateBase<void, Ts...>(std::forward<Ts>(args)...) {
  }

  LambdaDelegate(F&& lambda, std::nullptr_t)
    :func_(std::function<void(Ts...)>(std::move(lambda)))
    , detail::DelegateBase<void, Ts...>(std::nullptr_t{}) {
  }

  ~LambdaDelegate() override = default;

private:
  bool perform_call(DelegateResult<void>&, DelegateArgs<Ts...>& args) override {
    perform_function_call(args.get_tuple(), std::make_index_sequence<sizeof...(Ts)>{});
    return true;
  }

  template <std::size_t... Is>
  void perform_function_call(typename std::tuple<Ts...>& tup, std::index_sequence<Is...>) {
    func_(std::get<Is>(tup)...);
  }

  std::function<void(Ts...)> func_;
};

/// \brief    Delegate for class method call by shared pointer implementation. Both void and non-void result types are supported
template <class TClass, typename TResult, typename... Ts>
class SharedMethodDelegate
  : public MethodDelegate<TClass, TResult, Ts...> {
 public:
  SharedMethodDelegate(std::shared_ptr<TClass> callee,
                       TResult (TClass::*method)(Ts...),
                       Ts&&... args)
    :MethodDelegate<TClass, TResult, Ts...>(callee.get(), method, std::forward<Ts>(args)...)
    ,callee_ptr_(callee) {}

  SharedMethodDelegate(std::shared_ptr<TClass> callee,
                       TResult (TClass::*method)(Ts...), std::nullptr_t)
    :MethodDelegate<TClass, TResult, Ts...>(callee.get(), method, std::nullptr_t{})
    ,callee_ptr_(callee) {}

  ~SharedMethodDelegate() = default;
 private:
  std::shared_ptr<TClass> callee_ptr_;
};

/// \brief    Delegate for class method call by shared pointer implementation. Both void and non-void result types are supported
template <class TClass, typename TResult, typename... Ts>
class SharedConstMethodDelegate
  : public ConstMethodDelegate<TClass, TResult, Ts...> {
public:
  SharedConstMethodDelegate(std::shared_ptr<TClass> callee,
    TResult(TClass::* method)(Ts...) const,
    Ts&&... args)
    :ConstMethodDelegate<TClass, TResult, Ts...>(callee.get(), method, std::forward<Ts>(args)...)
    , callee_ptr_(callee) {}

  SharedConstMethodDelegate(std::shared_ptr<TClass> callee,
    TResult(TClass::* method)(Ts...) const, std::nullptr_t)
    :ConstMethodDelegate<TClass, TResult, Ts...>(callee.get(), method, std::nullptr_t{})
    , callee_ptr_(callee) {}

  ~SharedConstMethodDelegate() = default;
private:
  std::shared_ptr<TClass> callee_ptr_;
};


/// \brief    Delegate for class method call by shared pointer implementation. Non-void result types are supported
template <class TClass, typename TResult, typename... Ts>
class WeakMethodDelegate
  : public detail::DelegateBase<TResult,Ts...> {
 public:
  WeakMethodDelegate(std::weak_ptr<TClass> callee, TResult (TClass::*mem_fn)(Ts...), Ts&&... args)
    :detail::DelegateBase<TResult,Ts...>(std::forward<Ts>(args)...)
    ,callee_(callee)
    ,mem_fn_(mem_fn) {}

  WeakMethodDelegate(std::weak_ptr<TClass> callee, TResult (TClass::*mem_fn)(Ts...), std::nullptr_t)
    :detail::DelegateBase<TResult,Ts...>(std::nullptr_t{})
    ,callee_(callee)
    ,mem_fn_(mem_fn) {}

  ~WeakMethodDelegate() = default;
 private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args) override {
    return perform_call(result, args.get_tuple(), std::make_index_sequence<sizeof...(Ts)>{} /* call_helper::gen_seq<sizeof...(Ts)>{}*/ );
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, std::tuple<Ts...>& tup, std::index_sequence<Is...>) {
    auto callee = callee_.lock();
    if (!callee)
      return false;
    result.set((callee.get()->*(mem_fn_))(std::get<Is>(tup)...));
    return true;
  }

  typedef TResult (TClass::*MethodType)(Ts...);
  std::weak_ptr<TClass> callee_;
  MethodType mem_fn_;
};

/// \brief    Delegate for class method call by shared pointer implementation. For void result type
template <class TClass, typename... Ts>
class WeakMethodDelegate<TClass,void,Ts...>
  : public detail::DelegateBase<void,Ts...> {
 public:
  explicit WeakMethodDelegate(std::weak_ptr<TClass> callee, void(TClass::*method)(Ts...), Ts&&... args)
    :detail::DelegateBase<void,Ts...>(std::forward<Ts>(args)...)
    ,callee_(callee)
    ,mem_fn_(method) {}

  WeakMethodDelegate(std::weak_ptr<TClass> callee, void(TClass::*method)(Ts...), std::nullptr_t)
    :detail::DelegateBase<void,Ts...>(std::nullptr_t{})
    ,callee_(callee)
    ,mem_fn_(method) {}

  ~WeakMethodDelegate() = default;
 private:
  template <std::size_t... Is>
  bool perform_call(std::tuple<Ts...>& tup, std::index_sequence<Is...>) {
    auto callee = callee_.lock();
    if (!callee)
      return false;
    (callee.get()->*(mem_fn_))(std::get<Is>(tup)...);
    return true;
  }

  bool perform_call(DelegateResult<void>& result, DelegateArgs<Ts...>& args) override {
    return perform_call(args.get_tuple(), std::make_index_sequence<sizeof...(Ts)>{} /* call_helper::gen_seq<sizeof...(Ts)>{}*/ );
  }

  typedef void (TClass::*MethodType)(Ts...);
  std::weak_ptr<TClass> callee_;
  MethodType mem_fn_;
};

}//namespace delegates

#endif  // DEFERRED_CALL_HEADER
