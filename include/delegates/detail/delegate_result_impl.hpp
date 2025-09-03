
#ifndef DELEGATE_RESULT_IMPL_HEADER
#define DELEGATE_RESULT_IMPL_HEADER

#include "../i_delegate.h"

#include <functional>
#include <tuple>
#include <memory>
#include <vector>
#include <limits>
#include <list>
#include <mutex>
#include <cstddef>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {
namespace detail {

/// \brief    Result for all copyable types but void
template <typename TValue>
class DelegateResult 
  : public virtual IDelegateResult {
 public:
  DelegateResult()
      : default_value_(), value_(default_value_), has_value_(false) {
    static_assert(std::is_same<TValue,void>::value || std::is_copy_constructible<TValue>::value, "Result type must be copyable");
  }

  virtual ~DelegateResult() override { clear(); }

  size_t hash_code() const override {
    return typeid(TValue).hash_code();
  }

  bool set_ptr(const void* value_ptr, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) override {
    using value_noconst = typename std::decay<TValue>::type;

    clear();
    if (!value_ptr) // ptr==null means just clear stored value
      return true;

    if (hash_code() != type_hash) {
#if DELEGATES_TRACE
      std::cerr << "Delegate result was not set: value type hash code is not the same as result type" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Delegate result was not set: value type hash code is not the same as result type");
#endif //DELEGATES_STRICT

      return false;
    }

    const value_noconst* pv = reinterpret_cast<const value_noconst*>(value_ptr);
    value_ = *pv;
    deleter_ptr_ = deleter_ptr;
    has_value_ = true;
    return true;
  }

  bool detach_ptr(void* value_ptr, size_t value_size, std::function<void(void*)>& deleter_ptr) override {
    using value_noconst = typename std::decay<TValue>::type;

    if (!has_value_) {
#if DELEGATES_TRACE
      std::cerr << "WARNING Delegate result was not detached: has no value" << std::endl;
#endif //DELEGATES_TRACE
      return false;
    }

    if (value_size != std::numeric_limits<size_t>::max() && value_size != sizeof(value_)) {
#if DELEGATES_TRACE
      std::cerr << "Delegate result was not detached: value size is not the same" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Delegate result was not detached: value size is not the same");
#endif //DELEGATES_STRICT

      return false;
    }

    value_noconst* v = reinterpret_cast<value_noconst*>(value_ptr);
    *v = value_;
    deleter_ptr = deleter_ptr_;
    value_ = default_value_;
    has_value_ = false;
    deleter_ptr_ = [](void*) {};
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
  // copying is prohibited because DelegateResult owns stored value (deleter may be called for it)
  DelegateResult(const DelegateResult&) {}
  DelegateResult& operator= (const DelegateResult& other) { return *this; }
   
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

  bool set_ptr(const void* value_ptr, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void*) {}) override { 
    (void)value_ptr;
    (void)type_hash;
    (void)deleter_ptr;
#if DELEGATES_TRACE
    std::cerr << "Delegate result set() called for void result" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
    throw std::runtime_error("Delegate result set() called for void result");
#endif //DELEGATES_STRICT

    return false; 
  }
  bool detach_ptr(void* value_ptr, size_t value_size, std::function<void(void*)>& deleter_ptr) override { 
    (void)value_ptr;
    (void)value_size;
    (void)deleter_ptr;
#if DELEGATES_TRACE
    std::cerr << "Delegate result detach() called for void result" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
    throw std::runtime_error("Delegate result detach() called for void result");
#endif //DELEGATES_STRICT

    return false; 
  }
  bool has_value() const override { return false; }
  void* get_ptr() override { return nullptr; }
  const void* get_ptr() const override { return nullptr; }
  int size_bytes() const override { return 0; }
  size_t hash_code() const override { return typeid(void).hash_code(); }
  void clear() override {}
};

/// \brief    Move delegate result for non-void types
template<typename TResult>
struct MoveDelegateResult {
  bool operator()(IDelegateResult* from, IDelegateResult* to) {
    using result_noref = typename std::decay<TResult>::type;
    result_noref ret_val;
    std::function<void(void*)> deleter;
    if (from->detach_ptr(&ret_val, sizeof(ret_val), deleter)) {
      return to->set_ptr(&ret_val, typeid(TResult).hash_code(), deleter);
    }

#if DELEGATES_TRACE
    std::cerr << "Move delegate result failed" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
    throw std::runtime_error("Move delegate result failed");
#endif //DELEGATES_STRICT

    return false;
  }
};

/// \brief    Move delegate result for void types
template<>
struct MoveDelegateResult<void> {
  bool operator()(IDelegateResult* from, IDelegateResult* to) { (void)from; (void)to; return true; }
};

}//namespace detail
}//namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif //DELEGATE_RESULT_IMPL_HEADER
