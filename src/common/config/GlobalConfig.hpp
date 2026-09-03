#pragma once

#include <string>
#include <functional>

/**
 * @brief Global configuration constants that can be changed without redeployment
 * 
 * All components use these global constants for tag detection parameters.
 * Changes to these values take effect immediately across all components.
 */
namespace GlobalConfig {
    
    // AprilTag family - configurable without redeployment
    extern std::string TAG_FAMILY;
    
    // Default values
    constexpr const char* DEFAULT_TAG_FAMILY = "tag36h11";
    
    // Tag size in meters (can be overridden per-tag in known tags config)
    extern double TAG_SIZE;
    constexpr double DEFAULT_TAG_SIZE = 0.1651;
    
    // Detection parameters - all configurable at runtime
    extern double MIN_TAG_SIZE;
    constexpr double DEFAULT_MIN_TAG_SIZE = 0.05;
    
    extern double MAX_TAG_SIZE;
    constexpr double DEFAULT_MAX_TAG_SIZE = 10.0;
    
    extern double DETECT_THRESHOLD;
    constexpr double DEFAULT_DETECT_THRESHOLD = 0.5;
    
    // Callback for configuration changes
    using ConfigChangeCallback = std::function<void()>;
    
    /**
     * @brief Initialize global configuration from system.json
     * @param configPath Path to system.json configuration file
     */
    void initialize(const std::string& configPath);
    
    /**
     * @brief Reload configuration from file
     * Called automatically when config file changes, or manually to refresh
     */
    void reload();
    
    /**
     * @brief Get current configuration as JSON string
     * @return JSON string representation of global configuration
     */
    std::string toJson();
    
    /**
     * @brief Update configuration from JSON string
     * @param json JSON string with new configuration values
     */
    void fromJson(const std::string& json);
    
    /**
     * @brief Register a callback to be notified of configuration changes
     * @param callback Function to call when configuration changes
     */
    void registerChangeCallback(ConfigChangeCallback callback);
    
    /**
     * @brief Unregister a configuration change callback
     * @param callback The callback to remove
     */
    void unregisterChangeCallback(ConfigChangeCallback callback);
    
    /**
     * @brief Get the path to the configuration file
     * @return Path to the system.json file
     */
    const std::string& getConfigPath();
    
    /**
     * @brief Set the path to the configuration file
     * @param path New configuration file path
     */
    void setConfigPath(const std::string& path);

    /**
     * @brief Notify all registered callbacks of configuration changes
     */
    void notifyCallbacks();
    
} // namespace GlobalConfig
