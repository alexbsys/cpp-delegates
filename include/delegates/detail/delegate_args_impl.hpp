
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


namespace delegates {
namespace detail {

/// \brief    Delegate arguments implementation
template<std::size_t N, typename... Args>
class DelegateArgsImpl
  : public virtual delegates::IDelegateArgs {
 public:
  explicit DelegateArgsImpl(Args&&... args): args_(std::forward<Args>(args)...) { setup_deleters(); }

  DelegateArgsImpl(std::nullptr_t)
    : default_args_(std::tuple<typename std::decay<Args>::type...> {})  // default args used for empty initialization when some of arguments are references
    , args_(default_args_) {
    setup_deleters();
  }

  ~DelegateArgsImpl() override { clear(); }

  void clear() override {
    for(size_t i=0; i<deleters_.size(); i++)
      clear(i);
  }

  void clear(size_t idx) override {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    void* ptr = get_ptr(idx);
    if (ptr && deleters_[idx])
      deleters_[idx](ptr);

    tuple_runtime::runtime_tuple_set_value_ptr(args_, idx, nullptr, 0);
    deleters_[idx] = [](void* ptr) {};
  }

  bool set_ptr(size_t idx, void* pv, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) override  {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    clear(idx);
    if (tuple_runtime::runtime_tuple_set_value_ptr(args_, idx, pv, type_hash)) {
      deleters_[idx] = deleter_ptr;
      return true;
    }
    return false;
  }

  size_t size() const override {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    return std::tuple_size<tuple_type>::value;
  }

  size_t hash_code(size_t idx) const override {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    return tuple_runtime::runtime_tuple_get_element_type_hash(args_, idx);
  }

  void* get_ptr(size_t idx) const override  {
    using tuple_type=typename std::remove_reference<std::tuple<Args...> >::type;
    return tuple_runtime::runtime_tuple_get_value_ptr(const_cast<tuple_type&>(args_), idx);
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

template<>
class DelegateArgsImpl<0>
  : public virtual delegates::IDelegateArgs {
public:
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

template<typename ...TArgs>
struct DelegateArgs : public DelegateArgsImpl<sizeof...(TArgs), TArgs...> {
  explicit DelegateArgs(TArgs&&... args) : DelegateArgsImpl(std::forward<TArgs>(args)...) {}
  DelegateArgs(std::nullptr_t) : DelegateArgsImpl(std::nullptr_t{}) {}
  ~DelegateArgs() = default;
private:
  DelegateArgs(const DelegateArgs&) {}
  DelegateArgs& operator= (const DelegateArgs& other) { return *this; }
};

}//namespace detail
}//namespace delegates

#endif //DELEGATE_ARGS_IMPL_HEADER
