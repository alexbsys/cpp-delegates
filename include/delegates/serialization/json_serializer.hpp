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

#ifndef DELEGATES_JSON_SERIALIZER_HEADER
#define DELEGATES_JSON_SERIALIZER_HEADER

#ifdef DELEGATES_WITH_JSON_SERIALIZATION

#include "i_serializer.h"
#include <unordered_map>
#include <functional>
#include <cstring>
#include <type_traits>
#include <vector>
#include <list>
#include <string>
#include <cstdint>

// Include nlohmann/json if not already included
#ifndef NLOHMANN_JSON_HPP
#include <nlohmann/json.hpp>
#endif

DELEGATES_BASE_NAMESPACE_BEGIN

namespace delegates {
namespace serialization {

/// \brief    JSON serializer implementation using nlohmann/json
class JsonSerializer : public ISerializer {
public:
    JsonSerializer() {
        register_basic_types();
    }
    
    bool serialize_arg(size_t idx, const IDelegateArgs* args, 
                      std::vector<uint8_t>& output) override {
        if (!args || idx >= args->size()) return false;
        
        size_t type_hash = args->hash_code(idx);
        if (!can_serialize(type_hash)) return false;
        
        void* ptr = const_cast<IDelegateArgs*>(args)->get_ptr(idx);
        if (!ptr) return false;
        
        nlohmann::json json_value;
        if (!serialize_value(type_hash, ptr, json_value)) {
            return false;
        }
        
        std::string json_str = json_value.dump();
        output.insert(output.end(), json_str.begin(), json_str.end());
        
        // Add null terminator for string compatibility
        output.push_back(0);
        
        return true;
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
        
        // Find null terminator
        size_t str_end = offset;
        while (str_end < input.size() && input[str_end] != 0) {
            ++str_end;
        }
        
        if (str_end >= input.size()) return false;
        
        std::string json_str(reinterpret_cast<const char*>(input.data() + offset), 
                            str_end - offset);
        offset = str_end + 1;  // Skip null terminator
        
        try {
            nlohmann::json json_value = nlohmann::json::parse(json_str);
            // Use set() method to properly set the value
            return deserialize_value_to_arg(type_hash, json_value, args, idx);
        } catch (...) {
            return false;
        }
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
        
        nlohmann::json json_value;
        if (!serialize_value(type_hash, ptr, json_value)) {
            return false;
        }
        
        std::string json_str = json_value.dump();
        output.insert(output.end(), json_str.begin(), json_str.end());
        output.push_back(0);
        
        return true;
    }
    
    bool deserialize_result(IDelegateResult* result,
                           const std::vector<uint8_t>& input, size_t& offset) override {
        if (!result) return false;
        
        size_t type_hash = result->hash_code();
        
        // Check if it's a custom type
        auto custom_it = custom_deserializers_.find(type_hash);
        if (custom_it != custom_deserializers_.end()) {
            void* ptr = result->get_ptr();
            if (!ptr) return false;
            return custom_it->second(input, offset, ptr);
        }
        
        if (!can_serialize(type_hash)) return false;
        
        // Find null terminator
        size_t str_end = offset;
        while (str_end < input.size() && input[str_end] != 0) {
            ++str_end;
        }
        
        if (str_end >= input.size()) return false;
        
        std::string json_str(reinterpret_cast<const char*>(input.data() + offset), 
                            str_end - offset);
        offset = str_end + 1;
        
        try {
            nlohmann::json json_value = nlohmann::json::parse(json_str);
            // Use set() method to properly set the result value
            return deserialize_value_to_result(type_hash, json_value, result);
        } catch (...) {
            return false;
        }
    }
    
    bool can_serialize(size_t type_hash) const override {
        return serializers_.find(type_hash) != serializers_.end() ||
               custom_serializers_.find(type_hash) != custom_serializers_.end();
    }
    
    size_t get_serialized_size(size_t idx, const IDelegateArgs* args) const override {
        // JSON size is variable, return 0 (unknown)
        (void)idx;
        (void)args;
        return 0;
    }
    
    /// \brief    Register custom type serializer
    template<typename T>
    typename std::enable_if<!std::is_same<T, std::wstring>::value, void>::type
    register_type() {
        size_t hash = typeid(T).hash_code();
        serializers_[hash] = [](const void* ptr, nlohmann::json& json) {
            const T& value = *static_cast<const T*>(ptr);
            json = value;
            return true;
        };
        deserializers_[hash] = [](const nlohmann::json& json, void* ptr) {
            T& value = *static_cast<T*>(ptr);
            value = json.get<T>();
            return true;
        };
    }
    
    // Overload for std::wstring (convert to/from UTF-8 string)
    template<typename T>
    typename std::enable_if<std::is_same<T, std::wstring>::value, void>::type
    register_type() {
        size_t hash = typeid(std::wstring).hash_code();
        serializers_[hash] = [](const void* ptr, nlohmann::json& json) {
            const std::wstring& wstr = *static_cast<const std::wstring*>(ptr);
            // Convert wstring to UTF-8 string
            std::string utf8_str;
            utf8_str.reserve(wstr.size() * 4);
            for (wchar_t wc : wstr) {
                if (wc < 0x80) {
                    utf8_str += static_cast<char>(wc);
                } else if (wc < 0x800) {
                    utf8_str += static_cast<char>(0xC0 | (wc >> 6));
                    utf8_str += static_cast<char>(0x80 | (wc & 0x3F));
                } else if (wc < 0xD800 || wc >= 0xE000) {
                    utf8_str += static_cast<char>(0xE0 | (wc >> 12));
                    utf8_str += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
                    utf8_str += static_cast<char>(0x80 | (wc & 0x3F));
                }
            }
            json = utf8_str;
            return true;
        };
        deserializers_[hash] = [](const nlohmann::json& json, void* ptr) {
            std::wstring& wstr = *static_cast<std::wstring*>(ptr);
            // Get UTF-8 string from JSON
            std::string utf8_str = json.get<std::string>();
            // Convert UTF-8 to wstring
            wstr.clear();
            wstr.reserve(utf8_str.size());
            for (size_t i = 0; i < utf8_str.size(); ) {
                unsigned char c = static_cast<unsigned char>(utf8_str[i]);
                if (c < 0x80) {
                    wstr += static_cast<wchar_t>(c);
                    ++i;
                } else if ((c & 0xE0) == 0xC0) {
                    if (i + 1 >= utf8_str.size()) return false;
                    wchar_t wc = ((c & 0x1F) << 6) | (static_cast<unsigned char>(utf8_str[i+1]) & 0x3F);
                    wstr += wc;
                    i += 2;
                } else if ((c & 0xF0) == 0xE0) {
                    if (i + 2 >= utf8_str.size()) return false;
                    wchar_t wc = ((c & 0x0F) << 12) | 
                                 ((static_cast<unsigned char>(utf8_str[i+1]) & 0x3F) << 6) |
                                 (static_cast<unsigned char>(utf8_str[i+2]) & 0x3F);
                    wstr += wc;
                    i += 3;
                } else {
                    return false;  // Invalid UTF-8
                }
            }
            return true;
        };
    }
    
    /// \brief    Register container type (vector/list) for element type T
    template<typename T>
    void register_container_type() {
        // Register std::vector<T>
        register_type<std::vector<T>>();
        // Register std::list<T>
        register_type<std::list<T>>();
    }
    
private:
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
        // Boolean
        register_type<bool>();
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
    
    bool serialize_value(size_t type_hash, const void* ptr, nlohmann::json& json) {
        auto it = serializers_.find(type_hash);
        if (it == serializers_.end()) return false;
        return it->second(ptr, json);
    }
    
    // Helper to deserialize value and set it using set() method
    template<typename T>
    typename std::enable_if<!std::is_same<T, std::wstring>::value, bool>::type
    deserialize_and_set_arg(const nlohmann::json& json, IDelegateArgs* args, size_t idx) {
        T value;
        try {
            value = json.get<T>();
            return args->set<T>(idx, value);
        } catch (...) {
            return false;
        }
    }
    
    // Overload for std::wstring (nlohmann/json doesn't support wstring directly)
    bool deserialize_and_set_arg(const nlohmann::json& json, IDelegateArgs* args, size_t idx, std::wstring*) {
        try {
            // nlohmann/json stores strings as UTF-8, convert to wstring
            std::string utf8_str = json.get<std::string>();
            std::wstring wstr;
            wstr.reserve(utf8_str.size());
            for (size_t i = 0; i < utf8_str.size(); ) {
                unsigned char c = static_cast<unsigned char>(utf8_str[i]);
                if (c < 0x80) {
                    wstr += static_cast<wchar_t>(c);
                    ++i;
                } else if ((c & 0xE0) == 0xC0) {
                    if (i + 1 >= utf8_str.size()) return false;
                    wchar_t wc = ((c & 0x1F) << 6) | (static_cast<unsigned char>(utf8_str[i+1]) & 0x3F);
                    wstr += wc;
                    i += 2;
                } else if ((c & 0xF0) == 0xE0) {
                    if (i + 2 >= utf8_str.size()) return false;
                    wchar_t wc = ((c & 0x0F) << 12) | 
                                 ((static_cast<unsigned char>(utf8_str[i+1]) & 0x3F) << 6) |
                                 (static_cast<unsigned char>(utf8_str[i+2]) & 0x3F);
                    wstr += wc;
                    i += 3;
                } else {
                    return false;  // Invalid UTF-8
                }
            }
            return args->set<std::wstring>(idx, wstr);
        } catch (...) {
            return false;
        }
    }
    
    template<typename T>
    typename std::enable_if<!std::is_same<T, std::wstring>::value, bool>::type
    deserialize_and_set_result(const nlohmann::json& json, IDelegateResult* result) {
        T value;
        try {
            value = json.get<T>();
            return result->set<T>(value);
        } catch (...) {
            return false;
        }
    }
    
    // Overload for std::wstring (nlohmann/json doesn't support wstring directly)
    bool deserialize_and_set_result(const nlohmann::json& json, IDelegateResult* result, std::wstring*) {
        try {
            // nlohmann/json stores strings as UTF-8, convert to wstring
            std::string utf8_str = json.get<std::string>();
            std::wstring wstr;
            wstr.reserve(utf8_str.size());
            for (size_t i = 0; i < utf8_str.size(); ) {
                unsigned char c = static_cast<unsigned char>(utf8_str[i]);
                if (c < 0x80) {
                    wstr += static_cast<wchar_t>(c);
                    ++i;
                } else if ((c & 0xE0) == 0xC0) {
                    if (i + 1 >= utf8_str.size()) return false;
                    wchar_t wc = ((c & 0x1F) << 6) | (static_cast<unsigned char>(utf8_str[i+1]) & 0x3F);
                    wstr += wc;
                    i += 2;
                } else if ((c & 0xF0) == 0xE0) {
                    if (i + 2 >= utf8_str.size()) return false;
                    wchar_t wc = ((c & 0x0F) << 12) | 
                                 ((static_cast<unsigned char>(utf8_str[i+1]) & 0x3F) << 6) |
                                 (static_cast<unsigned char>(utf8_str[i+2]) & 0x3F);
                    wstr += wc;
                    i += 3;
                } else {
                    return false;  // Invalid UTF-8
                }
            }
            return result->set<std::wstring>(wstr);
        } catch (...) {
            return false;
        }
    }
    
    bool deserialize_value_to_arg(size_t type_hash, const nlohmann::json& json, 
                                  IDelegateArgs* args, size_t idx) {
        // Try to deserialize based on type hash
        // This is a type-erased approach - we need to match type hash to actual type
        return deserialize_value_to_arg_impl(type_hash, json, args, idx);
    }
    
    bool deserialize_value_to_result(size_t type_hash, const nlohmann::json& json, 
                                     IDelegateResult* result) {
        // Try to deserialize based on type hash
        return deserialize_value_to_result_impl(type_hash, json, result);
    }
    
private:
    // Type-specific deserialization helpers
    bool deserialize_value_to_arg_impl(size_t type_hash, const nlohmann::json& json,
                                      IDelegateArgs* args, size_t idx) {
        // Match type hash to known types and deserialize
        if (type_hash == typeid(int).hash_code()) {
            return deserialize_and_set_arg<int>(json, args, idx);
        } else if (type_hash == typeid(long).hash_code()) {
            return deserialize_and_set_arg<long>(json, args, idx);
        } else if (type_hash == typeid(long long).hash_code()) {
            return deserialize_and_set_arg<long long>(json, args, idx);
        } else if (type_hash == typeid(unsigned int).hash_code()) {
            return deserialize_and_set_arg<unsigned int>(json, args, idx);
        } else if (type_hash == typeid(unsigned long).hash_code()) {
            return deserialize_and_set_arg<unsigned long>(json, args, idx);
        } else if (type_hash == typeid(unsigned long long).hash_code()) {
            return deserialize_and_set_arg<unsigned long long>(json, args, idx);
        } else if (type_hash == typeid(float).hash_code()) {
            return deserialize_and_set_arg<float>(json, args, idx);
        } else if (type_hash == typeid(double).hash_code()) {
            return deserialize_and_set_arg<double>(json, args, idx);
        } else if (type_hash == typeid(bool).hash_code()) {
            return deserialize_and_set_arg<bool>(json, args, idx);
        } else if (type_hash == typeid(std::string).hash_code()) {
            return deserialize_and_set_arg<std::string>(json, args, idx);
        } else if (type_hash == typeid(std::wstring).hash_code()) {
            return deserialize_and_set_arg(json, args, idx, static_cast<std::wstring*>(nullptr));
        } else if (type_hash == typeid(char).hash_code()) {
            return deserialize_and_set_arg<char>(json, args, idx);
        } else if (type_hash == typeid(uint8_t).hash_code()) {
            // Note: unsigned char and uint8_t are the same type
            return deserialize_and_set_arg<uint8_t>(json, args, idx);
        } else if (type_hash == typeid(short).hash_code()) {
            return deserialize_and_set_arg<short>(json, args, idx);
        } else if (type_hash == typeid(unsigned short).hash_code()) {
            return deserialize_and_set_arg<unsigned short>(json, args, idx);
        } else if (type_hash == typeid(std::vector<int>).hash_code()) {
            return deserialize_and_set_arg<std::vector<int>>(json, args, idx);
        } else if (type_hash == typeid(std::vector<std::string>).hash_code()) {
            return deserialize_and_set_arg<std::vector<std::string>>(json, args, idx);
        } else if (type_hash == typeid(std::list<int>).hash_code()) {
            return deserialize_and_set_arg<std::list<int>>(json, args, idx);
        } else if (type_hash == typeid(std::list<std::string>).hash_code()) {
            return deserialize_and_set_arg<std::list<std::string>>(json, args, idx);
        }
        return false;
    }
    
    bool deserialize_value_to_result_impl(size_t type_hash, const nlohmann::json& json,
                                         IDelegateResult* result) {
        // Match type hash to known types and deserialize
        if (type_hash == typeid(int).hash_code()) {
            return deserialize_and_set_result<int>(json, result);
        } else if (type_hash == typeid(long).hash_code()) {
            return deserialize_and_set_result<long>(json, result);
        } else if (type_hash == typeid(long long).hash_code()) {
            return deserialize_and_set_result<long long>(json, result);
        } else if (type_hash == typeid(unsigned int).hash_code()) {
            return deserialize_and_set_result<unsigned int>(json, result);
        } else if (type_hash == typeid(unsigned long).hash_code()) {
            return deserialize_and_set_result<unsigned long>(json, result);
        } else if (type_hash == typeid(unsigned long long).hash_code()) {
            return deserialize_and_set_result<unsigned long long>(json, result);
        } else if (type_hash == typeid(float).hash_code()) {
            return deserialize_and_set_result<float>(json, result);
        } else if (type_hash == typeid(double).hash_code()) {
            return deserialize_and_set_result<double>(json, result);
        } else if (type_hash == typeid(bool).hash_code()) {
            return deserialize_and_set_result<bool>(json, result);
        } else if (type_hash == typeid(std::string).hash_code()) {
            return deserialize_and_set_result<std::string>(json, result);
        } else if (type_hash == typeid(std::wstring).hash_code()) {
            return deserialize_and_set_result(json, result, static_cast<std::wstring*>(nullptr));
        } else if (type_hash == typeid(char).hash_code()) {
            return deserialize_and_set_result<char>(json, result);
        } else if (type_hash == typeid(uint8_t).hash_code()) {
            // Note: unsigned char and uint8_t are the same type
            return deserialize_and_set_result<uint8_t>(json, result);
        } else if (type_hash == typeid(short).hash_code()) {
            return deserialize_and_set_result<short>(json, result);
        } else if (type_hash == typeid(unsigned short).hash_code()) {
            return deserialize_and_set_result<unsigned short>(json, result);
        } else if (type_hash == typeid(std::vector<int>).hash_code()) {
            return deserialize_and_set_result<std::vector<int>>(json, result);
        } else if (type_hash == typeid(std::vector<std::string>).hash_code()) {
            return deserialize_and_set_result<std::vector<std::string>>(json, result);
        } else if (type_hash == typeid(std::list<int>).hash_code()) {
            return deserialize_and_set_result<std::list<int>>(json, result);
        } else if (type_hash == typeid(std::list<std::string>).hash_code()) {
            return deserialize_and_set_result<std::list<std::string>>(json, result);
        }
        return false;
    }
    
    using SerializeFunc = std::function<bool(const void*, nlohmann::json&)>;
    using DeserializeFunc = std::function<bool(const nlohmann::json&, void*)>;
    using CustomSerializeFunc = std::function<bool(const void*, std::vector<uint8_t>&)>;
    using CustomDeserializeFunc = std::function<bool(const std::vector<uint8_t>&, size_t&, void*)>;
    
    std::unordered_map<size_t, SerializeFunc> serializers_;
    std::unordered_map<size_t, DeserializeFunc> deserializers_;
    std::unordered_map<size_t, CustomSerializeFunc> custom_serializers_;
    std::unordered_map<size_t, CustomDeserializeFunc> custom_deserializers_;
};

} // namespace serialization
} // namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif // DELEGATES_WITH_JSON_SERIALIZATION

#endif // DELEGATES_JSON_SERIALIZER_HEADER

