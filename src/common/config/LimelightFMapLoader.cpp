#include "LimelightFMapLoader.hpp"
#include "../utils/FileSystem.hpp"
#include "../logging/Logger.hpp"
#include <nlohmann/json.hpp>
#include <cmath>

using json = nlohmann::json;

LimelightFMapLoader::LimelightFMapLoader() {
}

LimelightFMapLoader::~LimelightFMapLoader() {
}

LimelightFMapLoader::FieldMap LimelightFMapLoader::loadFMap(const std::string& path) {
    std::string content = FileSystem::readFile(path);
    if (content.empty()) {
        LOG_ERROR_F("Failed to read FMap file: %s", path.c_str());
        return FieldMap();
    }
    
    return loadFMapFromString(content);
}

LimelightFMapLoader::FieldMap LimelightFMapLoader::loadFMapFromString(const std::string& jsonString) {
    FieldMap fieldMap;
    
    try {
        json j = json::parse(jsonString);
        
        // Parse field map name and description
        if (j.contains("name")) {
            fieldMap.name = j["name"].get<std::string>();
        }
        if (j.contains("description")) {
            fieldMap.description = j["description"].get<std::string>();
        }
        
        // Parse field size if available
        if (j.contains("field_size")) {
            json sizeJson = j["field_size"];
            if (sizeJson.is_array() && sizeJson.size() >= 2) {
                fieldMap.fieldSize = {sizeJson[0].get<double>(), sizeJson[1].get<double>()};
            }
        }
        
        // Parse tags array
        if (j.contains("tags")) {
            json tagsArray = j["tags"];
            for (const auto& tagJson : tagsArray) {
                FMapTag tag = parseTag(tagJson);
                if (tag.id >= 0) {
                    fieldMap.tags[tag.id] = tag;
                }
            }
        }
        
        // Alternative format: tags as object with ID keys
        if (j.contains("tag_positions") || j.contains("apriltags")) {
            // Handle alternative FMap formats
            std::string tagsKey = j.contains("tag_positions") ? "tag_positions" : "apriltags";
            json tagsObj = j[tagsKey];
            
            for (auto it = tagsObj.begin(); it != tagsObj.end(); ++it) {
                int tagId = std::stoi(it.key());
                json tagJson = it.value();
                
                FMapTag tag;
                tag.id = tagId;
                
                if (tagJson.contains("name")) {
                    tag.name = tagJson["name"].get<std::string>();
                } else {
                    tag.name = "Tag_" + std::to_string(tagId);
                }
                
                if (tagJson.contains("size")) {
                    tag.size = tagJson["size"].get<double>();
                }
                
                // Parse pose
                if (tagJson.contains("pose")) {
                    tag.pose = parsePose(tagJson["pose"]);
                } else if (tagJson.contains("position")) {
                    tag.pose = parsePose(tagJson["position"]);
                }
                
                // Parse rotation
                if (tagJson.contains("rotation")) {
                    tag.rotation = parseRotation(tagJson["rotation"]);
                } else if (tagJson.contains("orientation")) {
                    tag.rotation = parseRotation(tagJson["orientation"]);
                }
                
                fieldMap.tags[tagId] = tag;
            }
        }
        
        LOG_INFO_F("Loaded FMap '%s' with %zu tags", 
                   fieldMap.name.c_str(), fieldMap.tags.size());
        
    } catch (const nlohmann::json_exception& e) {
        LOG_ERROR_F("Failed to parse FMap JSON: %s", e.what());
    } catch (const std::exception& e) {
        LOG_ERROR_F("Error loading FMap: %s", e.what());
    }
    
    return fieldMap;
}

bool LimelightFMapLoader::saveFMap(const FieldMap& fieldMap, const std::string& path) {
    try {
        json j;
        
        j["name"] = fieldMap.name;
        j["description"] = fieldMap.description;
        j["field_size"] = fieldMap.fieldSize;
        
        json tagsArray = json::array();
        for (const auto& pair : fieldMap.tags) {
            const FMapTag& tag = pair.second;
            json tagJson;
            
            tagJson["id"] = tag.id;
            tagJson["name"] = tag.name;
            tagJson["pose"] = tag.pose;
            tagJson["rotation"] = tag.rotation;
            tagJson["size"] = tag.size;
            
            tagsArray.push_back(tagJson);
        }
        
        j["tags"] = tagsArray;
        
        FileSystem::writeFile(path, j.dump(4));
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR_F("Failed to save FMap: %s", e.what());
        return false;
    }
}

std::map<int, ConfigManager::TagConfig> LimelightFMapLoader::toTagConfig(const FieldMap& fieldMap) {
    std::map<int, ConfigManager::TagConfig> tagConfigs;
    
    for (const auto& pair : fieldMap.tags) {
        const FMapTag& fmapTag = pair.second;
        ConfigManager::TagConfig tagConfig;
        
        tagConfig.id = fmapTag.id;
        tagConfig.pose.translation = fmapTag.pose;
        tagConfig.pose.rotation = fmapTag.rotation;
        tagConfig.size = fmapTag.size;
        
        tagConfigs[fmapTag.id] = tagConfig;
    }
    
    return tagConfigs;
}

std::map<std::string, LimelightFMapLoader::FieldMap> LimelightFMapLoader::getBuiltInFieldMaps() {
    std::map<std::string, FieldMap> maps;
    
    maps["FRC_2025_Reefscape"] = loadFRC2025Field();
    maps["FRC_2024_Crescendo"] = loadFRC2024Field();
    maps["FRC_2023_ChargedUp"] = loadFRC2023Field();
    maps["FRC_2022_RapidReact"] = loadFRC2022Field();
    
    return maps;
}

LimelightFMapLoader::FieldMap LimelightFMapLoader::loadFRC2025Field() {
    // FRC 2025 Reefscape field
    // Based on official field drawings and Limelight FMap format
    FieldMap fieldMap;
    fieldMap.name = "FRC_2025_Reefscape";
    fieldMap.description = "FRC 2025 Reefscape Field";
    fieldMap.fieldSize = {16.54, 8.21}; // Approximate field dimensions in meters
    
    // Tag layout for 2025 Reefscape
    // Note: These are approximate positions - verify with official field drawings
    std::vector<FMapTag> tags = {
        // Reef tags
        {1, "Reef_Corner_1", {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {2, "Reef_Corner_2", {16.54, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {3, "Reef_Corner_3", {16.54, 8.21, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {4, "Reef_Corner_4", {0.0, 8.21, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        // Cable tags
        {5, "Cable_1", {2.0, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {6, "Cable_2", {14.54, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {7, "Cable_3", {14.54, 6.21, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {8, "Cable_4", {2.0, 6.21, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        // Barge tags
        {9, "Barge_1", {8.27, 1.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {10, "Barge_2", {8.27, 7.21, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        // Processor tags
        {11, "Processor_1", {4.0, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {12, "Processor_2", {12.54, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
    };
    
    for (const auto& tag : tags) {
        fieldMap.tags[tag.id] = tag;
    }
    
    return fieldMap;
}

LimelightFMapLoader::FieldMap LimelightFMapLoader::loadFRC2024Field() {
    // FRC 2024 Crescendo field
    FieldMap fieldMap;
    fieldMap.name = "FRC_2024_Crescendo";
    fieldMap.description = "FRC 2024 Crescendo Field";
    fieldMap.fieldSize = {16.46, 8.21}; // Field dimensions in meters
    
    // Tag layout for 2024 Crescendo
    // Based on official field drawings
    std::vector<FMapTag> tags = {
        // Stage tags
        {1, "Stage_Left", {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {2, "Stage_Right", {16.46, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        // Amp tags
        {3, "Amp_Red", {1.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {4, "Amp_Blue", {15.46, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        // Speaker tags
        {5, "Speaker_Red", {0.0, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {6, "Speaker_Blue", {16.46, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        // Midfield tags
        {7, "Mid_1", {4.0, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {8, "Mid_2", {8.23, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {9, "Mid_3", {12.46, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {10, "Mid_4", {4.0, 6.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {11, "Mid_5", {8.23, 6.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {12, "Mid_6", {12.46, 6.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
    };
    
    for (const auto& tag : tags) {
        fieldMap.tags[tag.id] = tag;
    }
    
    return fieldMap;
}

LimelightFMapLoader::FieldMap LimelightFMapLoader::loadFRC2023Field() {
    // FRC 2023 Charged Up field
    FieldMap fieldMap;
    fieldMap.name = "FRC_2023_ChargedUp";
    fieldMap.description = "FRC 2023 Charged Up Field";
    fieldMap.fieldSize = {16.54, 8.02};
    
    // Tag layout for 2023 Charged Up
    std::vector<FMapTag> tags = {
        // Community tags
        {1, "Community_1", {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {2, "Community_2", {16.54, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {3, "Community_3", {16.54, 8.02, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {4, "Community_4", {0.0, 8.02, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        // Grid tags
        {5, "Grid_1", {2.0, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {6, "Grid_2", {4.0, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {7, "Grid_3", {6.0, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {8, "Grid_4", {8.27, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {9, "Grid_5", {10.27, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {10, "Grid_6", {12.27, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {11, "Grid_7", {14.27, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        {12, "Grid_8", {2.0, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {13, "Grid_9", {4.0, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {14, "Grid_10", {6.0, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {15, "Grid_11", {8.27, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {16, "Grid_12", {10.27, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {17, "Grid_13", {12.27, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {18, "Grid_14", {14.27, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
    };
    
    for (const auto& tag : tags) {
        fieldMap.tags[tag.id] = tag;
    }
    
    return fieldMap;
}

LimelightFMapLoader::FieldMap LimelightFMapLoader::loadFRC2022Field() {
    // FRC 2022 Rapid React field
    FieldMap fieldMap;
    fieldMap.name = "FRC_2022_RapidReact";
    fieldMap.description = "FRC 2022 Rapid React Field";
    fieldMap.fieldSize = {16.54, 8.02};
    
    // Tag layout for 2022 Rapid React
    std::vector<FMapTag> tags = {
        // Hub tags
        {1, "Hub_1", {8.27, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {2, "Hub_2", {8.27, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {3, "Hub_3", {8.27, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {4, "Hub_4", {8.27, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        // Terminal tags
        {5, "Terminal_Red_1", {0.0, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {6, "Terminal_Red_2", {0.0, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {7, "Terminal_Red_3", {0.0, 6.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        
        {8, "Terminal_Blue_1", {16.54, 2.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {9, "Terminal_Blue_2", {16.54, 4.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
        {10, "Terminal_Blue_3", {16.54, 6.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, 0.1651},
    };
    
    for (const auto& tag : tags) {
        fieldMap.tags[tag.id] = tag;
    }
    
    return fieldMap;
}

LimelightFMapLoader::FMapTag LimelightFMapLoader::parseTag(const nlohmann::json& tagJson) {
    FMapTag tag;
    
    if (tagJson.contains("id")) {
        tag.id = tagJson["id"].get<int>();
    }
    
    if (tagJson.contains("name")) {
        tag.name = tagJson["name"].get<std::string>();
    }
    
    if (tagJson.contains("size")) {
        tag.size = tagJson["size"].get<double>();
    }
    
    if (tagJson.contains("pose")) {
        tag.pose = parsePose(tagJson["pose"]);
    }
    
    if (tagJson.contains("rotation")) {
        tag.rotation = parseRotation(tagJson["rotation"]);
    }
    
    return tag;
}

std::vector<double> LimelightFMapLoader::parsePose(const nlohmann::json& poseJson) {
    std::vector<double> pose = {0.0, 0.0, 0.0};
    
    if (poseJson.is_array() && poseJson.size() >= 3) {
        pose[0] = poseJson[0].get<double>();
        pose[1] = poseJson[1].get<double>();
        pose[2] = poseJson[2].get<double>();
    }
    
    return pose;
}

std::vector<double> LimelightFMapLoader::parseRotation(const nlohmann::json& rotJson) {
    std::vector<double> rotation = {1.0, 0.0, 0.0, 0.0}; // Identity quaternion
    
    if (rotJson.is_array()) {
        if (rotJson.size() >= 4) {
            // Quaternion format [w, x, y, z]
            rotation[0] = rotJson[0].get<double>();
            rotation[1] = rotJson[1].get<double>();
            rotation[2] = rotJson[2].get<double>();
            rotation[3] = rotJson[3].get<double>();
        } else if (rotJson.size() >= 3) {
            // Euler angles format [roll, pitch, yaw] - convert to quaternion
            double roll = rotJson[0].get<double>();
            double pitch = rotJson[1].get<double>();
            double yaw = rotJson[2].get<double>();
            
            // Convert Euler angles to quaternion
            double cy = cos(yaw * 0.5);
            double sy = sin(yaw * 0.5);
            double cp = cos(pitch * 0.5);
            double sp = sin(pitch * 0.5);
            double cr = cos(roll * 0.5);
            double sr = sin(roll * 0.5);
            
            rotation[0] = cr * cp * cy + sr * sp * sy;
            rotation[1] = sr * cp * cy - cr * sp * sy;
            rotation[2] = cr * sp * cy + sr * cp * sy;
            rotation[3] = cr * cp * sy - sr * sp * cy;
        }
    }
    
    return rotation;
}
