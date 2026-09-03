#include "GlobalConfig.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/Platform.hpp"
#include <nlohmann/json.hpp>
#include <vector>
#include <mutex>

using json = nlohmann::json;

namespace GlobalConfig {
    
    // Global configuration variables
    std::string TAG_FAMILY = DEFAULT_TAG_FAMILY;
    double TAG_SIZE = DEFAULT_TAG_SIZE;
    double MIN_TAG_SIZE = DEFAULT_MIN_TAG_SIZE;
    double MAX_TAG_SIZE = DEFAULT_MAX_TAG_SIZE;
    double DETECT_THRESHOLD = DEFAULT_DETECT_THRESHOLD;
    
    // Configuration file path
    static std::string s_configPath = "config/system.json";
    
    // Change callbacks
    static std::vector<ConfigChangeCallback> s_changeCallbacks;
    static std::mutex s_callbackMutex;
    
    void initialize(const std::string& configPath) {
        s_configPath = configPath;
        reload();
    }
    
    void reload() {
        std::string content = FileSystem::readFile(s_configPath);
        if (content.empty()) {
            // Use defaults if file doesn't exist
            return;
        }
        
        try {
            json config = json::parse(content);
            
            if (config.contains("globals")) {
                json globals = config["globals"];
                
                if (globals.contains("tag_family")) {
                    std::string newFamily = globals["tag_family"].get<std::string>();
                    if (newFamily != TAG_FAMILY) {
                        TAG_FAMILY = newFamily;
                    }
                }
                
                if (globals.contains("default_tag_size")) {
                    double newSize = globals["default_tag_size"].get<double>();
                    if (newSize != TAG_SIZE) {
                        TAG_SIZE = newSize;
                    }
                }
                
                if (globals.contains("min_tag_size")) {
                    double newMin = globals["min_tag_size"].get<double>();
                    if (newMin != MIN_TAG_SIZE) {
                        MIN_TAG_SIZE = newMin;
                    }
                }
                
                if (globals.contains("max_tag_size")) {
                    double newMax = globals["max_tag_size"].get<double>();
                    if (newMax != MAX_TAG_SIZE) {
                        MAX_TAG_SIZE = newMax;
                    }
                }
                
                if (globals.contains("detect_threshold")) {
                    double newThreshold = globals["detect_threshold"].get<double>();
                    if (newThreshold != DETECT_THRESHOLD) {
                        DETECT_THRESHOLD = newThreshold;
                    }
                }
            }
            
            // Notify callbacks
            notifyCallbacks();
            
        } catch (const json::exception& e) {
            // Parse error - keep current values
            // In production, log this error
        } catch (...) {
            // Other error - keep current values
        }
    }
    
    std::string toJson() const {
        json result;
        result["tag_family"] = TAG_FAMILY;
        result["default_tag_size"] = TAG_SIZE;
        result["min_tag_size"] = MIN_TAG_SIZE;
        result["max_tag_size"] = MAX_TAG_SIZE;
        result["detect_threshold"] = DETECT_THRESHOLD;
        return result.dump(4);
    }
    
    void fromJson(const std::string& jsonStr) {
        try {
            json config = json::parse(jsonStr);
            
            if (config.contains("tag_family")) {
                TAG_FAMILY = config["tag_family"].get<std::string>();
            }
            
            if (config.contains("default_tag_size")) {
                TAG_SIZE = config["default_tag_size"].get<double>();
            }
            
            if (config.contains("min_tag_size")) {
                MIN_TAG_SIZE = config["min_tag_size"].get<double>();
            }
            
            if (config.contains("max_tag_size")) {
                MAX_TAG_SIZE = config["max_tag_size"].get<double>();
            }
            
            if (config.contains("detect_threshold")) {
                DETECT_THRESHOLD = config["detect_threshold"].get<double>();
            }
            
            // Save to file
            std::string currentContent = FileSystem::readFile(s_configPath);
            json fullConfig;
            
            if (!currentContent.empty()) {
                try {
                    fullConfig = json::parse(currentContent);
                } catch (...) {
                    fullConfig = json::object();
                }
            }
            
            fullConfig["globals"] = config;
            FileSystem::writeFile(s_configPath, fullConfig.dump(4));
            
            // Notify callbacks
            notifyCallbacks();
            
        } catch (const json::exception& e) {
            // Parse error
        } catch (...) {
            // Other error
        }
    }
    
    void registerChangeCallback(ConfigChangeCallback callback) {
        std::lock_guard<std::mutex> lock(s_callbackMutex);
        s_changeCallbacks.push_back(callback);
    }
    
    void unregisterChangeCallback(ConfigChangeCallback callback) {
        std::lock_guard<std::mutex> lock(s_callbackMutex);
        auto it = std::find(s_changeCallbacks.begin(), s_changeCallbacks.end(), callback);
        if (it != s_changeCallbacks.end()) {
            s_changeCallbacks.erase(it);
        }
    }
    
    const std::string& getConfigPath() {
        return s_configPath;
    }
    
    void setConfigPath(const std::string& path) {
        s_configPath = path;
    }
    
    void notifyCallbacks() {
        std::lock_guard<std::mutex> lock(s_callbackMutex);
        for (auto& callback : s_changeCallbacks) {
            try {
                callback();
            } catch (...) {
                // Don't let callback errors affect others
            }
        }
    }
    
} // namespace GlobalConfig
