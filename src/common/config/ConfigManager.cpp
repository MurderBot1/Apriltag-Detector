#include "ConfigManager.hpp"
#include "LimelightFMapLoader.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/FileSystem.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
    // Helper to convert string to lowercase
    std::string toLower(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            c = static_cast<char>(tolower(c));
        }
        return result;
    }
}

ConfigManager::SystemConfig ConfigManager::loadSystemConfig(const std::string& path) {
    SystemConfig config;
    
    // Default values
    config.version = "1.0";
    config.logLevel = "INFO";
    config.logDirectory = "./logs";
    config.numCameras = 0;
    config.nt4.robotAddress = "10.0.0.2";
    config.nt4.teamNumber = 0;
    config.nt4.publishRate = 30;
    config.web.port = 8080;
    config.web.bindAddress = "0.0.0.0";
    config.web.enableAuth = false;
    config.selfHealing.enabled = true;
    config.selfHealing.restartDelay = 1000;
    config.selfHealing.maxAttempts = 5;
    config.fmap.defaultField = "FRC_2025_Reefscape";
    config.fmap.fmapDirectory = "config/fmaps";
    
    std::string content = FileSystem::readFile(path);
    if (content.empty()) {
        // File doesn't exist, return defaults
        return config;
    }
    
    try {
        json j = json::parse(content);
        
        if (j.contains("version")) {
            config.version = j["version"].get<std::string>();
        }
        
        if (j.contains("log_level")) {
            config.logLevel = j["log_level"].get<std::string>();
        }
        
        if (j.contains("log_directory")) {
            config.logDirectory = j["log_directory"].get<std::string>();
        }
        
        if (j.contains("num_cameras")) {
            config.numCameras = j["num_cameras"].get<int>();
        }
        
        if (j.contains("nt4")) {
            json nt4 = j["nt4"];
            if (nt4.contains("robot_address")) {
                config.nt4.robotAddress = nt4["robot_address"].get<std::string>();
            }
            if (nt4.contains("team_number")) {
                config.nt4.teamNumber = nt4["team_number"].get<int>();
            }
            if (nt4.contains("publish_rate")) {
                config.nt4.publishRate = nt4["publish_rate"].get<int>();
            }
        }
        
        if (j.contains("web")) {
            json web = j["web"];
            if (web.contains("port")) {
                config.web.port = web["port"].get<int>();
            }
            if (web.contains("bind_address")) {
                config.web.bindAddress = web["bind_address"].get<std::string>();
            }
            if (web.contains("enable_auth")) {
                config.web.enableAuth = web["enable_auth"].get<bool>();
            }
        }
        
        if (j.contains("self_healing")) {
            json healing = j["self_healing"];
            if (healing.contains("enabled")) {
                config.selfHealing.enabled = healing["enabled"].get<bool>();
            }
            if (healing.contains("restart_delay")) {
                config.selfHealing.restartDelay = healing["restart_delay"].get<int>();
            }
            if (healing.contains("max_attempts")) {
                config.selfHealing.maxAttempts = healing["max_attempts"].get<int>();
            }
        }
        
        if (j.contains("fmap")) {
            json fmap = j["fmap"];
            if (fmap.contains("default_field")) {
                config.fmap.defaultField = fmap["default_field"].get<std::string>();
            }
            if (fmap.contains("fmap_directory")) {
                config.fmap.fmapDirectory = fmap["fmap_directory"].get<std::string>();
            }
        }
        
    } catch (const json::exception& e) {
        // Parse error, return defaults
    } catch (...) {
        // Other error, return defaults
    }
    
    return config;
}

ConfigManager::CameraConfig ConfigManager::loadCameraConfig(int cameraId) {
    CameraConfig config;
    
    // Default values
    config.cameraId = cameraId;
    config.devicePath = "/dev/video" + std::to_string(cameraId);
    config.cameraType = "v4l2";
    config.resolution.width = 1280;
    config.resolution.height = 720;
    config.fps = 30;
    config.settings.exposure = 50;
    config.settings.brightness = 50;
    config.settings.contrast = 50;
    config.settings.saturation = 50;
    config.settings.gain = 0;
    config.intrinsics.fx = 800.0;
    config.intrinsics.fy = 800.0;
    config.intrinsics.cx = 640.0;
    config.intrinsics.cy = 360.0;
    config.intrinsics.distortion = {0.0, 0.0, 0.0, 0.0, 0.0};
    config.extrinsics.translation = {0.0, 0.0, 0.0};
    config.extrinsics.rotation = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    
    std::string path = getCameraConfigPath(cameraId);
    std::string content = FileSystem::readFile(path);
    if (content.empty()) {
        // Save defaults
        saveCameraConfig(config, path);
        return config;
    }
    
    try {
        json j = json::parse(content);
        
        if (j.contains("camera_id")) {
            config.cameraId = j["camera_id"].get<int>();
        }
        
        if (j.contains("device_path")) {
            config.devicePath = j["device_path"].get<std::string>();
        }
        
        if (j.contains("camera_type")) {
            config.cameraType = j["camera_type"].get<std::string>();
        }
        
        if (j.contains("resolution")) {
            json res = j["resolution"];
            if (res.contains("width")) {
                config.resolution.width = res["width"].get<int>();
            }
            if (res.contains("height")) {
                config.resolution.height = res["height"].get<int>();
            }
        }
        
        if (j.contains("fps")) {
            config.fps = j["fps"].get<int>();
        }
        
        if (j.contains("settings")) {
            json settings = j["settings"];
            if (settings.contains("exposure")) {
                config.settings.exposure = settings["exposure"].get<int>();
            }
            if (settings.contains("brightness")) {
                config.settings.brightness = settings["brightness"].get<int>();
            }
            if (settings.contains("contrast")) {
                config.settings.contrast = settings["contrast"].get<int>();
            }
            if (settings.contains("saturation")) {
                config.settings.saturation = settings["saturation"].get<int>();
            }
            if (settings.contains("gain")) {
                config.settings.gain = settings["gain"].get<int>();
            }
        }
        
        if (j.contains("intrinsics")) {
            json intrinsics = j["intrinsics"];
            if (intrinsics.contains("fx")) {
                config.intrinsics.fx = intrinsics["fx"].get<double>();
            }
            if (intrinsics.contains("fy")) {
                config.intrinsics.fy = intrinsics["fy"].get<double>();
            }
            if (intrinsics.contains("cx")) {
                config.intrinsics.cx = intrinsics["cx"].get<double>();
            }
            if (intrinsics.contains("cy")) {
                config.intrinsics.cy = intrinsics["cy"].get<double>();
            }
            if (intrinsics.contains("distortion")) {
                config.intrinsics.distortion = intrinsics["distortion"].get<std::vector<double>>();
            }
        }
        
        if (j.contains("extrinsics")) {
            json extrinsics = j["extrinsics"];
            if (extrinsics.contains("translation")) {
                config.extrinsics.translation = extrinsics["translation"].get<std::vector<double>>();
            }
            if (extrinsics.contains("rotation")) {
                config.extrinsics.rotation = extrinsics["rotation"].get<std::vector<double>>();
            }
        }
        
    } catch (const json::exception& e) {
        // Parse error, return defaults
    } catch (...) {
        // Other error, return defaults
    }
    
    return config;
}

std::map<int, ConfigManager::CameraConfig> ConfigManager::loadAllCameraConfigs() {
    std::map<int, CameraConfig> configs;
    
    // Try to load cameras from 0 to 9
    for (int i = 0; i < 10; ++i) {
        std::string path = getCameraConfigPath(i);
        if (FileSystem::exists(path)) {
            configs[i] = loadCameraConfig(i);
        }
    }
    
    return configs;
}

std::map<int, ConfigManager::TagConfig> ConfigManager::loadTagConfig(const std::string& path) {
    std::map<int, TagConfig> tags;
    
    std::string content = FileSystem::readFile(path);
    if (content.empty()) {
        return tags;
    }
    
    try {
        json j = json::parse(content);
        
        if (j.contains("tags")) {
            json tagsArray = j["tags"];
            for (const auto& tagJson : tagsArray) {
                TagConfig tag;
                
                if (tagJson.contains("id")) {
                    tag.id = tagJson["id"].get<int>();
                }
                
                if (tagJson.contains("size")) {
                    tag.size = tagJson["size"].get<double>();
                }
                
                if (tagJson.contains("pose")) {
                    json pose = tagJson["pose"];
                    if (pose.contains("translation")) {
                        tag.pose.translation = pose["translation"].get<std::vector<double>>();
                    }
                    if (pose.contains("rotation")) {
                        tag.pose.rotation = pose["rotation"].get<std::vector<double>>();
                    }
                }
                
                tags[tag.id] = tag;
            }
        }
        
    } catch (const json::exception& e) {
        // Parse error
    } catch (...) {
        // Other error
    }
    
    return tags;
}

std::map<int, ConfigManager::TagConfig> ConfigManager::loadFMapConfig(const std::string& path) {
    // Load from Limelight FMap format
    LimelightFMapLoader::FieldMap fieldMap = LimelightFMapLoader::loadFMap(path);
    return LimelightFMapLoader::toTagConfig(fieldMap);
}

void ConfigManager::saveSystemConfig(const SystemConfig& config, const std::string& path) {
    json j;
    
    j["version"] = config.version;
    j["log_level"] = config.logLevel;
    j["log_directory"] = config.logDirectory;
    j["num_cameras"] = config.numCameras;
    
    json nt4;
    nt4["robot_address"] = config.nt4.robotAddress;
    nt4["team_number"] = config.nt4.teamNumber;
    nt4["publish_rate"] = config.nt4.publishRate;
    j["nt4"] = nt4;
    
    json web;
    web["port"] = config.web.port;
    web["bind_address"] = config.web.bindAddress;
    web["enable_auth"] = config.web.enableAuth;
    j["web"] = web;
    
    json healing;
    healing["enabled"] = config.selfHealing.enabled;
    healing["restart_delay"] = config.selfHealing.restartDelay;
    healing["max_attempts"] = config.selfHealing.maxAttempts;
    j["self_healing"] = healing;
    
    json fmap;
    fmap["default_field"] = config.fmap.defaultField;
    fmap["fmap_directory"] = config.fmap.fmapDirectory;
    j["fmap"] = fmap;
    
    FileSystem::writeFile(path, j.dump(4));
}

void ConfigManager::saveCameraConfig(const CameraConfig& config, const std::string& path) {
    std::string actualPath = path.empty() ? getCameraConfigPath(config.cameraId) : path;
    
    json j;
    
    j["camera_id"] = config.cameraId;
    j["device_path"] = config.devicePath;
    j["camera_type"] = config.cameraType;
    
    json resolution;
    resolution["width"] = config.resolution.width;
    resolution["height"] = config.resolution.height;
    j["resolution"] = resolution;
    
    j["fps"] = config.fps;
    
    json settings;
    settings["exposure"] = config.settings.exposure;
    settings["brightness"] = config.settings.brightness;
    settings["contrast"] = config.settings.contrast;
    settings["saturation"] = config.settings.saturation;
    settings["gain"] = config.settings.gain;
    j["settings"] = settings;
    
    json intrinsics;
    intrinsics["fx"] = config.intrinsics.fx;
    intrinsics["fy"] = config.intrinsics.fy;
    intrinsics["cx"] = config.intrinsics.cx;
    intrinsics["cy"] = config.intrinsics.cy;
    intrinsics["distortion"] = config.intrinsics.distortion;
    j["intrinsics"] = intrinsics;
    
    json extrinsics;
    extrinsics["translation"] = config.extrinsics.translation;
    extrinsics["rotation"] = config.extrinsics.rotation;
    j["extrinsics"] = extrinsics;
    
    FileSystem::writeFile(actualPath, j.dump(4));
}

bool ConfigManager::validateSystemConfig(const SystemConfig& config) {
    if (config.nt4.publishRate <= 0) return false;
    if (config.web.port < 0 || config.web.port > 65535) return false;
    if (config.selfHealing.restartDelay < 0) return false;
    if (config.selfHealing.maxAttempts < 0) return false;
    return true;
}

bool ConfigManager::validateCameraConfig(const CameraConfig& config) {
    if (config.resolution.width <= 0 || config.resolution.height <= 0) return false;
    if (config.fps <= 0) return false;
    return true;
}

std::string ConfigManager::getCameraConfigPath(int cameraId) {
    return "config/cameras/camera_" + std::to_string(cameraId) + ".json";
}
