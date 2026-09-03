#pragma once

#include "ConfigManager.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <map>
#include <vector>

/**
 * @brief Loader for Limelight Field Map (FMap) files
 * 
 * Supports loading tag layouts from Limelight's FMap JSON format
 * which is commonly used in FRC for field tag configurations.
 */
class LimelightFMapLoader {
public:
    
    /**
     * @brief Tag information from FMap
     */
    struct FMapTag {
        int id;                     // Tag ID
        std::string name;          // Tag name
        std::vector<double> pose;  // [x, y, z] in meters
        std::vector<double> rotation; // Quaternion [w, x, y, z]
        double size;               // Tag size in meters
        
        FMapTag() : id(-1), size(0.1651) {
            pose = {0.0, 0.0, 0.0};
            rotation = {1.0, 0.0, 0.0, 0.0};
        }
    };
    
    /**
     * @brief Field map structure
     */
    struct FieldMap {
        std::string name;
        std::string description;
        std::map<int, FMapTag> tags;
        std::vector<double> fieldSize; // [width, height] in meters
    };
    
    /**
     * @brief Constructor
     */
    LimelightFMapLoader();
    
    /**
     * @brief Destructor
     */
    ~LimelightFMapLoader();
    
    /**
     * @brief Load FMap from file
     * @param path Path to FMap JSON file
     * @return FieldMap structure
     */
    static FieldMap loadFMap(const std::string& path);
    
    /**
     * @brief Load FMap from string
     * @param jsonString JSON string
     * @return FieldMap structure
     */
    static FieldMap loadFMapFromString(const std::string& jsonString);
    
    /**
     * @brief Save FMap to file
     * @param fieldMap FieldMap to save
     * @param path Output path
     * @return true if successful
     */
    static bool saveFMap(const FieldMap& fieldMap, const std::string& path);
    
    /**
     * @brief Convert FMap to ConfigManager tag config
     * @param fieldMap FieldMap to convert
     * @return Map of tag ID to TagConfig
     */
    static std::map<int, ConfigManager::TagConfig> toTagConfig(const FieldMap& fieldMap);
    
    /**
     * @brief Get built-in FRC field maps
     * @return Map of field name to FieldMap
     */
    static std::map<std::string, FieldMap> getBuiltInFieldMaps();
    
    /**
     * @brief Load FRC 2025 field map (Reefscape)
     * @return FieldMap for 2025 FRC field
     */
    static FieldMap loadFRC2025Field();
    
    /**
     * @brief Load FRC 2024 field map (Crescendo)
     * @return FieldMap for 2024 FRC field
     */
    static FieldMap loadFRC2024Field();
    
    /**
     * @brief Load FRC 2023 field map (Charged Up)
     * @return FieldMap for 2023 FRC field
     */
    static FieldMap loadFRC2023Field();
    
    /**
     * @brief Load FRC 2022 field map (Rapid React)
     * @return FieldMap for 2022 FRC field
     */
    static FieldMap loadFRC2022Field();
    
private:
    /**
     * @brief Parse tag from JSON
     * @param tagJson JSON tag object
     * @return FMapTag
     */
    static FMapTag parseTag(const nlohmann::json& tagJson);
    
    /**
     * @brief Parse pose from JSON array
     * @param poseJson JSON array [x, y, z]
     * @return Pose vector
     */
    static std::vector<double> parsePose(const nlohmann::json& poseJson);
    
    /**
     * @brief Parse rotation from JSON array
     * @param rotJson JSON array (quaternion or Euler angles)
     * @return Rotation vector (quaternion)
     */
    static std::vector<double> parseRotation(const nlohmann::json& rotJson);
};
