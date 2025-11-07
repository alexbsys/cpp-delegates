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

#ifndef DELEGATES_ISERIALIZER_HEADER
#define DELEGATES_ISERIALIZER_HEADER

#include "../delegates_conf.h"
#include "../i_delegate.h"
#include <vector>
#include <cstdint>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {
namespace serialization {

/// \brief    Interface for serializing delegate arguments and results
///           Used for IPC/RPC scenarios
struct ISerializer {
    virtual ~ISerializer() = default;
    
    /// \brief    Serialize argument by index
    /// \param    idx - argument index
    /// \param    args - delegate arguments interface
    /// \param    output - output buffer
    /// \return   true if successful, false if type cannot be serialized
    virtual bool serialize_arg(size_t idx, const IDelegateArgs* args, 
                                std::vector<uint8_t>& output) = 0;
    
    /// \brief    Deserialize argument by index
    /// \param    idx - argument index
    /// \param    args - delegate arguments interface
    /// \param    input - input buffer
    /// \param    offset - current offset in buffer (updated after deserialization)
    /// \return   true if successful
    virtual bool deserialize_arg(size_t idx, IDelegateArgs* args,
                                 const std::vector<uint8_t>& input, size_t& offset) = 0;
    
    /// \brief    Serialize result
    /// \param    result - delegate result interface
    /// \param    output - output buffer
    /// \return   true if successful
    virtual bool serialize_result(const IDelegateResult* result,
                                  std::vector<uint8_t>& output) = 0;
    
    /// \brief    Deserialize result
    /// \param    result - delegate result interface
    /// \param    input - input buffer
    /// \param    offset - current offset in buffer (updated after deserialization)
    /// \return   true if successful
    virtual bool deserialize_result(IDelegateResult* result,
                                     const std::vector<uint8_t>& input, size_t& offset) = 0;
    
    /// \brief    Check if type can be serialized
    /// \param    type_hash - RTTI type hash code
    /// \return   true if type can be serialized
    virtual bool can_serialize(size_t type_hash) const = 0;
    
    /// \brief    Get serialized size of argument (for pre-allocation)
    /// \param    idx - argument index
    /// \param    args - delegate arguments interface
    /// \return   estimated size in bytes, 0 if unknown or cannot serialize
    virtual size_t get_serialized_size(size_t idx, const IDelegateArgs* args) const = 0;
    
    /// \brief    Register custom type serializer (for user-defined types)
    ///           Override this method in derived classes to support custom types
    /// \param    type_hash - RTTI type hash code
    /// \param    serialize_func - function to serialize the type
    /// \param    deserialize_func - function to deserialize the type
    /// \return   true if registration successful
    /// \note     Default implementation does nothing, returns false
    ///           Derived classes should override this to support custom registration
    virtual bool register_custom_type(size_t type_hash,
                                      std::function<bool(const void*, std::vector<uint8_t>&)> serialize_func,
                                      std::function<bool(const std::vector<uint8_t>&, size_t&, void*)> deserialize_func) {
        (void)type_hash;
        (void)serialize_func;
        (void)deserialize_func;
        return false;
    }
};

/// \brief    Utility class for serializing/deserializing entire delegates
class DelegateSerializer {
public:
    explicit DelegateSerializer(ISerializer* serializer) 
        : serializer_(serializer) {
        if (!serializer_) {
            throw std::invalid_argument("DelegateSerializer: serializer cannot be null");
        }
    }
    
    /// \brief    Serialize delegate arguments
    /// \param    delegate - delegate to serialize
    /// \param    output - output buffer
    /// \return   true if successful
    bool serialize_args(const IDelegate* delegate, std::vector<uint8_t>& output);
    
    /// \brief    Deserialize delegate arguments
    /// \param    delegate - delegate to deserialize into
    /// \param    input - input buffer
    /// \param    offset - starting offset (updated after deserialization)
    /// \return   true if successful
    bool deserialize_args(IDelegate* delegate,
                         const std::vector<uint8_t>& input, size_t& offset);
    
    /// \brief    Serialize delegate result
    /// \param    delegate - delegate with result
    /// \param    output - output buffer
    /// \return   true if successful
    bool serialize_result(const IDelegate* delegate, std::vector<uint8_t>& output);
    
    /// \brief    Deserialize delegate result
    /// \param    delegate - delegate to deserialize result into
    /// \param    input - input buffer
    /// \param    offset - starting offset (updated after deserialization)
    /// \return   true if successful
    bool deserialize_result(IDelegate* delegate,
                           const std::vector<uint8_t>& input, size_t& offset);
    
    /// \brief    Serialize entire delegate (args + metadata)
    /// \param    delegate - delegate to serialize
    /// \param    output - output buffer
    /// \return   true if successful
    bool serialize(const IDelegate* delegate, std::vector<uint8_t>& output);
    
private:
    void serialize_metadata(const IDelegate* delegate, std::vector<uint8_t>& output);
    bool deserialize_metadata(const std::vector<uint8_t>& input, size_t& offset,
                               size_t& args_count, std::vector<size_t>& type_hashes);
    
    ISerializer* serializer_;
};

} // namespace serialization
} // namespace delegates

DELEGATES_BASE_NAMESPACE_END

// Include implementation
#include "serializer_impl.hpp"

#endif // DELEGATES_ISERIALIZER_HEADER

