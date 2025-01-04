#ifndef DEFERRED_CALL_HEADER
#define DEFERRED_CALL_HEADER

#include <functional>
#include <tuple>
#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <cstddef>

#include "i_delegate.h"
#include "call_helper.hpp"

namespace delegates {

/// \brief    Result for all copyable types but void
template <typename TValue>
class DelegateResult : public virtual IDelegateResult {
 public:
  DelegateResult()
      : default_value_(), value_(default_value_), has_value_(false) {
    static_assert(call_helper::is_same_v<TValue,void> || call_helper::is_copy_constructible_v<TValue>, "Result type must be copyable");
  }

  DelegateResult(const TValue& value)
      : value_(value), has_value_(true) {
    static_assert(call_helper::is_same_v<TValue,void> || call_helper::is_copy_constructible_v<TValue>, "Result type must be copyable");
  }

  virtual ~DelegateResult() override { clear(); }

  size_t hash_code() const override {
    return typeid(TValue).hash_code();
  }

  bool set_ptr(const void* value_ptr, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) override {
    using value_noconst = typename std::decay<TValue>::type;

    clear();
    if (!value_ptr) // ptr==null means just clear
      return true;

    if (hash_code() != type_hash)
      return false;

    const value_noconst* pv = reinterpret_cast<const value_noconst*>(value_ptr);
    value_ = *pv;
    deleter_ptr_ = deleter_ptr;
    has_value_ = true;
    return true;
  }

  bool detach_ptr(void* value_ptr, size_t value_size, std::function<void(void*)>& deleter_ptr) override {
    using value_noconst = typename std::decay<TValue>::type;

    if (!has_value_)
      return false;

    if (value_size != sizeof(value_))
      return false;

    value_noconst* v = reinterpret_cast<value_noconst*>(value_ptr);
    *v = value_;
    deleter_ptr = deleter_ptr_;
    value_ = default_value_;
    has_value_ = false;
    deleter_ptr_ = [](void* ptr) {};
    return true;
  }

  void clear() override {
    if (has_value_) {
      if (deleter_ptr_)
        deleter_ptr_(reinterpret_cast<void*>(&value_));

      value_ = default_value_;
      has_value_ = false;
    }
  }

  bool has_value() const override { return has_value_; }
  void* get_ptr() override {
    return has_value_ ? reinterpret_cast<void*>(&value_) : nullptr;
  }
  const void* get_ptr() const override { return has_value_ ? &value_ : nullptr; }
  int size_bytes() const override { return sizeof(value_); }

 private:
  typename std::decay<TValue>::type default_value_;
  typename std::decay<TValue>::type value_;
  bool has_value_;
  std::function<void(void*)> deleter_ptr_ = [](void*) {};
};

/// \brief    Delegate result for 'void' type
template <>
class DelegateResult<void>
  : public virtual IDelegateResult {
 public:
  ~DelegateResult() override = default;

  bool set_ptr(const void* value_ptr, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) override { return false; }
  bool detach_ptr(void* value_ptr, size_t value_size, std::function<void(void*)>& deleter_ptr) override { return false; }
  bool has_value() const override { return false; }
  void* get_ptr() override { return nullptr; }
  const void* get_ptr() const override { return nullptr; }
  int size_bytes() const override { return 0; }
  size_t hash_code() const override { return typeid(void).hash_code(); }
  void clear() override {}
};


/// \brief    Delegate arguments implementation
template<typename... Args>
class DelegateArgs : public virtual IDelegateArgs {
 public:
  DelegateArgs(Args&&... args): args_(std::forward<Args>(args)...) { setup_deleters(); }

  DelegateArgs(std::nullptr_t)
    : default_args_(std::tuple<typename std::decay<Args>::type...> {})  // default args used for empty initialization when some of arguments are references
    , args_(default_args_) {
    setup_deleters();
  }

  ~DelegateArgs() override { clear(); }

  void clear() override {
    for(size_t i=0; i<deleters_.size(); i++)
      clear(i);
  }

  void clear(size_t idx) override {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    if constexpr (0 < std::tuple_size<tuple_type>::value) {
      void* ptr = get_ptr(idx);
      if (ptr && deleters_[idx])
        deleters_[idx](ptr);

      tuple_runtime::runtime_tuple_set_value_ptr(args_, idx, nullptr, 0);
      deleters_[idx] = [](void* ptr) {};
    }
  }

  bool set_ptr(size_t idx, void* pv, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) override  {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    if constexpr (std::tuple_size<tuple_type>::value > 0) {
      clear(idx);
      bool r = tuple_runtime::runtime_tuple_set_value_ptr(args_, idx, pv, type_hash);
      if (r) {
        deleters_[idx] = deleter_ptr;
        return true;
      }
    }
    return false;
  }

  size_t size() const override {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    return std::tuple_size<tuple_type>::value;
  }

  size_t hash_code(size_t idx) const override {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    if constexpr (std::tuple_size<tuple_type>::value > 0) {
      return tuple_runtime::runtime_tuple_get_element_type_hash(args_, idx);
    } else {
      return 0;
    }
  }

  void* get_ptr(size_t idx) const override  {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    if constexpr (std::tuple_size<tuple_type>::value > 0) {
      return tuple_runtime::runtime_tuple_get_value_ptr(const_cast<tuple_type&>(args_), idx);
    } else {
      return nullptr;
    }
  }

  std::tuple<Args...>& get_tuple() { return args_; }
  const std::tuple<Args...>& get_tuple() const { return args_; }

 private:
  void setup_deleters() {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    size_t constexpr args_count = std::tuple_size<tuple_type>::value;
    deleters_.resize(args_count);
  }

  // contains non-reference non-const default arguments tuple which is used for initialization when Empty{} arguments are provided
  std::tuple<typename std::decay<Args>::type...> default_args_;
  std::tuple<Args...> args_;
  std::vector<std::function<void(void*)> > deleters_;
};

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


/// \brief    Multidelegate implementation
template<typename TResult, typename... Args>
struct Multidelegate : public virtual IMultidelegate {
  Multidelegate(Args&&... args) : params_(std::forward<Args>(args)...) {}
  Multidelegate(std::nullptr_t) : params_(std::nullptr_t{}) {}
  ~Multidelegate() override { remove_all(); }

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
  // Copy args without deleters. They are accessible only during source arguments list is alive. Must be released before source
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

    bool ret = call->call();
    call->args()->clear();

    if (!ret)
      return false;

    if constexpr (!call_helper::is_same_v<void, TResult>) {
      result_noref ret_val;
      std::function<void(void*)> deleter;
      if (call->result()->detach_ptr(&ret_val, sizeof(ret_val), deleter)) {
        result()->set_ptr(&ret_val, typeid(TResult).hash_code(), deleter);
      }
    }

    return true;
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

/// Class method deferred call template for all result types but void
template <class TClass, typename TResult, typename... Ts>
class MethodDelegate : public detail::DelegateBase<TResult, Ts...> {
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
    return perform_call(result, args, call_helper::gen_seq<sizeof...(Ts)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args, call_helper::index<Is...>) {
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
    return perform_call(result, args, call_helper::gen_seq<sizeof...(Ts)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args, call_helper::index<Is...>) {
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
    return perform_call(args, call_helper::gen_seq<sizeof...(Ts)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateArgs<Ts...>& args, call_helper::index<Is...>) {
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
  FunctionalDelegate(std::function<TResult(Ts...)> func, Ts&&... args)
    :detail::DelegateBase<TResult,Ts...>(std::forward<Ts>(args)...)
    ,func_(func) {}

  FunctionalDelegate(std::function<TResult(Ts...)> func, std::nullptr_t)
    :detail::DelegateBase<TResult,Ts...>(std::nullptr_t{})
    ,func_(func) {}

  ~FunctionalDelegate() override = default;
private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args) override {
    return perform_call(result, args, call_helper::gen_seq<sizeof...(Ts)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args, call_helper::index<Is...>) {
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
  FunctionalDelegate(std::function<void(Ts...)> func, Ts&&... args)
    :detail::DelegateBase<void,Ts...>(std::forward<Ts>(args)...)
    ,func_(func) {}

  FunctionalDelegate(std::function<void(Ts...)> func, std::nullptr_t)
    :detail::DelegateBase<void,Ts...>(std::nullptr_t{})
    ,func_(func) {}

  ~FunctionalDelegate() = default;
private:
  bool perform_call(DelegateResult<void>& result, DelegateArgs<Ts...>& args) override {
    perform_call(args, call_helper::gen_seq<sizeof...(Ts)>{});
    return true;
  }

  template<std::size_t... Is>
  void perform_call(DelegateArgs<Ts...>& args, call_helper::index<Is...>) {
    func_(std::get<Is>(args.get_tuple())...);
  }

  std::function<void(Ts...)> func_;
};

/// \brief    Lambda delegate implementation for all result types (void and non-void)
template <typename TResult, typename F, typename... Ts>
class LambdaDelegate
  : public detail::DelegateBase<TResult,Ts...> {
public:
  LambdaDelegate(F && lambda, Ts&&... args)
   :fn_(new std::function<TResult(Ts...)>(std::move(lambda)))
   ,detail::DelegateBase<TResult,Ts...>(std::forward<Ts>(args)...) {
  }

  LambdaDelegate(F && lambda, std::nullptr_t)
   :fn_(new std::function<TResult(Ts...)>(std::move(lambda)))
   ,detail::DelegateBase<TResult,Ts...>(std::nullptr_t{}) {
  }

  virtual ~LambdaDelegate() override {
    if (fn_) {
      delete fn_;
      fn_ = nullptr;
    }
  }

private:
  bool perform_call(DelegateResult<TResult>& result, DelegateArgs<Ts...>& args) override {
    if constexpr (!call_helper::is_same_v<void, TResult>) {
      result.set<TResult>(call_helper::DoFunctionCall(*fn_, args.get_tuple(), call_helper::gen_seq<sizeof...(Ts)>{}));
    } else {
      call_helper::DoFunctionCall(*fn_, args.get_tuple(), call_helper::gen_seq<sizeof...(Ts)>{});
    }
    return true;
  }

  std::function<TResult(Ts...)>* fn_;
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
  bool perform_call(DelegateResult<TResult>& result, std::tuple<Ts...>& tup) override {
    return perform_call(result, tup, call_helper::gen_seq<sizeof...(Ts)>{});
  }

  template <std::size_t... Is>
  bool perform_call(DelegateResult<TResult>& result, std::tuple<Ts...>& tup, call_helper::index<Is...>) {
    auto callee = callee_.lock();
    if (!callee)
      return false;
    result.set((callee->*(mem_fn_))(std::get<Is>(tup)...));
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
  WeakMethodDelegate(std::weak_ptr<TClass> callee, void(TClass::*method)(Ts...), Ts&&... args)
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
  bool perform_call(std::tuple<Ts...>& tup, call_helper::index<Is...>) {
    auto callee = callee_.lock();
    if (!callee)
      return false;
    (callee->*(mem_fn_))(std::get<Is>(tup)...);
    return true;
  }

  bool perform_call(DelegateResult<void>& result, std::tuple<Ts...>& tup) override {
    return perform_call(tup, call_helper::gen_seq<sizeof...(Ts)>{});
  }

  typedef void (TClass::*MethodType)(Ts...);
  std::weak_ptr<TClass> callee_;
  MethodType mem_fn_;
};


///////////////////

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

// class pointer is weak_ptr
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
static IMultidelegate* make_multidelegate(Ts&&... args) {
  return new detail::Multidelegate<TResult, Ts...>(std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts>
static IMultidelegate* make_multidelegate(std::nullptr_t) {
  return new detail::Multidelegate<TResult, Ts...>(std::nullptr_t{});
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
static std::shared_ptr<IMultidelegate> make_shared_multidelegate(Ts&&... args) {
  return std::shared_ptr<IMultidelegate>(make_multidelegate<TResult,Ts...>(std::forward<Ts>(args)...));
}

template <typename TResult=void, typename... Ts>
static std::shared_ptr<IMultidelegate> make_shared_multidelegate(std::nullptr_t) {
  return std::shared_ptr<IMultidelegate>(make_multidelegate<TResult,Ts...>(std::nullptr_t{}));
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
static std::unique_ptr<IMultidelegate> make_unique_multidelegate(Ts&&... args) {
  return std::make_unique<detail::Multidelegate<TResult,Ts...> >(std::forward<Ts>(args)...);
}

template <typename TResult=void, typename... Ts>
static std::unique_ptr<IMultidelegate> make_unique_multidelegate(std::nullptr_t) {
  return std::make_unique<detail::Multidelegate<TResult,Ts...> >(std::nullptr_t{});
}

}// namespace factory
}//namespace delegates

#endif  // DEFERRED_CALL_HEADER
