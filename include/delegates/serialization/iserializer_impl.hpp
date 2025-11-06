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

#ifndef DELEGATES_ISERIALIZER_IMPL_HEADER
#define DELEGATES_ISERIALIZER_IMPL_HEADER

#include "iserializer.h"
#include <stdexcept>
#include <cstring>

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {
namespace serialization {

inline bool DelegateSerializer::serialize_args(const IDelegate* delegate, 
                                               std::vector<uint8_t>& output) {
    if (!delegate) return false;
    
    // args() is not const, so we need to remove const
    IDelegateArgs* args = const_cast<IDelegate*>(delegate)->args();
    if (!args) return false;
    
    size_t count = args->size();
    for (size_t i = 0; i < count; ++i) {
        if (!serializer_->serialize_arg(i, args, output)) {
            return false;  // Type not serializable
        }
    }
    
    return true;
}

inline bool DelegateSerializer::deserialize_args(IDelegate* delegate,
                                                 const std::vector<uint8_t>& input, 
                                                 size_t& offset) {
    if (!delegate) return false;
    
    IDelegateArgs* args = delegate->args();
    if (!args) return false;
    
    size_t count = args->size();
    for (size_t i = 0; i < count; ++i) {
        if (!serializer_->deserialize_arg(i, args, input, offset)) {
            return false;
        }
    }
    
    return true;
}

inline bool DelegateSerializer::serialize_result(const IDelegate* delegate, 
                                                  std::vector<uint8_t>& output) {
    if (!delegate) return false;
    
    // result() is not const, so we need to remove const
    IDelegateResult* result = const_cast<IDelegate*>(delegate)->result();
    if (!result) return false;
    
    return serializer_->serialize_result(result, output);
}

inline bool DelegateSerializer::deserialize_result(IDelegate* delegate,
                                                   const std::vector<uint8_t>& input, 
                                                   size_t& offset) {
    if (!delegate) return false;
    
    IDelegateResult* result = delegate->result();
    if (!result) return false;
    
    return serializer_->deserialize_result(result, input, offset);
}

inline bool DelegateSerializer::serialize(const IDelegate* delegate, 
                                          std::vector<uint8_t>& output) {
    if (!delegate) return false;
    
    // Serialize metadata first
    serialize_metadata(delegate, output);
    
    // Serialize arguments
    if (!serialize_args(delegate, output)) {
        return false;
    }
    
    return true;
}

inline void DelegateSerializer::serialize_metadata(const IDelegate* delegate, 
                                                    std::vector<uint8_t>& output) {
    // args() is not const, so we need to remove const
    IDelegateArgs* args = const_cast<IDelegate*>(delegate)->args();
    size_t count = args->size();
    
    // Write argument count
    const uint8_t* count_bytes = reinterpret_cast<const uint8_t*>(&count);
    output.insert(output.end(), count_bytes, count_bytes + sizeof(count));
    
    // Write type hashes
    for (size_t i = 0; i < count; ++i) {
        size_t hash = args->hash_code(i);
        const uint8_t* hash_bytes = reinterpret_cast<const uint8_t*>(&hash);
        output.insert(output.end(), hash_bytes, hash_bytes + sizeof(hash));
    }
}

inline bool DelegateSerializer::deserialize_metadata(const std::vector<uint8_t>& input, 
                                                      size_t& offset,
                                                      size_t& args_count, 
                                                      std::vector<size_t>& type_hashes) {
    if (offset + sizeof(size_t) > input.size()) {
        return false;
    }
    
    // Read argument count
    std::memcpy(&args_count, input.data() + offset, sizeof(size_t));
    offset += sizeof(size_t);
    
    // Read type hashes
    type_hashes.resize(args_count);
    size_t hashes_size = args_count * sizeof(size_t);
    if (offset + hashes_size > input.size()) {
        return false;
    }
    
    std::memcpy(type_hashes.data(), input.data() + offset, hashes_size);
    offset += hashes_size;
    
    return true;
}

} // namespace serialization
} // namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif // DELEGATES_ISERIALIZER_IMPL_HEADER

