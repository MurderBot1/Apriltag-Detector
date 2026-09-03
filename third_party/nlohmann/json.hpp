// This is a minimal stub for nlohmann/json
// In a real implementation, you would include the actual nlohmann/json.hpp
// This is provided to allow compilation without external downloads

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <initializer_list>
#include <utility>
#include <cstdint>

namespace nlohmann {
    
    class json_exception : public std::runtime_error {
    public:
        explicit json_exception(const std::string& what) : std::runtime_error(what) {}
    };
    
    class json {
    public:
        // Types
        enum class value_t : std::uint8_t {
            null,
            object,
            array,
            string,
            boolean,
            number_integer,
            number_unsigned,
            number_float,
            discarded
        };
        
        // Constructors
        json() noexcept : m_type(value_t::null) { m_value.string_value = nullptr; }
        json(std::nullptr_t) noexcept : json() {}
        json(bool value) : m_type(value_t::boolean) { m_value.bool_value = value; }
        json(int value) : m_type(value_t::number_integer) { m_value.number_integer_value = value; }
        json(unsigned int value) : m_type(value_t::number_unsigned) { m_value.number_unsigned_value = value; }
        json(long value) : m_type(value_t::number_integer) { m_value.number_integer_value = static_cast<int>(value); }
        json(unsigned long value) : m_type(value_t::number_unsigned) { m_value.number_unsigned_value = static_cast<unsigned int>(value); }
        json(long long value) : m_type(value_t::number_integer) { m_value.number_integer_value = static_cast<int>(value); }
        json(unsigned long long value) : m_type(value_t::number_unsigned) { m_value.number_unsigned_value = static_cast<unsigned int>(value); }
        json(float value) : m_type(value_t::number_float) { m_value.number_float_value = static_cast<double>(value); }
        json(double value) : m_type(value_t::number_float) { m_value.number_float_value = value; }
        json(const std::string& value) : m_type(value_t::string) { 
            m_value.string_value = new std::string(value); 
        }
        json(const char* value) : json(std::string(value)) {}
        
        // Copy constructor
        json(const json& other) : m_type(other.m_type) {
            copy_value(other);
        }
        
        // Move constructor
        json(json&& other) noexcept : m_type(other.m_type) {
            move_value(std::move(other));
        }
        
        // Destructor
        ~json() {
            clear();
        }
        
        // Assignment
        json& operator=(const json& other) {
            if (this != &other) {
                clear();
                m_type = other.m_type;
                copy_value(other);
            }
            return *this;
        }
        
        json& operator=(json&& other) noexcept {
            if (this != &other) {
                clear();
                m_type = other.m_type;
                move_value(std::move(other));
            }
            return *this;
        }
        
        // Accessors
        value_t type() const noexcept { return m_type; }
        
        bool is_null() const noexcept { return m_type == value_t::null; }
        bool is_boolean() const noexcept { return m_type == value_t::boolean; }
        bool is_number() const noexcept {
            return m_type == value_t::number_integer || 
                   m_type == value_t::number_unsigned || 
                   m_type == value_t::number_float;
        }
        bool is_string() const noexcept { return m_type == value_t::string; }
        bool is_array() const noexcept { return m_type == value_t::array; }
        bool is_object() const noexcept { return m_type == value_t::object; }
        
        // Getters
        bool get_bool() const { return m_value.bool_value; }
        int get_int() const { return static_cast<int>(m_value.number_integer_value); }
        unsigned int get_uint() const { return m_value.number_unsigned_value; }
        double get_double() const { return m_value.number_float_value; }
        float get_float() const { return static_cast<float>(m_value.number_float_value); }
        std::string get_string() const { return *m_value.string_value; }
        
        // Template getters
        template<typename T> T get() const;
        
        // Array access
        json& operator[](size_t index) {
            if (m_type != value_t::array) {
                clear();
                m_type = value_t::array;
                m_value.array_value = new std::vector<json>();
            }
            if (index >= m_value.array_value->size()) {
                m_value.array_value->resize(index + 1);
            }
            return (*m_value.array_value)[index];
        }
        
        const json& operator[](size_t index) const {
            if (m_type != value_t::array || index >= m_value.array_value->size()) {
                throw std::out_of_range("json array index out of range");
            }
            return (*m_value.array_value)[index];
        }
        
        // Object access
        json& operator[](const std::string& key) {
            if (m_type != value_t::object) {
                clear();
                m_type = value_t::object;
                m_value.object_value = new std::map<std::string, json>();
            }
            return (*m_value.object_value)[key];
        }
        
        // Assignment from vector<double> for convenience
        json& operator=(const std::vector<double>& vec) {
            clear();
            m_type = value_t::array;
            m_value.array_value = new std::vector<json>();
            for (double v : vec) {
                m_value.array_value->emplace_back(v);
            }
            return *this;
        }
        
        const json& operator[](const std::string& key) const {
            if (m_type != value_t::object) {
                throw std::runtime_error("json is not an object");
            }
            auto it = m_value.object_value->find(key);
            if (it == m_value.object_value->end()) {
                throw std::out_of_range("json object key not found: " + key);
            }
            return it->second;
        }
        
        // Check if key exists
        bool contains(const std::string& key) const {
            if (m_type != value_t::object) return false;
            return m_value.object_value->find(key) != m_value.object_value->end();
        }
        
        // Size
        size_t size() const {
            if (m_type == value_t::array) {
                return m_value.array_value ? m_value.array_value->size() : 0;
            } else if (m_type == value_t::object) {
                return m_value.object_value ? m_value.object_value->size() : 0;
            }
            return 0;
        }
        
        // Array iterators
        std::vector<json>::iterator array_begin() { 
            if (m_type != value_t::array) throw std::runtime_error("not an array");
            return m_value.array_value->begin();
        }
        std::vector<json>::iterator array_end() { 
            if (m_type != value_t::array) throw std::runtime_error("not an array");
            return m_value.array_value->end();
        }
        std::vector<json>::const_iterator array_begin() const { 
            if (m_type != value_t::array) throw std::runtime_error("not an array");
            return m_value.array_value->begin();
        }
        std::vector<json>::const_iterator array_end() const { 
            if (m_type != value_t::array) throw std::runtime_error("not an array");
            return m_value.array_value->end();
        }
        
        // Object iterators
        std::map<std::string, json>::iterator object_begin() { 
            if (m_type != value_t::object) throw std::runtime_error("not an object");
            return m_value.object_value->begin();
        }
        std::map<std::string, json>::iterator object_end() { 
            if (m_type != value_t::object) throw std::runtime_error("not an object");
            return m_value.object_value->end();
        }
        std::map<std::string, json>::const_iterator object_begin() const { 
            if (m_type != value_t::object) throw std::runtime_error("not an object");
            return m_value.object_value->begin();
        }
        std::map<std::string, json>::const_iterator object_end() const { 
            if (m_type != value_t::object) throw std::runtime_error("not an object");
            return m_value.object_value->end();
        }
        
        // Generic begin/end for range-based for loops
        // For arrays
        std::vector<json>::iterator begin() { 
            if (m_type != value_t::array) throw std::runtime_error("begin() called on non-array json");
            return array_begin();
        }
        std::vector<json>::iterator end() { 
            if (m_type != value_t::array) throw std::runtime_error("end() called on non-array json");
            return array_end();
        }
        std::vector<json>::const_iterator begin() const { 
            if (m_type != value_t::array) throw std::runtime_error("begin() called on non-array json");
            return array_begin();
        }
        std::vector<json>::const_iterator end() const { 
            if (m_type != value_t::array) throw std::runtime_error("end() called on non-array json");
            return array_end();
        }
        
        // Parse from string
        static json parse(const std::string& str) {
            json result;
            std::string s = str;
            s.erase(0, s.find_first_not_of(" \t\n\r"));
            s.erase(s.find_last_not_of(" \t\n\r") + 1);
            
            if (s.empty()) {
                return result;
            }
            
            if (s == "null") {
                return json();
            } else if (s == "true") {
                return json(true);
            } else if (s == "false") {
                return json(false);
            } else if (s[0] == '"') {
                return json(s.substr(1, s.size() - 2));
            } else if (s[0] == '{') {
                result.m_type = value_t::object;
                result.m_value.object_value = new std::map<std::string, json>();
            } else if (s[0] == '[') {
                result.m_type = value_t::array;
                result.m_value.array_value = new std::vector<json>();
            } else {
                try {
                    if (s.find('.') != std::string::npos || s.find('e') != std::string::npos || s.find('E') != std::string::npos) {
                        return json(std::stod(s));
                    } else {
                        return json(std::stoi(s));
                    }
                } catch (...) {
                    return json();
                }
            }
            return result;
        }
        
        // Dump to string
        std::string dump(int indent = -1) const {
            if (indent < 0) {
                return dump_value();
            }
            return dump_with_indent(0, indent);
        }
        
        // Create empty object
        static json object() {
            json result;
            result.m_type = value_t::object;
            result.m_value.object_value = new std::map<std::string, json>();
            return result;
        }
    private:
        union Value {
            bool bool_value;
            int number_integer_value;
            unsigned int number_unsigned_value;
            double number_float_value;
            std::string* string_value;
            std::vector<json>* array_value;
            std::map<std::string, json>* object_value;
            Value() : bool_value(false) {}
            ~Value() {}
        };
        
        value_t m_type;
        Value m_value;
        
        void copy_value(const json& other) {
            switch (other.m_type) {
                case value_t::boolean:
                    m_value.bool_value = other.m_value.bool_value;
                    break;
                case value_t::number_integer:
                    m_value.number_integer_value = other.m_value.number_integer_value;
                    break;
                case value_t::number_unsigned:
                    m_value.number_unsigned_value = other.m_value.number_unsigned_value;
                    break;
                case value_t::number_float:
                    m_value.number_float_value = other.m_value.number_float_value;
                    break;
                case value_t::string:
                    m_value.string_value = new std::string(*other.m_value.string_value);
                    break;
                case value_t::array:
                    m_value.array_value = new std::vector<json>(*other.m_value.array_value);
                    break;
                case value_t::object:
                    m_value.object_value = new std::map<std::string, json>(*other.m_value.object_value);
                    break;
                default:
                    break;
            }
        }
        
        void move_value(json&& other) {
            m_value = other.m_value;
            other.m_value.string_value = nullptr;
        }
        
        void clear() {
            switch (m_type) {
                case value_t::string:
                    delete m_value.string_value;
                    break;
                case value_t::array:
                    delete m_value.array_value;
                    break;
                case value_t::object:
                    delete m_value.object_value;
                    break;
                default:
                    break;
            }
            m_type = value_t::null;
            m_value.string_value = nullptr;
        }
        
        std::string dump_value() const {
            switch (m_type) {
                case value_t::null:
                    return "null";
                case value_t::boolean:
                    return m_value.bool_value ? "true" : "false";
                case value_t::number_integer:
                    return std::to_string(m_value.number_integer_value);
                case value_t::number_unsigned:
                    return std::to_string(m_value.number_unsigned_value);
                case value_t::number_float:
                    return std::to_string(m_value.number_float_value);
                case value_t::string:
                    return "\"" + *m_value.string_value + "\"";
                case value_t::array: {
                    std::string result = "[";
                    if (m_value.array_value) {
                        for (size_t i = 0; i < m_value.array_value->size(); ++i) {
                            if (i > 0) result += ",";
                            result += (*m_value.array_value)[i].dump_value();
                        }
                    }
                    return result + "]";
                }
                case value_t::object: {
                    std::string result = "{";
                    bool first = true;
                    if (m_value.object_value) {
                        for (const auto& pair : *m_value.object_value) {
                            if (!first) result += ",";
                            first = false;
                            result += "\"" + pair.first + "\":" + pair.second.dump_value();
                        }
                    }
                    return result + "}";
                }
                default:
                    return "null";
            }
        }
        
        std::string dump_with_indent(size_t current_indent, int indent) const {
            std::string indent_str(current_indent, ' ');
            std::string child_indent_str(current_indent + indent, ' ');
            
            switch (m_type) {
                case value_t::null:
                    return "null";
                case value_t::boolean:
                    return m_value.bool_value ? "true" : "false";
                case value_t::number_integer:
                    return std::to_string(m_value.number_integer_value);
                case value_t::number_unsigned:
                    return std::to_string(m_value.number_unsigned_value);
                case value_t::number_float:
                    return std::to_string(m_value.number_float_value);
                case value_t::string:
                    return "\"" + *m_value.string_value + "\"";
                case value_t::array: {
                    if (!m_value.array_value || m_value.array_value->empty()) {
                        return "[]";
                    }
                    std::string result = "[\n";
                    for (const auto& item : *m_value.array_value) {
                        result += child_indent_str + item.dump_with_indent(current_indent + indent, indent) + ",\n";
                    }
                    result.erase(result.size() - 2);
                    return result + "\n" + indent_str + "]";
                }
                case value_t::object: {
                    if (!m_value.object_value || m_value.object_value->empty()) {
                        return "{}";
                    }
                    std::string result = "{\n";
                    for (const auto& pair : *m_value.object_value) {
                        result += child_indent_str + "\"" + pair.first + "\": " + pair.second.dump_with_indent(current_indent + indent, indent) + ",\n";
                    }
                    result.erase(result.size() - 2);
                    return result + "\n" + indent_str + "}";
                }
                default:
                    return "null";
            }
        }
    };
    
    // Template specializations for get()
    template<> inline bool json::get<bool>() const { return get_bool(); }
    template<> inline int json::get<int>() const { return get_int(); }
    template<> inline unsigned int json::get<unsigned int>() const { return get_uint(); }
    template<> inline double json::get<double>() const { return get_double(); }
    template<> inline float json::get<float>() const { return get_float(); }
    template<> inline std::string json::get<std::string>() const { return get_string(); }
    
} // namespace nlohmann

// Alias for convenience
using json = nlohmann::json;
