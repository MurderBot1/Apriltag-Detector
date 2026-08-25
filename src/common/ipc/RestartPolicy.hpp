#pragma once

#include <cstdint>

/**
 * @brief Restart policy constants
 */
namespace RestartPolicy {
    
    enum Type {
        NONE = 0,        // Never restart
        ALWAYS = 1,     // Always restart when process exits
        ON_FAILURE = 2   // Restart only on non-zero exit code
    };
    
    /**
     * @brief Restart policy configuration
     */
    struct Config {
        Type type;
        int maxAttempts;
        int delayMs;
        
        Config() : type(ON_FAILURE), maxAttempts(5), delayMs(1000) {}
        Config(Type t, int max, int delay) 
            : type(t), maxAttempts(max), delayMs(delay) {}
    };
    
    /**
     * @brief Convert restart policy type to string
     */
    const char* toString(Type type);
    
    /**
     * @brief Parse restart policy type from string
     */
    Type fromString(const std::string& str);
    
} // namespace RestartPolicy
