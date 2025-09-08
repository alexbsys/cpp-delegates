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
  : public delegates::IDelegateArgs {
  // disable copying
  DelegateArgsImpl(const DelegateArgsImpl&) {}
  DelegateArgsImpl& operator=(const DelegateArgsImpl&) { return *this;  }
  
public:
  DelegateArgsImpl(DelegateArgsImpl&& params) noexcept
    : values_args_(std::move(params.values_args_))
    , def_args_(std::tuple<typename std::decay<TArgs>::type...> {})
    , ref_args_(tuple_runtime::ref_tuple(values_args_))
    , deleters_(std::move(params.deleters_))  {}

  explicit DelegateArgsImpl(TArgs&&... args)
    : values_args_(std::forward<TArgs>(args)...)
    , def_args_(std::tuple<typename std::decay<TArgs>::type...> {})
    , ref_args_(tuple_runtime::ref_tuple(values_args_)) { 
    setup_deleters(); 
  }

  // Constructor with std::nullptr_t{} parameter means that arguments are initialized with default values
  DelegateArgsImpl(std::nullptr_t) noexcept
    : values_args_(std::tuple<typename std::decay<TArgs>::type...> {})  // default args used for empty initialization when some of arguments are references
    , def_args_(std::tuple<typename std::decay<TArgs>::type...> {})
    , ref_args_(tuple_runtime::ref_tuple(values_args_)) {
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

    tuple_runtime::runtime_tuple_set_value_ptr(values_args_, def_args_, idx, nullptr, 0);
    deleters_[idx] = [](void*) {};
  }

  bool set_ptr(size_t idx, void* pv, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void* ptr) {}) override  {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs...> >::type;
    clear(idx);
    if (tuple_runtime::runtime_tuple_set_value_ptr(values_args_, def_args_, idx, pv, type_hash)) {
      deleters_[idx] = deleter_ptr;
      return true;
    }

#if DELEGATES_TRACE
    std::cerr << "Delegate argument was not set, idx=" << idx << ", type hash " << type_hash << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
    throw std::runtime_error("Delegate argument was not set");
#endif //DELEGATES_STRICT

    return false;
  }

  size_t size() const override {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs&...> >::type;
    return std::tuple_size<tuple_type>::value;
  }

  size_t hash_code(size_t idx) const override {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs&...> >::type;
    return tuple_runtime::runtime_tuple_get_element_type_hash(ref_args_, idx);
  }

  void* get_ptr(size_t idx) const override  {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs&...> >::type;
    return tuple_runtime::runtime_tuple_get_value_ptr(const_cast<tuple_type&>(ref_args_), idx);
  }

  std::tuple<TArgs&...>& get_tuple() { return ref_args_; }
  const std::tuple<TArgs&...>& get_tuple() const { return ref_args_; }

 private:
  void setup_deleters() {
    using tuple_type=typename std::remove_reference<std::tuple<TArgs...> >::type;
    size_t constexpr args_count = std::tuple_size<tuple_type>::value;
    deleters_.resize(args_count);
  }

  // contains non-reference non-const default arguments tuple which is used for initialization when Empty{} arguments are provided
  std::tuple<typename std::decay<TArgs>::type...> def_args_;
  std::tuple<typename std::decay<TArgs>::type...> values_args_;
  std::tuple<TArgs&...> ref_args_;
  std::vector<std::function<void(void*)> > deleters_;
};

/// \brief    Delegates arguments specialization for empty list
template<>
class DelegateArgsImpl<0>
  : public delegates::IDelegateArgs {
  DelegateArgsImpl(const DelegateArgsImpl&) {}
  DelegateArgsImpl& operator=(const DelegateArgsImpl&) { return *this;  }

public:
  DelegateArgsImpl(DelegateArgsImpl&&) noexcept {}
  DelegateArgsImpl() {}
  DelegateArgsImpl(std::nullptr_t) {}
  ~DelegateArgsImpl() override = default;

  void clear() override {}
  void clear(size_t idx) override { (void)idx; }
  bool set_ptr(size_t idx, void* pv, size_t type_hash, std::function<void(void*)> deleter_ptr = [](void*) {}) override {
    (void)idx; (void)pv; (void)type_hash; (void)deleter_ptr;
#if DELEGATES_TRACE
    std::cerr << "DelegateArgs: called set() for void argument" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
    throw std::runtime_error("DelegateArgs: called set() for void argument");
#endif //DELEGATES_STRICT

    return false;
  }

  size_t size() const override { return 0; }

  size_t hash_code(size_t idx) const override { 
    (void)idx;
#if DELEGATES_TRACE
    std::cerr << "DelegateArgs: called hash_code() for empty argument" << std::endl;
#endif //DELEGATES_TRACE
    return 0;
  }
  void* get_ptr(size_t idx) const override { 
    (void)idx;
#if DELEGATES_TRACE
    std::cerr << "DelegateArgs: called get() for void argument" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
    throw std::runtime_error("DelegateArgs: called get() for void argument");
#endif //DELEGATES_STRICT

    return nullptr; 
  }

  std::tuple<>& get_tuple() { return ref_args_; }
  const std::tuple<>& get_tuple() const { return ref_args_; }
  
private:
  std::tuple<> ref_args_;
};
}//namespace detail

template<typename ...TArgs>
struct DelegateArgs 
  : public detail::DelegateArgsImpl<sizeof...(TArgs), TArgs...> {

  template<bool TCheck=true, typename = typename std::enable_if<sizeof...(TArgs) && TCheck>::type>
  explicit DelegateArgs(TArgs&&... args) : detail::DelegateArgsImpl<sizeof...(TArgs), TArgs...>(std::forward<TArgs>(args)...) {}

  DelegateArgs(DelegateArgs&& other) noexcept : detail::DelegateArgsImpl<sizeof...(TArgs), TArgs...>(std::move(other)) {}
  DelegateArgs(std::nullptr_t = std::nullptr_t{}) : detail::DelegateArgsImpl<sizeof...(TArgs), TArgs...>(std::nullptr_t{}) {}
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
