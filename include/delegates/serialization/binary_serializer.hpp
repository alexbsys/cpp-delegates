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

#ifndef DELEGATES_BINARY_SERIALIZER_HEADER
#define DELEGATES_BINARY_SERIALIZER_HEADER

#ifdef DELEGATES_WITH_BINARY_SERIALIZATION

#include "i_serializer.h"
#include <unordered_map>
#include <functional>
#include <cstring>
#include <type_traits>
#include <vector>
#include <list>
#include <string>

// msgpack-c C API
#include <msgpack.h>

// Suppress MSVC warnings about type conversions in template code
// These are safe because we check types before conversion
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)  // conversion from 'type1' to 'type2', possible loss of data
#endif

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {
namespace serialization {

/// \brief    Binary serializer implementation using msgpack-c (C API, no boost dependency)
class BinarySerializer : public ISerializer {
public:
    BinarySerializer() {
        register_basic_types();
    }
    
    /// \brief    Register custom type serializer (for user-defined types)
    ///           Allows users to extend serialization without modifying library code
    /// \param    type_hash - RTTI type hash code
    /// \param    serialize_func - function to serialize the type to binary
    /// \param    deserialize_func - function to deserialize the type from binary
    /// \return   true if registration successful
    /// \example
    ///   struct MyStruct { int x; std::string y; };
    ///   BinarySerializer serializer;
    ///   serializer.register_custom_type(
    ///       typeid(MyStruct).hash_code(),
    ///       [](const void* ptr, std::vector<uint8_t>& output) {
    ///           const MyStruct& s = *static_cast<const MyStruct*>(ptr);
    ///           // Serialize s.x and s.y to output...
    ///           return true;
    ///       },
    ///       [](const std::vector<uint8_t>& input, size_t& offset, void* ptr) {
    ///           MyStruct& s = *static_cast<MyStruct*>(ptr);
    ///           // Deserialize from input at offset...
    ///           return true;
    ///       }
    ///   );
    bool register_custom_type(size_t type_hash,
                              std::function<bool(const void*, std::vector<uint8_t>&)> serialize_func,
                              std::function<bool(const std::vector<uint8_t>&, size_t&, void*)> deserialize_func) override {
        custom_serializers_[type_hash] = serialize_func;
        custom_deserializers_[type_hash] = deserialize_func;
        return true;
    }
    
    /// \brief    Template helper to register custom type with automatic msgpack handling
    /// \example
    ///   struct MyStruct { int x; std::string y; };
    ///   BinarySerializer serializer;
    ///   serializer.register_custom_type<MyStruct>(
    ///       [](const MyStruct& s, std::vector<uint8_t>& output) {
    ///           // Serialize using msgpack or custom format
    ///           return true;
    ///       },
    ///       [](const std::vector<uint8_t>& input, size_t& offset, MyStruct& s) {
    ///           // Deserialize from input at offset
    ///           return true;
    ///       }
    ///   );
    template<typename T>
    void register_custom_type(
        std::function<bool(const T&, std::vector<uint8_t>&)> serialize_func,
        std::function<bool(const std::vector<uint8_t>&, size_t&, T&)> deserialize_func) {
        size_t hash = typeid(T).hash_code();
        serializers_[hash] = [serialize_func](const void* ptr, std::vector<uint8_t>& output) {
            const T& value = *static_cast<const T*>(ptr);
            return serialize_func(value, output);
        };
        deserializers_[hash] = [deserialize_func](const std::vector<uint8_t>& input, size_t& offset, void* ptr) {
            T& value = *static_cast<T*>(ptr);
            return deserialize_func(input, offset, value);
        };
    }
    
    bool serialize_arg(size_t idx, const IDelegateArgs* args, 
                      std::vector<uint8_t>& output) override {
        if (!args || idx >= args->size()) return false;
        
        size_t type_hash = args->hash_code(idx);
        
        // Check if it's a custom type
        auto custom_it = custom_serializers_.find(type_hash);
        if (custom_it != custom_serializers_.end()) {
            void* ptr = const_cast<IDelegateArgs*>(args)->get_ptr(idx);
            if (!ptr) return false;
            return custom_it->second(ptr, output);
        }
        
        if (!can_serialize(type_hash)) return false;
        
        void* ptr = const_cast<IDelegateArgs*>(args)->get_ptr(idx);
        if (!ptr) return false;
        
        return serialize_value(type_hash, ptr, output);
    }
    
    bool deserialize_arg(size_t idx, IDelegateArgs* args,
                        const std::vector<uint8_t>& input, size_t& offset) override {
        if (!args || idx >= args->size()) return false;
        
        size_t type_hash = args->hash_code(idx);
        
        // Check if it's a custom type
        auto custom_it = custom_deserializers_.find(type_hash);
        if (custom_it != custom_deserializers_.end()) {
            void* ptr = args->get_ptr(idx);
            if (!ptr) return false;
            return custom_it->second(input, offset, ptr);
        }
        
        if (!can_serialize(type_hash)) return false;
        
        // Deserialize to temporary value and set using set() method
        return deserialize_value_to_arg(type_hash, input, offset, args, idx);
    }
    
    bool serialize_result(const IDelegateResult* result,
                         std::vector<uint8_t>& output) override {
        if (!result || !result->has_value()) return false;
        
        size_t type_hash = result->hash_code();
        
        // Check if it's a custom type
        auto custom_it = custom_serializers_.find(type_hash);
        if (custom_it != custom_serializers_.end()) {
            void* ptr = const_cast<IDelegateResult*>(result)->get_ptr();
            if (!ptr) return false;
            return custom_it->second(ptr, output);
        }
        
        if (!can_serialize(type_hash)) return false;
        
        void* ptr = const_cast<IDelegateResult*>(result)->get_ptr();
        if (!ptr) return false;
        
        return serialize_value(type_hash, ptr, output);
    }
    
    bool deserialize_result(IDelegateResult* result,
                           const std::vector<uint8_t>& input, size_t& offset) override {
        if (!result) return false;
        
        size_t type_hash = result->hash_code();
        if (!can_serialize(type_hash)) return false;
        
        // Deserialize to temporary value and set using set() method
        return deserialize_value_to_result(type_hash, input, offset, result);
    }
    
    bool can_serialize(size_t type_hash) const override {
        return serializers_.find(type_hash) != serializers_.end() ||
               custom_serializers_.find(type_hash) != custom_serializers_.end();
    }
    
    size_t get_serialized_size(size_t idx, const IDelegateArgs* args) const override {
        if (!args || idx >= args->size()) return 0;
        
        size_t type_hash = args->hash_code(idx);
        auto it = size_calculators_.find(type_hash);
        if (it != size_calculators_.end()) {
            return it->second(args, idx);
        }
        return 0;  // Unknown size
    }
    
    /// \brief    Register custom type serializer
    template<typename T>
    void register_type() {
        size_t hash = typeid(T).hash_code();
        serializers_[hash] = [](const void* ptr, std::vector<uint8_t>& output) {
            const T& value = *static_cast<const T*>(ptr);
            return serialize_impl(value, output);
        };
        deserializers_[hash] = [](const std::vector<uint8_t>& input, size_t& offset, void* ptr) {
            T& value = *static_cast<T*>(ptr);
            return deserialize_impl(input, offset, value);
        };
        size_calculators_[hash] = [](const IDelegateArgs* args, size_t idx) -> size_t {
            // Estimate size - for simple types it's known
            // C++14 compatible: use std::is_arithmetic instead of is_arithmetic_v
            if (std::is_arithmetic<T>::value) {
                return sizeof(T) + 1;  // +1 for msgpack header
            } else {
                return 0;  // Unknown for complex types
            }
        };
    }
    
private:
    /// \brief    Register container type (vector/list) for element type T
    template<typename T>
    void register_container_type() {
        // Register std::vector<T>
        register_vector_impl<T>();
        // Register std::list<T>
        register_list_impl<T>();
    }
    
    /// \brief    Register container implementation for std::vector
    template<typename T>
    void register_vector_impl() {
        size_t hash = typeid(std::vector<T>).hash_code();
        serializers_[hash] = [](const void* ptr, std::vector<uint8_t>& output) {
            const std::vector<T>& container = *static_cast<const std::vector<T>*>(ptr);
            return serialize_vector_impl(container, output);
        };
        deserializers_[hash] = [](const std::vector<uint8_t>& input, size_t& offset, void* ptr) {
            std::vector<T>& container = *static_cast<std::vector<T>*>(ptr);
            return deserialize_vector_impl(input, offset, container);
        };
        size_calculators_[hash] = [](const IDelegateArgs* args, size_t idx) -> size_t {
            // Estimate: size_t for count + elements
            return sizeof(size_t) + 1;  // +1 for msgpack header, actual size depends on elements
        };
    }
    
    /// \brief    Register container implementation for std::list
    template<typename T>
    void register_list_impl() {
        size_t hash = typeid(std::list<T>).hash_code();
        serializers_[hash] = [](const void* ptr, std::vector<uint8_t>& output) {
            const std::list<T>& container = *static_cast<const std::list<T>*>(ptr);
            return serialize_list_impl(container, output);
        };
        deserializers_[hash] = [](const std::vector<uint8_t>& input, size_t& offset, void* ptr) {
            std::list<T>& container = *static_cast<std::list<T>*>(ptr);
            return deserialize_list_impl(input, offset, container);
        };
        size_calculators_[hash] = [](const IDelegateArgs* args, size_t idx) -> size_t {
            // Estimate: size_t for count + elements
            return sizeof(size_t) + 1;  // +1 for msgpack header, actual size depends on elements
        };
    }
    
    void register_basic_types() {
        // Integer types
        register_type<char>();
        // Note: unsigned char and uint8_t are the same type, so we only register uint8_t
        register_type<uint8_t>();
        register_type<short>();
        register_type<unsigned short>();
        register_type<int>();
        register_type<long>();
        register_type<long long>();
        register_type<unsigned int>();
        register_type<unsigned long>();
        register_type<unsigned long long>();
        // Floating point types
        register_type<float>();
        register_type<double>();
        // bool is handled specially (serialized as int)
        // String types
        register_type<std::string>();
        register_type<std::wstring>();
        
        // Register containers for basic types
        register_container_type<char>();
        // Note: unsigned char and uint8_t are the same type, so we only register uint8_t
        register_container_type<uint8_t>();
        register_container_type<short>();
        register_container_type<unsigned short>();
        register_container_type<int>();
        register_container_type<long>();
        register_container_type<long long>();
        register_container_type<unsigned int>();
        register_container_type<unsigned long>();
        register_container_type<unsigned long long>();
        register_container_type<float>();
        register_container_type<double>();
        register_container_type<bool>();
        register_container_type<std::string>();
        register_container_type<std::wstring>();
    }
    
    // Serialization implementations for basic types
    static bool serialize_impl(int value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_int(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(long value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_long(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(long long value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_long_long(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(unsigned int value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_unsigned_int(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(unsigned long value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_unsigned_long(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(unsigned long long value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_unsigned_long_long(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(float value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_float(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(double value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_double(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(bool value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_int(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(char value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_int8(&pk, static_cast<int8_t>(value));
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(uint8_t value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_uint8(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(short value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_short(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(unsigned short value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_unsigned_short(&pk, value);
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(const std::string& value, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        msgpack_pack_str(&pk, value.size());
        msgpack_pack_str_body(&pk, value.data(), value.size());
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    static bool serialize_impl(const std::wstring& value, std::vector<uint8_t>& output) {
        // Serialize wstring as binary data (array of wchar_t bytes)
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        
        // Pack as binary data
        size_t byte_size = value.size() * sizeof(wchar_t);
        msgpack_pack_bin(&pk, byte_size);
        msgpack_pack_bin_body(&pk, value.data(), byte_size);
        
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    // Deserialization implementations
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, char& value) {
        if (offset >= input.size()) return false;
        
        size_t local_offset = 0;
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = static_cast<char>(obj.via.u64);
        } else if (obj.type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
            value = static_cast<char>(obj.via.i64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, int& value) {
        if (offset >= input.size()) return false;
        
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = static_cast<int>(obj.via.u64);
        } else if (obj.type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
            value = static_cast<int>(obj.via.i64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, long& value) {
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = static_cast<long>(obj.via.u64);
        } else if (obj.type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
            value = static_cast<long>(obj.via.i64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, long long& value) {
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = static_cast<long long>(obj.via.u64);
        } else if (obj.type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
            value = static_cast<long long>(obj.via.i64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, unsigned int& value) {
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = static_cast<unsigned int>(obj.via.u64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, unsigned long& value) {
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = static_cast<unsigned long>(obj.via.u64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, unsigned long long& value) {
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = obj.via.u64;
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, float& value) {
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_FLOAT32 || obj.type == MSGPACK_OBJECT_FLOAT64 || obj.type == MSGPACK_OBJECT_FLOAT) {
            value = static_cast<float>(obj.via.f64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, double& value) {
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_FLOAT32 || obj.type == MSGPACK_OBJECT_FLOAT64 || obj.type == MSGPACK_OBJECT_FLOAT) {
            value = obj.via.f64;
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, bool& value) {
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_BOOLEAN) {
            value = obj.via.boolean;
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, std::string& value) {
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;  // Local offset relative to buffer start
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_STR) {
            value.assign(obj.via.str.ptr, obj.via.str.size);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Update absolute offset
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, std::wstring& value) {
        // Deserialize wstring as binary data (array of wchar_t bytes)
        if (offset >= input.size()) return false;
        
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        size_t local_offset = 0;
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_BIN) {
            // Binary data - interpret as wchar_t array
            size_t byte_size = obj.via.bin.size;
            size_t char_count = byte_size / sizeof(wchar_t);
            if (byte_size % sizeof(wchar_t) != 0) {
                msgpack_unpacked_destroy(&msg);
                return false;
            }
            value.assign(reinterpret_cast<const wchar_t*>(obj.via.bin.ptr), char_count);
        } else if (obj.type == MSGPACK_OBJECT_STR) {
            // String data - treat as binary (for backward compatibility)
            size_t byte_size = obj.via.str.size;
            size_t char_count = byte_size / sizeof(wchar_t);
            if (byte_size % sizeof(wchar_t) != 0) {
                msgpack_unpacked_destroy(&msg);
                return false;
            }
            value.assign(reinterpret_cast<const wchar_t*>(obj.via.str.ptr), char_count);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, uint8_t& value) {
        if (offset >= input.size()) return false;
        
        size_t local_offset = 0;
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = static_cast<uint8_t>(obj.via.u64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, short& value) {
        if (offset >= input.size()) return false;
        
        size_t local_offset = 0;
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = static_cast<short>(obj.via.u64);
        } else if (obj.type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
            value = static_cast<short>(obj.via.i64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    static bool deserialize_impl(const std::vector<uint8_t>& input, size_t& offset, unsigned short& value) {
        if (offset >= input.size()) return false;
        
        size_t local_offset = 0;
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        msgpack_unpack_return ret = msgpack_unpack_next(&msg, 
            reinterpret_cast<const char*>(input.data() + offset), 
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            value = static_cast<unsigned short>(obj.via.u64);
        } else {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        offset += local_offset;
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    // Container serialization helpers for std::vector
    template<typename T>
    static bool serialize_vector_impl(const std::vector<T>& container, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        
        // Pack array header with size
        size_t size = container.size();
        msgpack_pack_array(&pk, size);
        
        // Pack each element using msgpack_packer
        for (const auto& elem : container) {
            if (!pack_element(pk, elem)) {
                msgpack_sbuffer_destroy(&sbuf);
                return false;
            }
        }
        
        // Append the packed data
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    // Container serialization helpers for std::list
    template<typename T>
    static bool serialize_list_impl(const std::list<T>& container, std::vector<uint8_t>& output) {
        msgpack_sbuffer sbuf;
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer pk;
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
        
        // Pack array header with size
        size_t size = container.size();
        msgpack_pack_array(&pk, size);
        
        // Pack each element using msgpack_packer
        for (const auto& elem : container) {
            if (!pack_element(pk, elem)) {
                msgpack_sbuffer_destroy(&sbuf);
                return false;
            }
        }
        
        // Append the packed data
        output.insert(output.end(), sbuf.data, sbuf.data + sbuf.size);
        msgpack_sbuffer_destroy(&sbuf);
        return true;
    }
    
    // Helper to pack individual elements
    template<typename T>
    static bool pack_element(msgpack_packer& pk, const T& elem) {
        return pack_element_impl(pk, elem, typename std::is_arithmetic<T>::type{});
    }
    
    template<typename T>
    static bool pack_element_impl(msgpack_packer& pk, const T& elem, std::true_type) {
        // Arithmetic types - use type traits for better matching
        if (std::is_signed<T>::value && std::is_integral<T>::value) {
            // Signed integer types
            if (sizeof(T) <= sizeof(int32_t)) {
                msgpack_pack_int32(&pk, static_cast<int32_t>(elem));
            } else {
                msgpack_pack_int64(&pk, static_cast<int64_t>(elem));
            }
        } else if (std::is_unsigned<T>::value && std::is_integral<T>::value) {
            // Unsigned integer types
            if (sizeof(T) <= sizeof(uint32_t)) {
                msgpack_pack_uint32(&pk, static_cast<uint32_t>(elem));
            } else {
                msgpack_pack_uint64(&pk, static_cast<uint64_t>(elem));
            }
        } else if (std::is_same<T, float>::value) {
            msgpack_pack_float(&pk, static_cast<float>(elem));
        } else if (std::is_same<T, double>::value) {
            msgpack_pack_double(&pk, static_cast<double>(elem));
        } else if (std::is_same<T, bool>::value) {
            msgpack_pack_int(&pk, elem ? 1 : 0);
        } else {
            return false;
        }
        return true;
    }
    
    // Specialization for std::string
    static bool pack_element_impl(msgpack_packer& pk, const std::string& str, std::false_type) {
        msgpack_pack_str(&pk, str.size());
        msgpack_pack_str_body(&pk, str.data(), str.size());
        return true;
    }
    
    // Specialization for std::wstring
    static bool pack_element_impl(msgpack_packer& pk, const std::wstring& wstr, std::false_type) {
        // Serialize wstring as binary data (array of wchar_t bytes)
        size_t byte_size = wstr.size() * sizeof(wchar_t);
        msgpack_pack_bin(&pk, byte_size);
        msgpack_pack_bin_body(&pk, wstr.data(), byte_size);
        return true;
    }
    
    // Generic fallback for other non-arithmetic types
    template<typename T>
    static typename std::enable_if<!std::is_same<T, std::string>::value && 
                                   !std::is_same<T, std::wstring>::value, bool>::type
    pack_element_impl(msgpack_packer& pk, const T& elem, std::false_type) {
        return false;
    }
    
    // Container deserialization helpers for std::vector
    template<typename T>
    static bool deserialize_vector_impl(const std::vector<uint8_t>& input, size_t& offset,
                                       std::vector<T>& container) {
        if (offset >= input.size()) return false;
        
        size_t local_offset = 0;
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        msgpack_unpack_return ret = msgpack_unpack_next(&msg,
            reinterpret_cast<const char*>(input.data() + offset),
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type != MSGPACK_OBJECT_ARRAY) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Clear container
        container.clear();
        size_t array_size = obj.via.array.size;
        
        // Update offset for array header
        offset += local_offset;
        
        // Extract elements from the unpacked array
        for (size_t i = 0; i < array_size; ++i) {
            msgpack_object elem_obj = obj.via.array.ptr[i];
            T elem;
            if (!unpack_element(elem_obj, elem)) {
                msgpack_unpacked_destroy(&msg);
                return false;
            }
            container.push_back(elem);
        }
        
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    // Container deserialization helpers for std::list
    template<typename T>
    static bool deserialize_list_impl(const std::vector<uint8_t>& input, size_t& offset,
                                      std::list<T>& container) {
        if (offset >= input.size()) return false;
        
        size_t local_offset = 0;
        msgpack_unpacked msg;
        msgpack_unpacked_init(&msg);
        msgpack_unpack_return ret = msgpack_unpack_next(&msg,
            reinterpret_cast<const char*>(input.data() + offset),
            input.size() - offset, &local_offset);
        
        if (ret != MSGPACK_UNPACK_SUCCESS) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        msgpack_object obj = msg.data;
        if (obj.type != MSGPACK_OBJECT_ARRAY) {
            msgpack_unpacked_destroy(&msg);
            return false;
        }
        
        // Clear container
        container.clear();
        size_t array_size = obj.via.array.size;
        
        // Update offset for array header
        offset += local_offset;
        
        // Extract elements from the unpacked array
        for (size_t i = 0; i < array_size; ++i) {
            msgpack_object elem_obj = obj.via.array.ptr[i];
            T elem;
            if (!unpack_element(elem_obj, elem)) {
                msgpack_unpacked_destroy(&msg);
                return false;
            }
            container.push_back(elem);
        }
        
        msgpack_unpacked_destroy(&msg);
        return true;
    }
    
    // Helper to unpack individual elements from msgpack_object
    template<typename T>
    static bool unpack_element(const msgpack_object& obj, T& elem) {
        return unpack_element_impl(obj, elem, typename std::is_arithmetic<T>::type{});
    }
    
    template<typename T>
    static bool unpack_element_impl(const msgpack_object& obj, T& elem, std::true_type) {
        // Arithmetic types - use type traits for better matching
        if (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            if (std::is_floating_point<T>::value) {
                // Floating point types can be packed as integers in some cases
                if (std::is_same<T, float>::value) {
                    elem = static_cast<float>(obj.via.u64);
                } else if (std::is_same<T, double>::value) {
                    elem = static_cast<double>(obj.via.u64);
                } else {
                    return false;
                }
            } else if (std::is_unsigned<T>::value && std::is_integral<T>::value) {
                // Unsigned integer types
                elem = static_cast<T>(obj.via.u64);
            } else if (std::is_signed<T>::value && std::is_integral<T>::value) {
                // Signed integer types (can be positive)
                elem = static_cast<T>(obj.via.u64);
            } else if (std::is_same<T, bool>::value) {
                elem = (obj.via.u64 != 0);
            } else {
                return false;
            }
        } else if (obj.type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
            if (std::is_floating_point<T>::value) {
                // Floating point types can be packed as integers in some cases
                if (std::is_same<T, float>::value) {
                    elem = static_cast<float>(obj.via.i64);
                } else if (std::is_same<T, double>::value) {
                    elem = static_cast<double>(obj.via.i64);
                } else {
                    return false;
                }
            } else if (std::is_signed<T>::value && std::is_integral<T>::value) {
                // Signed integer types (negative)
                elem = static_cast<T>(obj.via.i64);
            } else {
                return false;
            }
        } else if (obj.type == MSGPACK_OBJECT_FLOAT32) {
            // Handle float (32-bit)
            if (std::is_same<T, float>::value) {
                elem = static_cast<float>(obj.via.f64);
            } else if (std::is_same<T, double>::value) {
                // Can convert float32 to double
                elem = static_cast<double>(obj.via.f64);
            } else {
                return false;
            }
        } else if (obj.type == MSGPACK_OBJECT_FLOAT64) {
            // Handle double (64-bit)
            if (std::is_same<T, double>::value) {
                elem = static_cast<double>(obj.via.f64);
            } else if (std::is_same<T, float>::value) {
                // Can convert double to float (with potential precision loss)
                elem = static_cast<float>(obj.via.f64);
            } else {
                return false;
            }
        } else if (obj.type == MSGPACK_OBJECT_FLOAT) {
            // Legacy/fallback: handle MSGPACK_OBJECT_FLOAT (if it exists)
            // msgpack-c uses f64 for both float and double in this case
            if (std::is_floating_point<T>::value) {
                if (std::is_same<T, float>::value) {
                    elem = static_cast<float>(obj.via.f64);
                } else if (std::is_same<T, double>::value) {
                    elem = static_cast<double>(obj.via.f64);
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
        return true;
    }
    
    // Specialization for std::string
    static bool unpack_element_impl(const msgpack_object& obj, std::string& elem, std::false_type) {
        if (obj.type == MSGPACK_OBJECT_STR) {
            elem.assign(obj.via.str.ptr, obj.via.str.size);
            return true;
        }
        return false;
    }
    
    // Specialization for std::wstring
    static bool unpack_element_impl(const msgpack_object& obj, std::wstring& elem, std::false_type) {
        // Deserialize wstring as binary data (array of wchar_t bytes)
        if (obj.type == MSGPACK_OBJECT_BIN) {
            size_t byte_size = obj.via.bin.size;
            size_t char_count = byte_size / sizeof(wchar_t);
            if (byte_size % sizeof(wchar_t) != 0) {
                return false;
            }
            elem.assign(reinterpret_cast<const wchar_t*>(obj.via.bin.ptr), char_count);
            return true;
        } else if (obj.type == MSGPACK_OBJECT_STR) {
            // String data - treat as binary (for backward compatibility)
            size_t byte_size = obj.via.str.size;
            size_t char_count = byte_size / sizeof(wchar_t);
            if (byte_size % sizeof(wchar_t) != 0) {
                return false;
            }
            elem.assign(reinterpret_cast<const wchar_t*>(obj.via.str.ptr), char_count);
            return true;
        }
        return false;
    }
    
    // Generic fallback for other non-arithmetic types
    template<typename T>
    static typename std::enable_if<!std::is_same<T, std::string>::value && 
                                   !std::is_same<T, std::wstring>::value, bool>::type
    unpack_element_impl(const msgpack_object& obj, T& elem, std::false_type) {
        return false;
    }
    
    bool serialize_value(size_t type_hash, const void* ptr, std::vector<uint8_t>& output) {
        // First check built-in serializers
        auto it = serializers_.find(type_hash);
        if (it != serializers_.end()) {
            return it->second(ptr, output);
        }
        // Then check custom serializers (for user-defined types)
        auto custom_it = custom_serializers_.find(type_hash);
        if (custom_it != custom_serializers_.end()) {
            return custom_it->second(ptr, output);
        }
        return false;
    }
    
    bool deserialize_value(size_t type_hash, const std::vector<uint8_t>& input, 
                          size_t& offset, void* ptr) {
        // First check built-in deserializers
        auto it = deserializers_.find(type_hash);
        if (it != deserializers_.end()) {
            return it->second(input, offset, ptr);
        }
        // Then check custom deserializers (for user-defined types)
        auto custom_it = custom_deserializers_.find(type_hash);
        if (custom_it != custom_deserializers_.end()) {
            return custom_it->second(input, offset, ptr);
        }
        return false;
    }
    
    // Helper to deserialize value and set it using set() method
    template<typename T>
    bool deserialize_and_set_arg(const std::vector<uint8_t>& input, size_t& offset,
                                 IDelegateArgs* args, size_t idx) {
        return deserialize_and_set_arg_impl(input, offset, args, idx, T{});
    }
    
    template<typename T>
    bool deserialize_and_set_arg_impl(const std::vector<uint8_t>& input, size_t& offset,
                                     IDelegateArgs* args, size_t idx, T) {
        // For non-container types
        T value;
        if (!deserialize_impl(input, offset, value)) {
            return false;
        }
        return args->set<T>(idx, value);
    }
    
    // Specialization for std::vector
    template<typename T>
    bool deserialize_and_set_arg_impl(const std::vector<uint8_t>& input, size_t& offset,
                                     IDelegateArgs* args, size_t idx, std::vector<T>) {
        std::vector<T> value;
        if (!deserialize_vector_impl(input, offset, value)) {
            return false;
        }
        return args->set<std::vector<T>>(idx, value);
    }
    
    // Specialization for std::list
    template<typename T>
    bool deserialize_and_set_arg_impl(const std::vector<uint8_t>& input, size_t& offset,
                                     IDelegateArgs* args, size_t idx, std::list<T>) {
        std::list<T> value;
        if (!deserialize_list_impl(input, offset, value)) {
            return false;
        }
        return args->set<std::list<T>>(idx, value);
    }
    
    template<typename T>
    bool deserialize_and_set_result(const std::vector<uint8_t>& input, size_t& offset,
                                   IDelegateResult* result) {
        return deserialize_and_set_result_impl(input, offset, result, T{});
    }
    
    template<typename T>
    bool deserialize_and_set_result_impl(const std::vector<uint8_t>& input, size_t& offset,
                                        IDelegateResult* result, T) {
        // For non-container types
        T value;
        if (!deserialize_impl(input, offset, value)) {
            return false;
        }
        return result->set<T>(value);
    }
    
    // Specialization for std::vector
    template<typename T>
    bool deserialize_and_set_result_impl(const std::vector<uint8_t>& input, size_t& offset,
                                         IDelegateResult* result, std::vector<T>) {
        std::vector<T> value;
        if (!deserialize_vector_impl(input, offset, value)) {
            return false;
        }
        return result->set<std::vector<T>>(value);
    }
    
    // Specialization for std::list
    template<typename T>
    bool deserialize_and_set_result_impl(const std::vector<uint8_t>& input, size_t& offset,
                                         IDelegateResult* result, std::list<T>) {
        std::list<T> value;
        if (!deserialize_list_impl(input, offset, value)) {
            return false;
        }
        return result->set<std::list<T>>(value);
    }
    
    bool deserialize_value_to_arg(size_t type_hash, const std::vector<uint8_t>& input,
                                 size_t& offset, IDelegateArgs* args, size_t idx) {
        // Match type hash to known types and deserialize
        if (type_hash == typeid(int).hash_code()) {
            return deserialize_and_set_arg<int>(input, offset, args, idx);
        } else if (type_hash == typeid(long).hash_code()) {
            return deserialize_and_set_arg<long>(input, offset, args, idx);
        } else if (type_hash == typeid(long long).hash_code()) {
            return deserialize_and_set_arg<long long>(input, offset, args, idx);
        } else if (type_hash == typeid(unsigned int).hash_code()) {
            return deserialize_and_set_arg<unsigned int>(input, offset, args, idx);
        } else if (type_hash == typeid(unsigned long).hash_code()) {
            return deserialize_and_set_arg<unsigned long>(input, offset, args, idx);
        } else if (type_hash == typeid(unsigned long long).hash_code()) {
            return deserialize_and_set_arg<unsigned long long>(input, offset, args, idx);
        } else if (type_hash == typeid(float).hash_code()) {
            return deserialize_and_set_arg<float>(input, offset, args, idx);
        } else if (type_hash == typeid(double).hash_code()) {
            return deserialize_and_set_arg<double>(input, offset, args, idx);
        } else if (type_hash == typeid(bool).hash_code()) {
            // bool is serialized as int in msgpack-c
            int int_value;
            if (!deserialize_impl(input, offset, int_value)) {
                return false;
            }
            bool bool_value = (int_value != 0);
            return args->set<bool>(idx, bool_value);
        } else if (type_hash == typeid(std::string).hash_code()) {
            return deserialize_and_set_arg<std::string>(input, offset, args, idx);
        } else if (type_hash == typeid(std::wstring).hash_code()) {
            return deserialize_and_set_arg<std::wstring>(input, offset, args, idx);
        } else if (type_hash == typeid(char).hash_code()) {
            return deserialize_and_set_arg<char>(input, offset, args, idx);
        } else if (type_hash == typeid(uint8_t).hash_code()) {
            // Note: unsigned char and uint8_t are the same type
            return deserialize_and_set_arg<uint8_t>(input, offset, args, idx);
        } else if (type_hash == typeid(short).hash_code()) {
            return deserialize_and_set_arg<short>(input, offset, args, idx);
        } else if (type_hash == typeid(unsigned short).hash_code()) {
            return deserialize_and_set_arg<unsigned short>(input, offset, args, idx);
        }
        
        // Check for container types - try common container types
        if (type_hash == typeid(std::vector<int>).hash_code()) {
            return deserialize_and_set_arg<std::vector<int>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::vector<std::string>).hash_code()) {
            return deserialize_and_set_arg<std::vector<std::string>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::list<int>).hash_code()) {
            return deserialize_and_set_arg<std::list<int>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::list<std::string>).hash_code()) {
            return deserialize_and_set_arg<std::list<std::string>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::vector<short>).hash_code()) {
            return deserialize_and_set_arg<std::vector<short>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::vector<float>).hash_code()) {
            return deserialize_and_set_arg<std::vector<float>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::vector<double>).hash_code()) {
            return deserialize_and_set_arg<std::vector<double>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::list<short>).hash_code()) {
            return deserialize_and_set_arg<std::list<short>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::list<float>).hash_code()) {
            return deserialize_and_set_arg<std::list<float>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::list<double>).hash_code()) {
            return deserialize_and_set_arg<std::list<double>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::vector<long>).hash_code()) {
            return deserialize_and_set_arg<std::vector<long>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::vector<unsigned int>).hash_code()) {
            return deserialize_and_set_arg<std::vector<unsigned int>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::list<long>).hash_code()) {
            return deserialize_and_set_arg<std::list<long>>(input, offset, args, idx);
        } else if (type_hash == typeid(std::list<unsigned int>).hash_code()) {
            return deserialize_and_set_arg<std::list<unsigned int>>(input, offset, args, idx);
        }
        
        return false;
    }
    
    bool deserialize_value_to_result(size_t type_hash, const std::vector<uint8_t>& input,
                                    size_t& offset, IDelegateResult* result) {
        // Match type hash to known types and deserialize
        if (type_hash == typeid(int).hash_code()) {
            return deserialize_and_set_result<int>(input, offset, result);
        } else if (type_hash == typeid(long).hash_code()) {
            return deserialize_and_set_result<long>(input, offset, result);
        } else if (type_hash == typeid(long long).hash_code()) {
            return deserialize_and_set_result<long long>(input, offset, result);
        } else if (type_hash == typeid(unsigned int).hash_code()) {
            return deserialize_and_set_result<unsigned int>(input, offset, result);
        } else if (type_hash == typeid(unsigned long).hash_code()) {
            return deserialize_and_set_result<unsigned long>(input, offset, result);
        } else if (type_hash == typeid(unsigned long long).hash_code()) {
            return deserialize_and_set_result<unsigned long long>(input, offset, result);
        } else if (type_hash == typeid(float).hash_code()) {
            return deserialize_and_set_result<float>(input, offset, result);
        } else if (type_hash == typeid(double).hash_code()) {
            return deserialize_and_set_result<double>(input, offset, result);
        } else if (type_hash == typeid(bool).hash_code()) {
            // bool is serialized as int in msgpack-c
            int int_value;
            if (!deserialize_impl(input, offset, int_value)) {
                return false;
            }
            bool bool_value = (int_value != 0);
            return result->set<bool>(bool_value);
        } else if (type_hash == typeid(std::string).hash_code()) {
            return deserialize_and_set_result<std::string>(input, offset, result);
        } else if (type_hash == typeid(std::wstring).hash_code()) {
            return deserialize_and_set_result<std::wstring>(input, offset, result);
        } else if (type_hash == typeid(char).hash_code()) {
            return deserialize_and_set_result<char>(input, offset, result);
        } else if (type_hash == typeid(uint8_t).hash_code()) {
            // Note: unsigned char and uint8_t are the same type
            return deserialize_and_set_result<uint8_t>(input, offset, result);
        } else if (type_hash == typeid(short).hash_code()) {
            return deserialize_and_set_result<short>(input, offset, result);
        } else if (type_hash == typeid(unsigned short).hash_code()) {
            return deserialize_and_set_result<unsigned short>(input, offset, result);
        } else if (type_hash == typeid(std::vector<int>).hash_code()) {
            return deserialize_and_set_result<std::vector<int>>(input, offset, result);
        } else if (type_hash == typeid(std::vector<std::string>).hash_code()) {
            return deserialize_and_set_result<std::vector<std::string>>(input, offset, result);
        } else if (type_hash == typeid(std::list<int>).hash_code()) {
            return deserialize_and_set_result<std::list<int>>(input, offset, result);
        } else if (type_hash == typeid(std::list<std::string>).hash_code()) {
            return deserialize_and_set_result<std::list<std::string>>(input, offset, result);
        } else if (type_hash == typeid(std::vector<short>).hash_code()) {
            return deserialize_and_set_result<std::vector<short>>(input, offset, result);
        } else if (type_hash == typeid(std::vector<float>).hash_code()) {
            return deserialize_and_set_result<std::vector<float>>(input, offset, result);
        } else if (type_hash == typeid(std::vector<double>).hash_code()) {
            return deserialize_and_set_result<std::vector<double>>(input, offset, result);
        } else if (type_hash == typeid(std::list<short>).hash_code()) {
            return deserialize_and_set_result<std::list<short>>(input, offset, result);
        } else if (type_hash == typeid(std::list<float>).hash_code()) {
            return deserialize_and_set_result<std::list<float>>(input, offset, result);
        } else if (type_hash == typeid(std::list<double>).hash_code()) {
            return deserialize_and_set_result<std::list<double>>(input, offset, result);
        } else if (type_hash == typeid(std::vector<long>).hash_code()) {
            return deserialize_and_set_result<std::vector<long>>(input, offset, result);
        } else if (type_hash == typeid(std::vector<unsigned int>).hash_code()) {
            return deserialize_and_set_result<std::vector<unsigned int>>(input, offset, result);
        } else if (type_hash == typeid(std::list<long>).hash_code()) {
            return deserialize_and_set_result<std::list<long>>(input, offset, result);
        } else if (type_hash == typeid(std::list<unsigned int>).hash_code()) {
            return deserialize_and_set_result<std::list<unsigned int>>(input, offset, result);
        }
        return false;
    }
    
    using SerializeFunc = std::function<bool(const void*, std::vector<uint8_t>&)>;
    using DeserializeFunc = std::function<bool(const std::vector<uint8_t>&, size_t&, void*)>;
    using SizeCalculator = std::function<size_t(const IDelegateArgs*, size_t)>;
    
    std::unordered_map<size_t, SerializeFunc> serializers_;
    std::unordered_map<size_t, DeserializeFunc> deserializers_;
    std::unordered_map<size_t, SizeCalculator> size_calculators_;
    // Custom serializers for user-defined types
    std::unordered_map<size_t, SerializeFunc> custom_serializers_;
    std::unordered_map<size_t, DeserializeFunc> custom_deserializers_;
};

} // namespace serialization
} // namespace delegates

DELEGATES_BASE_NAMESPACE_END

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // DELEGATES_WITH_BINARY_SERIALIZATION

#endif // DELEGATES_BINARY_SERIALIZER_HEADER
