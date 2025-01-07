
#ifndef DELEGATE_ARGS_IMPL_HEADER
#define DELEGATE_ARGS_IMPL_HEADER

#include "../i_delegate.h"

#include <functional>
#include <tuple>
#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <cstddef>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {
namespace detail {

/// \brief    Delegate arguments implementation. N is arguments count, TArgs - arguments types list
template<std::size_t N, typename... TArgs>
class DelegateArgsImpl
  : public virtual delegates::IDelegateArgs {
  // disable copying
  DelegateArgsImpl(const DelegateArgsImpl&) {}
  DelegateArgsImpl& operator=(const DelegateArgsImpl&) {}
  
public:
  DelegateArgsImpl(DelegateArgsImpl&& params) noexcept
    : default_args_(std::move(params.default_args_))
    , args_(std::move(params.args_))
    , deleters_(std::move(params.deleters_))  {}

  DelegateArgsImpl(TArgs&&... args): args_(std::forward<TArgs>(args)...) { setup_deleters(); }

  // Constructor with std::nullptr_t{} parameter means that arguments are initialized with default values
  DelegateArgsImpl(std::nullptr_t) noexcept
    : default_args_(std::tuple<typename std::decay<TArgs>::type...> {})  // default args used for empty initialization when some of arguments are references
    , args_(default_args_) {
    setup_deleters();
  }

  ~DelegateArgsImpl() override { clear(); }

  void clear() override {
    for(size_t i=0; i<deleters_.size(); i++)
      clear(i);
  }

  void clear(size_t idx) override {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs...> >::type;
    void* ptr = get_ptr(idx);
    if (ptr && deleters_[idx])
      deleters_[idx](ptr);

    tuple_runtime::runtime_tuple_set_value_ptr(args_, default_args_, idx, nullptr, 0);
    deleters_[idx] = [](void* ptr) {};
  }

  bool set_ptr(size_t idx, void* pv, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) override  {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs...> >::type;
    clear(idx);
    if (tuple_runtime::runtime_tuple_set_value_ptr(args_, default_args_, idx, pv, type_hash)) {
      deleters_[idx] = deleter_ptr;
      return true;
    }
    return false;
  }

  size_t size() const override {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs...> >::type;
    return std::tuple_size<tuple_type>::value;
  }

  size_t hash_code(size_t idx) const override {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs...> >::type;
    return tuple_runtime::runtime_tuple_get_element_type_hash(args_, idx);
  }

  void* get_ptr(size_t idx) const override  {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs...> >::type;
    return tuple_runtime::runtime_tuple_get_value_ptr(const_cast<tuple_type&>(args_), idx);
  }

  std::tuple<TArgs...>& get_tuple() { return args_; }
  const std::tuple<TArgs...>& get_tuple() const { return args_; }

 private:
  void setup_deleters() {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs...> >::type;
    size_t constexpr args_count = std::tuple_size<tuple_type>::value;
    deleters_.resize(args_count);
  }

  // contains non-reference non-const default arguments tuple which is used for initialization when Empty{} arguments are provided
  std::tuple<typename std::decay<TArgs>::type...> default_args_;
  std::tuple<TArgs...> args_;
  std::vector<std::function<void(void*)> > deleters_;
};

/// \brief    Delegates arguments specialization for empty list
template<>
class DelegateArgsImpl<0>
  : public virtual delegates::IDelegateArgs {
  DelegateArgsImpl(const DelegateArgsImpl&) {}
  DelegateArgsImpl& operator=(const DelegateArgsImpl&) { return *this;  }

public:
  DelegateArgsImpl(DelegateArgsImpl&& params) noexcept {}
  DelegateArgsImpl() {}
  DelegateArgsImpl(std::nullptr_t) {}
  ~DelegateArgsImpl() override = default;

  void clear() override {}
  void clear(size_t idx) override {}
  bool set_ptr(size_t idx, void* pv, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) override {
    return false;
  }

  size_t size() const override { return 0; }
  size_t hash_code(size_t idx) const override { return 0; }
  void* get_ptr(size_t idx) const override { return nullptr; }

  std::tuple<>& get_tuple() { return args_; }
  const std::tuple<>& get_tuple() const { return args_; }
  
private:
  std::tuple<> args_;
};
}//namespace detail

template<typename ...TArgs>
struct DelegateArgs 
  : public detail::DelegateArgsImpl<sizeof...(TArgs), TArgs...> {
  DelegateArgs(DelegateArgs&& other) noexcept : detail::DelegateArgsImpl<sizeof...(TArgs), TArgs...>(std::move(other)) {}
  DelegateArgs(TArgs&&... args) : detail::DelegateArgsImpl<sizeof...(TArgs), TArgs...>(std::forward<TArgs>(args)...) {}
  DelegateArgs(std::nullptr_t) : detail::DelegateArgsImpl<sizeof...(TArgs), TArgs...>(std::nullptr_t{}) {}
  ~DelegateArgs() = default;
private:
  DelegateArgs(const DelegateArgs&) {}
  DelegateArgs& operator= (const DelegateArgs& other) { return *this; }
};

// Create delegate arguments with values without rvalue refs
template<typename ...TArgs>
constexpr DelegateArgs<TArgs...> DelegateArgsValues(TArgs... args) {
  return DelegateArgs<TArgs...>(std::forward<TArgs>(args)...);
}

}//namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif //DELEGATE_ARGS_IMPL_HEADER
