#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "GlobalConfig.hpp"

/**
 * @brief Configuration manager for loading and managing system configuration
 */
class ConfigManager {
public:
    
    /**
     * @brief System-wide configuration structure
     */
    struct SystemConfig {
        std::string version;
        std::string logLevel;
        std::string logDirectory;
        int numCameras;
        
        struct NT4Config {
            std::string robotAddress;
            int teamNumber;
            int publishRate;
        } nt4;
        
        struct WebConfig {
            int port;
            std::string bindAddress;
            bool enableAuth;
        } web;
        
        struct SelfHealingConfig {
            bool enabled;
            int restartDelay;
            int maxAttempts;
        } selfHealing;
        
        struct FMapConfig {
            std::string defaultField;
            std::string fmapDirectory;
        } fmap;
    };
    
    /**
     * @brief Camera configuration structure
     */
    struct CameraConfig {
        int cameraId;
        std::string devicePath;
        std::string cameraType;  // "v4l2", "wmf", "avfoundation"
        
        struct Resolution {
            int width;
            int height;
        } resolution;
        
        int fps;
        
        struct CameraSettings {
            int exposure;
            int brightness;
            int contrast;
            int saturation;
            int gain;
        } settings;
        
        struct Intrinsics {
            double fx, fy;
            double cx, cy;
            std::vector<double> distortion;  // 5 coefficients
        } intrinsics;
        
        struct Extrinsics {
            std::vector<double> translation;  // [x, y, z]
            std::vector<double> rotation;    // 3x3 matrix
        } extrinsics;
    };
    
    /**
     * @brief Tag configuration structure
     */
    struct TagConfig {
        int id;
        struct Pose {
            std::vector<double> translation;  // [x, y, z]
            std::vector<double> rotation;    // quaternion [w, x, y, z]
        } pose;
        double size;  // Tag size in meters
    };
    
    /**
     * @brief Load system configuration from file
     * @param path Path to system.json
     * @return SystemConfig structure
     */
    static SystemConfig loadSystemConfig(const std::string& path = "config/system.json");
    
    /**
     * @brief Load camera configuration from file
     * @param cameraId Camera ID
     * @return CameraConfig structure
     */
    static CameraConfig loadCameraConfig(int cameraId);
    
    /**
     * @brief Load all camera configurations
     * @return Map of camera ID to CameraConfig
     */
    static std::map<int, CameraConfig> loadAllCameraConfigs();
    
    /**
     * @brief Load tag configuration from file
     * @param path Path to tags.json
     * @return Map of tag ID to TagConfig
     */
    static std::map<int, TagConfig> loadTagConfig(const std::string& path = "config/tags.json");
    
    /**
     * @brief Load tag configuration from Limelight FMap
     * @param path Path to FMap JSON file
     * @return Map of tag ID to TagConfig
     */
    static std::map<int, TagConfig> loadFMapConfig(const std::string& path);
    
    /**
     * @brief Save system configuration to file
     * @param config SystemConfig to save
     * @param path Path to save to
     */
    static void saveSystemConfig(const SystemConfig& config, const std::string& path = "config/system.json");
    
    /**
     * @brief Save camera configuration to file
     * @param config CameraConfig to save
     * @param path Path to save to
     */
    static void saveCameraConfig(const CameraConfig& config, const std::string& path = "");
    
    /**
     * @brief Validate system configuration
     * @param config SystemConfig to validate
     * @return true if valid, false otherwise
     */
    static bool validateSystemConfig(const SystemConfig& config);
    
    /**
     * @brief Validate camera configuration
     * @param config CameraConfig to validate
     * @return true if valid, false otherwise
     */
    static bool validateCameraConfig(const CameraConfig& config);
    
    /**
     * @brief Get the path to a camera configuration file
     * @param cameraId Camera ID
     * @return Path to camera_<id>.json
     */
    static std::string getCameraConfigPath(int cameraId);
    
private:
    ConfigManager() = default;
    ~ConfigManager() = default;
};

/**
 * @brief Tags configuration for loading from tags.json
 */
struct TagsConfig {
    struct TagInfo {
        int id;
        std::vector<double> translation;  // [x, y, z]
        std::vector<double> rotation;    // quaternion [w, x, y, z]
        double size;
    };
    
    std::vector<TagInfo> tags;
};
