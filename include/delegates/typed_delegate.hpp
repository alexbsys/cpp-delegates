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

#ifndef DELEGATES_TYPED_DELEGATE_HEADER
#define DELEGATES_TYPED_DELEGATE_HEADER

#include "delegates_conf.h"
#include "i_delegate.h"
#include <memory>
#include <tuple>
#include <utility>
#include <stdexcept>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {

/// \brief    Typed wrapper for IDelegate providing convenient direct call API
///           while preserving access to untyped interface for executors
template<typename Result, typename... Args>
class TypedDelegate {
public:
    /// \brief    Construct from raw pointer
    TypedDelegate(IDelegate* delegate, bool owns = false)
        : delegate_(delegate), owns_(owns) {
        if (!delegate_) {
            throw std::invalid_argument("TypedDelegate: delegate cannot be null");
        }
    }
    
    /// \brief    Construct from unique_ptr (takes ownership)
    TypedDelegate(std::unique_ptr<IDelegate> delegate)
        : delegate_(delegate.release()), owns_(true) {
        if (!delegate_) {
            throw std::invalid_argument("TypedDelegate: delegate cannot be null");
        }
    }
    
    /// \brief    Construct from shared_ptr (shares ownership)
    TypedDelegate(std::shared_ptr<IDelegate> delegate)
        : delegate_(delegate.get()), shared_(delegate) {
        if (!delegate_) {
            throw std::invalid_argument("TypedDelegate: delegate cannot be null");
        }
    }
    
    /// \brief    Move constructor
    TypedDelegate(TypedDelegate&& other) noexcept
        : delegate_(other.delegate_)
        , owns_(other.owns_)
        , shared_(std::move(other.shared_)) {
        other.delegate_ = nullptr;
        other.owns_ = false;
    }
    
    /// \brief    Move assignment
    TypedDelegate& operator=(TypedDelegate&& other) noexcept {
        if (this != &other) {
            cleanup();
            delegate_ = other.delegate_;
            owns_ = other.owns_;
            shared_ = std::move(other.shared_);
            other.delegate_ = nullptr;
            other.owns_ = false;
        }
        return *this;
    }
    
    /// \brief    Destructor
    ~TypedDelegate() {
        cleanup();
    }
    
    // Delete copy constructor and assignment
    TypedDelegate(const TypedDelegate&) = delete;
    TypedDelegate& operator=(const TypedDelegate&) = delete;
    
    /// \brief    Direct call with arguments (convenient API)
    Result operator()(Args... args) {
        set_args(std::forward<Args>(args)...);
        bool success = delegate_->call();
        if (!success) {
            throw std::runtime_error("TypedDelegate: call failed");
        }
        return get_result();
    }
    
    /// \brief    Get untyped interface (for executor)
    IDelegate* get_interface() const { return delegate_; }
    
    /// \brief    Get arguments interface
    IDelegateArgs* args() const { return delegate_->args(); }
    
    /// \brief    Get result interface
    IDelegateResult* result() const { return delegate_->result(); }
    
    /// \brief    Set argument by index (for automation)
    template<typename T>
    bool set_arg(size_t idx, const T& value) {
        return delegate_->args()->set<T>(idx, value);
    }
    
    /// \brief    Get argument by index
    template<typename T>
    T get_arg(size_t idx) {
        return delegate_->args()->get<T>(idx);
    }
    
    /// \brief    Call through interface (for executor)
    bool call() { return delegate_->call(); }
    
    /// \brief    Call with external arguments
    bool call(IDelegateArgs* args) { return delegate_->call(args); }
    
    /// \brief    Get result
    template<typename T = Result>
    typename std::enable_if<!std::is_void<T>::value, T>::type get_result() {
        return delegate_->result()->get<T>();
    }
    
    /// \brief    Get result (void specialization)
    template<typename T = Result>
    typename std::enable_if<std::is_void<T>::value, void>::type get_result() {
        if (delegate_->result()->has_value()) {
            // For void, just check that call was successful
        }
    }
    
    /// \brief    Check if result has value
    bool has_result() const {
        return delegate_->result()->has_value();
    }
    
private:
    void cleanup() {
        if (owns_ && delegate_) {
            delete delegate_;
            delegate_ = nullptr;
            owns_ = false;
        }
    }
    
    /// \brief    Set arguments through variadic template
    void set_args(Args... args) {
        set_args_impl(std::make_index_sequence<sizeof...(Args)>{}, 
                     std::forward<Args>(args)...);
    }
    
    template<size_t... Is>
    void set_args_impl(std::index_sequence<Is...>, Args... args) {
        // C++14 compatible: use array initialization trick instead of fold expression
        // Store arguments in tuple for indexed access
        auto args_tuple = std::make_tuple(args...);
        using array_type = int[];
        (void)array_type{0, (set_arg_by_index<Is>(std::get<Is>(args_tuple)), 0)...};
    }
    
    template<size_t I>
    void set_arg_by_index(typename std::tuple_element<I, std::tuple<Args...>>::type arg) {
        using arg_type = typename std::decay<typename std::tuple_element<I, std::tuple<Args...>>::type>::type;
        delegate_->args()->set<arg_type>(I, arg);
    }
    
    IDelegate* delegate_ = nullptr;
    bool owns_ = false;
    std::shared_ptr<IDelegate> shared_;  // For shared_ptr ownership
};

} // namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif // DELEGATES_TYPED_DELEGATE_HEADER

