
#ifndef SIGNAL_HEADER
#define SIGNAL_HEADER

#include "delegate_impl.hpp"
#include "factory.hpp"
#include "../i_delegate.h"

#include <list>
#include <mutex>

namespace delegates {

template<typename TResult, typename ...TArgs>
struct Signal : public virtual ISignal {
  Signal(TArgs... args) : delegate_(delegates::factory::template make_unique_signal<TResult, TArgs...>(std::forward<TArgs>(args)...)) {}
  Signal(DelegateArgs<TArgs...>&& params = DelegateArgs<TArgs...>(std::nullptr_t{})) 
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

    add(signal.delegate_.get(), std::string(), [&signal, this](IDelegate* d) { 
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

  void add(IDelegate* delegate, const std::string& tag = std::string(), std::function<void(IDelegate*)> deleter = [](IDelegate*) {}) override {
    delegate_->add(delegate, tag, deleter);
  }

  void add(std::shared_ptr<IDelegate> delegate, const std::string& tag = std::string()) override {
    delegate_->add(delegate, tag);
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

#endif //SIGNAL_HEADER
