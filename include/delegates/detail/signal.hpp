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

#ifndef SIGNAL_HEADER
#define SIGNAL_HEADER

#include "../i_delegate.h"
#include "delegate_impl.hpp"
#include "factory.hpp"

#include <list>
#include <mutex>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {

template<typename TResult, typename ...TArgs>
struct Signal : public virtual ISignal {
  Signal(TArgs... args) : delegate_(delegates::factory::template make_unique_signal<TResult, TArgs...>(std::forward<TArgs>(args)...)) {}
  Signal(DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>()) 
    : delegate_(delegates::factory::template make_unique_signal<TResult, TArgs...>(std::move(params))) {}

  ~Signal() override {
    std::list<Signal*> ref_by_signals;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      ref_by_signals = ref_by_signals_;
    }

    for (const auto& ref_signal : ref_by_signals)
      ref_signal->remove(delegate_.get());
  }

  bool call() override {
    return delegate_->call();
  }

  bool call(IDelegateArgs* args) override {
    return delegate_->call(args);
  }

  IDelegateArgs* args() override {
    return delegate_->args();
  }

  IDelegateResult* result() override {
    return delegate_->result();
  }

  bool operator()() {
    return delegate_->call();
  }

  Signal& operator +=(std::shared_ptr<IDelegate> delegate) {
    add(delegate);
    return *this;
  }

  Signal& operator -=(std::shared_ptr<IDelegate> delegate) {
    remove(delegate);
    return *this;
  }

  Signal& operator +=(IDelegate* delegate) {
    add(delegate);
    return *this;
  }

  Signal& operator -=(IDelegate* delegate) {
    remove(delegate);
    return *this;
  }

  Signal& operator +=(Signal& signal) {
    ref_signals_.push_back(&signal);
    signal.ref_by_signals_.push_back(this);

    add(signal.delegate_.get(), std::string(), ISignal::kDelegateArgsMode_UseSignalArgs, [&signal, this](IDelegate* d) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        ref_signals_.remove(&signal);
      }
      {
        std::lock_guard<std::mutex> lock(signal.mutex_);
        signal.ref_by_signals_.remove(this);
      }
    });

    return *this;
  }

  Signal& operator -=(Signal& signal) {
    remove(signal.get_delegate());
    return *this;
  }

  void add(
    IDelegate* delegate, 
    const std::string& tag = std::string(), 
    DelegateArgsMode args_mode = kDelegateArgsMode_Auto,
    std::function<void(IDelegate*)> deleter = [](IDelegate*) {}) override {
    if (!delegate) {
#if DELEGATES_TRACE
      std::cerr << "Signal: cannot add delegate, null provided to add()" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Signal: cannot add delegate, null provided to add()");
#endif //DELEGATES_STRICT
      return;
    }

    delegate_->add(delegate, tag, args_mode, deleter);
  }

  void add(
    std::shared_ptr<IDelegate> delegate, 
    const std::string& tag = std::string(),
    DelegateArgsMode args_mode = kDelegateArgsMode_Auto) override {
    if (!delegate) {
#if DELEGATES_TRACE
      std::cerr << "Signal: cannot add delegate, null provided to add()" << std::endl;
#endif //DELEGATES_TRACE

#if DELEGATES_STRICT
      throw std::runtime_error("Signal: cannot add delegate, null provided to add()");
#endif //DELEGATES_STRICT
      return;
    }

    delegate_->add(delegate, tag, args_mode);
  }

  void get_all(std::vector<IDelegate*>& delegates) const override {
    delegate_->get_all(delegates);
  }

  void remove(const std::string& tag) override { delegate_->remove(tag); }
  void remove(IDelegate* delegate) override { delegate_->remove(delegate); }
  void remove(std::shared_ptr<IDelegate> delegate) override { delegate_->remove(delegate); }
  void remove_all() override { delegate_->remove_all(); }

  IDelegate* get_delegate() const { return delegate_.get(); }

private:
  std::unique_ptr<ISignal> delegate_;
  std::list<Signal*> ref_signals_;
  std::list<Signal*> ref_by_signals_;
  mutable std::mutex mutex_;
  
  Signal(const Signal&) {}
  Signal& operator=(const Signal&) { return *this; }
};

}//namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif //SIGNAL_HEADER
