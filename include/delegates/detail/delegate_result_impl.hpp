
#ifndef DELEGATE_RESULT_IMPL_HEADER
#define DELEGATE_RESULT_IMPL_HEADER

#include "../i_delegate.h"

#include <functional>
#include <tuple>
#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <cstddef>


namespace delegates {
namespace detail {

/// \brief    Result for all copyable types but void
template <typename TValue>
class DelegateResult 
  : public virtual delegates::IDelegateResult {
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

}//namespace detail
}//namespace delegates

#endif //DELEGATE_RESULT_IMPL_HEADER
