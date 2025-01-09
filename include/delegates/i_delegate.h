#ifndef DELEGATES_CPP_DELEGATE_INTERFACE_HEADER
#define DELEGATES_CPP_DELEGATE_INTERFACE_HEADER

#include "delegates_conf.h"

#include <cassert>
#include <cstdlib>
#include <string>
#include <memory>
#include <functional>
#include <stdexcept>
#include <vector>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {

/// \brief    Result of call accessor interface
struct IDelegateResult {
  virtual ~IDelegateResult() = default;

  /// \brief    check is result has been set
  virtual bool has_value() const = 0;

  /// \brief    clear result value
  virtual void clear() = 0;

  /// \brief    query non-const pointer to result variable
  virtual void* get_ptr() = 0;

  /// \brief    query const pointer to result variable
  virtual const void* get_ptr() const = 0;

  /// \brief    get size of result varialbe in bytes
  virtual int size_bytes() const = 0;

  /// \brief    get RTTI hash code of result type
  virtual size_t hash_code() const = 0;

  /// \brief    set result value by void* pointer
  virtual bool set_ptr(const void* value_ptr, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) = 0;

  /// \brief    detach stored value (copy to new buffer without call of deleter)
  virtual bool detach_ptr(void* value_ptr, size_t value_buffer_size, std::function<void(void*)>& deleter_ptr) = 0;

  /// \brief   get copy of result value
  /// \return  copy of argument value
  template<typename TValue>
  TValue get() const {
    using value_noref = typename std::decay<TValue>::type;

    if (typeid(TValue).hash_code() != hash_code())
      throw std::runtime_error("Value type error");

    return *(reinterpret_cast<const value_noref*>(get_ptr()));
  }

  /// \brief   get copy of argument value by argument index
  /// \param   out_value - buffer for store argument value copy
  /// \return  true - OK, false - wrong type provided
  template<typename TValue>
  bool try_get(TValue& out_value) const {
    using value_noref = typename std::decay<TValue>::type;

    if (typeid(TValue).hash_code() != hash_code())
      return false;

    out_value = *(reinterpret_cast<const value_noref*>(get_ptr()));
    return true;
  }

  template<typename TResult>
  const TResult& get_or_default(const TResult& def = TResult()) const {
    auto ptr = get_ptr();
    return ptr ? *(reinterpret_cast<const TResult*>(ptr)) : def;
  }

  /// \brief    Store result value copy by const ref
  /// \param    value - reference to result value
  /// \param    deleter - deleter function which called when result value is released by delegate
  template<typename TValue>
  bool set(const TValue& value, std::function<void(TValue&)> deleter = [](TValue&) {}) {
    using value_noref = typename std::decay<TValue>::type;

    std::function<void(void*)> deleter_ptr = [deleter](void* ptr) {
      value_noref* pv = reinterpret_cast<value_noref*>(ptr);
      if (pv) deleter(*pv);
    };
    return set_ptr(reinterpret_cast<const void*>(&value), typeid(TValue).hash_code(), deleter_ptr);
  }
};

/// \brief    Delegate arguments accessor interface
struct IDelegateArgs {
  virtual ~IDelegateArgs() = default;

  /// \brief    Get arguments count
  /// \return   Arguments count
  virtual size_t size() const = 0;

  /// \brief    Store argument from void* pointer
  /// \param    idx - argument index
  /// \param    pv - pointer to argument value buffer
  /// \param    type_hash - RTTI hash code from argument type
  /// \param    deleter_ptr - deleter function which called when argument value is released by delegate
  /// \return   true - OK, false - type error
  virtual bool set_ptr(size_t idx, void* pv, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) = 0;

  /// \brief    Get void* pointer to stored agrument value by index
  /// \param    idx - argument index
  /// \return   Raw pointer to argument value
  virtual void* get_ptr(size_t idx) const = 0;

  /// \brief    Get argument RTTI type hash code by argument index
  /// \param    idx - argument index
  /// \return   RTTI type hash code
  virtual size_t hash_code(size_t idx) const = 0;

  /// \brief    Clear argument by index
  /// \param    idx - argument index
  virtual void clear(size_t idx) = 0;

  /// \brief    Clear all arguments
  virtual void clear() = 0;

  /// \brief   get copy of argument value by argument index
  /// \param   idx - argument index
  /// \return  copy of argument value
  template<typename T>
  T get(size_t idx) {
    using value_type = typename std::decay<T>::type;
    static value_type default_val;

    if (typeid(T).hash_code() != hash_code(idx))
      throw std::runtime_error("Wrong type provided");

    void* p = get_ptr(idx);
    return p ? *reinterpret_cast<value_type*>(p) : default_val;
  }

  template<typename T>
  T& get_ref(size_t idx) {
    using value_type = typename std::decay<T>::type;
    static value_type default_val;

    if (typeid(T).hash_code() != hash_code(idx))
      throw std::runtime_error("Wrong type provided");

    void* p = get_ptr(idx);
    return p ? *reinterpret_cast<value_type*>(p) : default_val;
  }

  /// \brief   get copy of argument value by argument index
  /// \param   idx - argument index
  /// \param   out_value - buffer for store argument value copy
  /// \return  true - OK, false - wrong type provided
  template<typename T>
  bool try_get(size_t idx, T& out_value) {
    if (typeid(T).hash_code() != hash_code(idx))
      return false;

    void* p = get_ptr(idx);
    if (p == nullptr)
      return false;

    out_value = *reinterpret_cast<T*>(p);
    return true;
  }

  /// \brief    Store argument value copy
  /// \param    idx - argument index
  /// \param    v - reference to argument value
  /// \param    deleter - deleter function which called when argument value is released by delegate
  template<typename T>
  bool set(size_t idx, T& v, std::function<void(T&)> deleter = [](T&) {}) {
    using value_type = typename std::decay<T>::type;
    std::function<void(void*)> deleter_ptr = [deleter](void* ptr) { value_type* pv = reinterpret_cast<value_type*>(ptr); if (pv && deleter) deleter(*pv); };
    return set_ptr(idx, (void*)(&v), typeid(T).hash_code(), deleter_ptr);
  }

  /// \brief    Store argument value copy by const ref
  /// \param    idx - argument index
  /// \param    v - reference to argument value
  /// \param    deleter - deleter function which called when argument value is released by delegate
  template<typename T>
  bool set(size_t idx, const T& v, std::function<void(T&)> deleter = [](const T&) {}) {
    using value_type = typename std::decay<T>::type;
    std::function<void(void*)> deleter_ptr = [deleter](void* ptr) { value_type* pv = reinterpret_cast<value_type*>(ptr); if (pv && deleter) deleter(*pv); };
    return set_ptr(idx, (void*)(&v), typeid(T).hash_code(), deleter_ptr);
  }
};

/// \brief    Delegate interface
struct IDelegate {
  virtual ~IDelegate() = default;

  /// \brief    perform delegate call with stored parameters and save result into buffer
  virtual bool call() = 0;

  virtual bool call(IDelegateArgs* args) = 0;

  /// \brief    get delegate arguments accessor
  virtual IDelegateArgs* args() = 0;

  /// \brief    get delegate result accessor
  virtual IDelegateResult* result() = 0;
};

/// \brief    multi delegate interface: provides call list with many delegates with same arguments and return values can be called
struct ISignal
  : public virtual IDelegate {
  enum DelegateArgsMode {
    // Pass signals arguments only to delegate. Delegate arguments must be the same as in signal, or empty. 
    // If delegate has arguments, but they have different types, delegate will be added to signal, but call may produce error
    kDelegateArgsMode_UseSignalArgs = 0,

    // Signal will not pass any own arguments, but delegate will be called with their own arguments which were specified by used
    // into each delegate immediately
    kDelegateArgsMode_UseDelegateOwnArgs = 1,
    
    // Signal analyzes delegate arguments types. If signal and delegate arguments are the same, signal arguments will be
    // passed. Otherwise delegate will be called with own arguments
    kDelegateArgsMode_Auto = 2
  };

  virtual ~ISignal() = default;

  /// \brief    add delegate to call list. If delegate was added more than one time, it will be called many times
  /// \param    delegate - pointer to delegate
  /// \param    tag - string tag. Tag is not unique, more than one delegates may be added with same tags
  /// \param    deleter - function which called when delegate removed
  virtual void add(
    IDelegate* delegate, 
    const std::string& tag = std::string(), 
    DelegateArgsMode args_mode = kDelegateArgsMode_Auto,
    std::function<void(IDelegate*)> deleter = [](IDelegate*){}) = 0;

  /// \brief    add delegate to call list. If delegate was added more than one time, it will be called many times
  /// \param    delegate - pointer to delegate
  /// \param    tag - string tag. Tag is not unique, more than one delegates may be added with same tags
  virtual void add(
    std::shared_ptr<IDelegate> delegate, 
    const std::string& tag = std::string(),
    DelegateArgsMode args_mode = kDelegateArgsMode_Auto) = 0;

  /// \brief    remove delegate from call list by tag. If more than one delegates were added with single tag, all of them will be removed
  /// \param    tag - string tag
  virtual void remove(const std::string& tag) = 0;

  /// \brief    remove delegate from call list by raw pointer. Only delegates which added by raw pointer will be removed
  /// \param    delegate - pointer to delegate
  virtual void remove(IDelegate* delegate) = 0;

  /// \brief    remove delegate from call list by shared pointer. Only delegates which added by shared pointer will be removed
  /// \param    delegate - pointer to delegate
  virtual void remove(std::shared_ptr<IDelegate> delegate) = 0;

  /// \brief    remove all delegates from call list
  virtual void remove_all() = 0;

  /// \brief    get all connected delegates
  virtual void get_all(std::vector<IDelegate*>& delegates) const = 0;
};

}//namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif  // DELEGATES_CPP_DELEGATE_INTERFACE_HEADER
