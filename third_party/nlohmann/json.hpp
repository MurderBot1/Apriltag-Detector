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

namespace nlohmann {
    
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
        json() noexcept : m_type(value_t::null), m_value(nullptr) {}
        json(std::nullptr_t) noexcept : json() {}
        json(bool value) : m_type(value_t::boolean) { m_value.bool_value = value; }
        json(int value) : m_type(value_t::number_integer) { m_value.number_integer_value = value; }
        json(unsigned int value) : m_type(value_t::number_unsigned) { m_value.number_unsigned_value = value; }
        json(long value) : m_type(value_t::number_integer) { m_value.number_integer_value = value; }
        json(unsigned long value) : m_type(value_t::number_unsigned) { m_value.number_unsigned_value = value; }
        json(long long value) : m_type(value_t::number_integer) { m_value.number_integer_value = static_cast<int>(value); }
        json(unsigned long long value) : m_type(value_t::number_unsigned) { m_value.number_unsigned_value = static_cast<unsigned int>(value); }
        json(float value) : m_type(value_t::number_float) { m_value.number_float_value = value; }
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
        json(json&& other) noexcept : m_type(other.m_type), m_value(other.m_value) {
            other.m_type = value_t::null;
            other.m_value = nullptr;
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
                m_value = other.m_value;
                other.m_type = value_t::null;
                other.m_value = nullptr;
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
        bool get<bool>() const { return m_value.bool_value; }
        int get<int>() const { return static_cast<int>(m_value.number_integer_value); }
        unsigned int get<unsigned int>() const { return m_value.number_unsigned_value; }
        double get<double>() const { return m_value.number_float_value; }
        float get<float>() const { return static_cast<float>(m_value.number_float_value); }
        std::string get<std::string>() const { return *m_value.string_value; }
        
        // Array access
        json& operator[](size_t index) {
            if (m_type != value_t::array) {
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
                m_type = value_t::object;
                m_value.object_value = new std::map<std::string, json>();
            }
            return (*m_value.object_value)[key];
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
                return m_value.array_value->size();
            } else if (m_type == value_t::object) {
                return m_value.object_value->size();
            }
            return 0;
        }
        
        // Iterators
        auto begin() { 
            if (m_type == value_t::array) return m_value.array_value->begin();
            if (m_type == value_t::object) return m_value.object_value->begin();
            return std::vector<json>().begin(); 
        }
        auto end() { 
            if (m_type == value_t::array) return m_value.array_value->end();
            if (m_type == value_t::object) return m_value.object_value->end();
            return std::vector<json>().end(); 
        }
        
        // Parse from string
        static json parse(const std::string& str) {
            json result;
            // This is a simplified parser - real implementation would be more robust
            // For now, we'll just handle basic JSON structures
            
            // Trim whitespace
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
                // String
                return json(s.substr(1, s.size() - 2));
            } else if (s[0] == '{') {
                // Object
                result.m_type = value_t::object;
                result.m_value.object_value = new std::map<std::string, json>();
                // Simplified parsing - would need full parser for real use
            } else if (s[0] == '[') {
                // Array
                result.m_type = value_t::array;
                result.m_value.array_value = new std::vector<json>();
                // Simplified parsing
            } else {
                // Number
                try {
                    if (s.find('.') != std::string::npos || s.find('e') != std::string::npos) {
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
                    return '"' + *m_value.string_value + '"';
                case value_t::array: {
                    std::string result = "[";
                    for (size_t i = 0; i < m_value.array_value->size(); ++i) {
                        if (i > 0) result += ",";
                        result += (*m_value.array_value)[i].dump_value();
                    }
                    return result + "]";
                }
                case value_t::object: {
                    std::string result = "{";
                    bool first = true;
                    for (const auto& pair : *m_value.object_value) {
                        if (!first) result += ",";
                        first = false;
                        result += '"' + pair.first + '":' + pair.second.dump_value();
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
                    return '"' + *m_value.string_value + '"';
                case value_t::array: {
                    if (m_value.array_value->empty()) {
                        return "[]";
                    }
                    std::string result = "[\n";
                    for (const auto& item : *m_value.array_value) {
                        result += child_indent_str + item.dump_with_indent(current_indent + indent, indent) + ",\n";
                    }
                    result.erase(result.size() - 2); // Remove trailing comma and newline
                    return result + "\n" + indent_str + "]";
                }
                case value_t::object: {
                    if (m_value.object_value->empty()) {
                        return "{}";
                    }
                    std::string result = "{\n";
                    for (const auto& pair : *m_value.object_value) {
                        result += child_indent_str + '"' + pair.first + '": ' + pair.second.dump_with_indent(current_indent + indent, indent) + ",\n";
                    }
                    result.erase(result.size() - 2); // Remove trailing comma and newline
                    return result + "\n" + indent_str + "}";
                }
                default:
                    return "null";
            }
        }
    };
    
    // Exception class
    class json_exception : public std::runtime_error {
    public:
        explicit json_exception(const std::string& what) : std::runtime_error(what) {}
    };
    
} // namespace nlohmann

// Alias for convenience
using json = nlohmann::json;
