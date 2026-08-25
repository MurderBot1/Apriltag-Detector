#include "ConfigAPI.hpp"
#include "../common/logging/Logger.hpp"
#include "../common/utils/JsonConfig.hpp"
#include <sstream>
#include <algorithm>

ConfigAPI::ConfigAPI(WebServer* server) : m_server(server) {
}

ConfigAPI::~ConfigAPI() {
}

void ConfigAPI::registerEndpoints() {
    if (!m_server) {
        return;
    }
    
    // GET /api/config/system
    m_server->registerHandler("/api/config/system", 
        [this](const HttpRequest& request) {
            if (request.method == "GET") {
                return handleGetSystemConfig(request);
            }
            return HttpResponse();
        }, {"GET"});
    
    // PUT /api/config/system
    m_server->registerHandler("/api/config/system", 
        [this](const HttpRequest& request) {
            if (request.method == "PUT") {
                return handlePutSystemConfig(request);
            }
            return HttpResponse();
        }, {"PUT"});
    
    // GET /api/config/cameras
    m_server->registerHandler("/api/config/cameras", 
        [this](const HttpRequest& request) {
            if (request.method == "GET") {
                return handleGetCamerasConfig(request);
            }
            return HttpResponse();
        }, {"GET"});
    
    // GET /api/config/cameras/{id}
    m_server->registerHandler("/api/config/cameras/*", 
        [this](const HttpRequest& request) {
            if (request.method == "GET") {
                return handleGetCameraConfig(request);
            }
            return HttpResponse();
        }, {"GET"});
    
    // PUT /api/config/cameras/{id}
    m_server->registerHandler("/api/config/cameras/*", 
        [this](const HttpRequest& request) {
            if (request.method == "PUT") {
                return handlePutCameraConfig(request);
            }
            return HttpResponse();
        }, {"PUT"});
    
    // GET /api/config/tags
    m_server->registerHandler("/api/config/tags", 
        [this](const HttpRequest& request) {
            if (request.method == "GET") {
                return handleGetTagsConfig(request);
            }
            return HttpResponse();
        }, {"GET"});
    
    // PUT /api/config/tags
    m_server->registerHandler("/api/config/tags", 
        [this](const HttpRequest& request) {
            if (request.method == "PUT") {
                return handlePutTagsConfig(request);
            }
            return HttpResponse();
        }, {"PUT"});
    
    // GET /api/config/global
    m_server->registerHandler("/api/config/global", 
        [this](const HttpRequest& request) {
            if (request.method == "GET") {
                return handleGetGlobalConfig(request);
            }
            return HttpResponse();
        }, {"GET"});
    
    // PUT /api/config/global
    m_server->registerHandler("/api/config/global", 
        [this](const HttpRequest& request) {
            if (request.method == "PUT") {
                return handlePutGlobalConfig(request);
            }
            return HttpResponse();
        }, {"PUT"});
    
    // POST /api/config/reload
    m_server->registerHandler("/api/config/reload", 
        [this](const HttpRequest& request) {
            if (request.method == "POST") {
                return handlePostReloadConfig(request);
            }
            return HttpResponse();
        }, {"POST"});
    
    LOG_INFO("Registered ConfigAPI endpoints");
}

std::string ConfigAPI::getSystemConfig() const {
    ConfigManager::SystemConfig config = ConfigManager::loadSystemConfig();
    
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"nt4Address\": \"" << config.nt4Address << "\",\n";
    stream << "  \"nt4Port\": " << config.nt4Port << ",\n";
    stream << "  \"numCameras\": " << config.numCameras << ",\n";
    stream << "  \"logLevel\": \"" << config.logLevel << "\"\n";
    stream << "}";
    
    return stream.str();
}

std::string ConfigAPI::getCameraConfig(int cameraId) const {
    ConfigManager::CameraConfig config = ConfigManager::loadCameraConfig(cameraId);
    
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"cameraId\": " << config.cameraId << ",\n";
    stream << "  \"devicePath\": \"" << config.devicePath << "\",\n";
    stream << "  \"cameraType\": \"" << config.cameraType << "\",\n";
    
    // Intrinsics
    stream << "  \"intrinsics\": {\n";
    stream << "    \"fx\": " << config.intrinsics.fx << ",\n";
    stream << "    \"fy\": " << config.intrinsics.fy << ",\n";
    stream << "    \"cx\": " << config.intrinsics.cx << ",\n";
    stream << "    \"cy\": " << config.intrinsics.cy << ",\n";
    stream << "    \"width\": " << config.intrinsics.width << ",\n";
    stream << "    \"height\": " << config.intrinsics.height << "\n";
    stream << "  },\n";
    
    // Extrinsics
    stream << "  \"extrinsics\": {\n";
    stream << "    \"translation\": [" << config.extrinsics.translation[0] << ", "
              << config.extrinsics.translation[1] << ", "
              << config.extrinsics.translation[2] << "],\n";
    stream << "    \"rotation\": [" << config.extrinsics.rotation[0] << ", "
              << config.extrinsics.rotation[1] << ", "
              << config.extrinsics.rotation[2] << ", "
              << config.extrinsics.rotation[3] << "]\n";
    stream << "  },\n";
    
    // Settings
    stream << "  \"settings\": {\n";
    stream << "    \"exposure\": " << config.settings.exposure << ",\n";
    stream << "    \"brightness\": " << config.settings.brightness << ",\n";
    stream << "    \"contrast\": " << config.settings.contrast << ",\n";
    stream << "    \"saturation\": " << config.settings.saturation << ",\n";
    stream << "    \"gain\": " << config.settings.gain << "\n";
    stream << "  }\n";
    stream << "}";
    
    return stream.str();
}

std::string ConfigAPI::getAllCameraConfigs() const {
    auto configs = ConfigManager::loadAllCameraConfigs();
    
    std::ostringstream stream;
    stream << "[\n";
    
    for (size_t i = 0; i < configs.size(); ++i) {
        if (i > 0) {
            stream << ",\n";
        }
        stream << "  " << getCameraConfig(configs[i].cameraId);
    }
    
    stream << "\n]";
    return stream.str();
}

std::string ConfigAPI::getTagsConfig() const {
    ConfigManager::TagsConfig config = ConfigManager::loadTagsConfig();
    
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"tags\": [\n";
    
    for (size_t i = 0; i < config.tags.size(); ++i) {
        if (i > 0) {
            stream << ",\n";
        }
        stream << "    {\n";
        stream << "      \"id\": " << config.tags[i].id << ",\n";
        stream << "      \"translation\": [" << config.tags[i].translation[0] << ", "
                  << config.tags[i].translation[1] << ", "
                  << config.tags[i].translation[2] << "],\n";
        stream << "      \"rotation\": [" << config.tags[i].rotation[0] << ", "
                  << config.tags[i].rotation[1] << ", "
                  << config.tags[i].rotation[2] << ", "
                  << config.tags[i].rotation[3] << "],\n";
        stream << "      \"size\": " << config.tags[i].size << "\n";
        stream << "    }";
    }
    
    stream << "\n  ]\n";
    stream << "}";
    
    return stream.str();
}

bool ConfigAPI::updateSystemConfig(const std::string& config) {
    try {
        ConfigManager::SystemConfig systemConfig;
        
        // Parse JSON (simplified - in production use proper JSON parser)
        // For now, just log and return true
        LOG_INFO_F("Updating system config: %s", config.c_str());
        
        // In a real implementation, we'd parse the JSON and update the config
        // Then save to file
        
        return ConfigManager::saveSystemConfig(systemConfig);
    } catch (const std::exception& e) {
        LOG_ERROR_F("Failed to update system config: %s", e.what());
        return false;
    }
}

bool ConfigAPI::updateCameraConfig(int cameraId, const std::string& config) {
    try {
        ConfigManager::CameraConfig cameraConfig;
        cameraConfig.cameraId = cameraId;
        
        // Parse JSON (simplified)
        LOG_INFO_F("Updating camera %d config: %s", cameraId, config.c_str());
        
        // In a real implementation, we'd parse the JSON and update the config
        // Then save to file
        
        return ConfigManager::saveCameraConfig(cameraConfig);
    } catch (const std::exception& e) {
        LOG_ERROR_F("Failed to update camera config: %s", e.what());
        return false;
    }
}

bool ConfigAPI::updateTagsConfig(const std::string& config) {
    try {
        ConfigManager::TagsConfig tagsConfig;
        
        // Parse JSON (simplified)
        LOG_INFO_F("Updating tags config: %s", config.c_str());
        
        // In a real implementation, we'd parse the JSON and update the config
        // Then save to file
        
        return ConfigManager::saveTagsConfig(tagsConfig);
    } catch (const std::exception& e) {
        LOG_ERROR_F("Failed to update tags config: %s", e.what());
        return false;
    }
}

void ConfigAPI::reloadAllConfig() {
    GlobalConfig::reload();
    LOG_INFO("Reloaded all configuration files");
}

std::string ConfigAPI::getGlobalConfig() const {
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"tagFamily\": \"" << GlobalConfig::TAG_FAMILY << "\",\n";
    stream << "  \"tagSize\": " << GlobalConfig::TAG_SIZE << ",\n";
    stream << "  \"minTagSize\": " << GlobalConfig::MIN_TAG_SIZE << ",\n";
    stream << "  \"maxTagSize\": " << GlobalConfig::MAX_TAG_SIZE << ",\n";
    stream << "  \"detectThreshold\": " << GlobalConfig::DETECT_THRESHOLD << "\n";
    stream << "}";
    
    return stream.str();
}

bool ConfigAPI::updateGlobalConfig(const std::string& config) {
    try {
        // Parse JSON (simplified)
        LOG_INFO_F("Updating global config: %s", config.c_str());
        
        // In a real implementation, we'd parse the JSON and update GlobalConfig
        // Then save to file
        
        GlobalConfig::reload();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_F("Failed to update global config: %s", e.what());
        return false;
    }
}

std::string ConfigAPI::createJsonResponse(const std::string& status, 
                                         const std::string& data) const {
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"status\": \"" << status << "\",\n";
    stream << "  \"data\": " << data << "\n";
    stream << "}";
    
    return stream.str();
}

std::string ConfigAPI::createErrorResponse(const std::string& error) const {
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"status\": \"error\",\n";
    stream << "  \"error\": \"" << error << "\"\n";
    stream << "}";
    
    return stream.str();
}

HttpResponse ConfigAPI::handleGetSystemConfig(const HttpRequest& request) {
    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    std::string config = getSystemConfig();
    response.body = std::vector<uint8_t>(config.begin(), config.end());
    
    return response;
}

HttpResponse ConfigAPI::handlePutSystemConfig(const HttpRequest& request) {
    HttpResponse response;
    
    std::string config(request.body.begin(), request.body.end());
    
    if (updateSystemConfig(config)) {
        response.statusCode = 200;
        response.statusText = "OK";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"ok\"}".begin(), "{\"status\":\"ok\"}".end());
    } else {
        response.statusCode = 500;
        response.statusText = "Internal Server Error";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"error\",\"message\":\"Failed to update config\"}".begin(),
            "{\"status\":\"error\",\"message\":\"Failed to update config\"}".end());
    }
    
    return response;
}

HttpResponse ConfigAPI::handleGetCamerasConfig(const HttpRequest& request) {
    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    std::string config = getAllCameraConfigs();
    response.body = std::vector<uint8_t>(config.begin(), config.end());
    
    return response;
}

HttpResponse ConfigAPI::handleGetCameraConfig(const HttpRequest& request) {
    HttpResponse response;
    
    int cameraId = parseCameraId(request.path);
    
    if (cameraId < 0) {
        response.statusCode = 400;
        response.statusText = "Bad Request";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"error\",\"message\":\"Invalid camera ID\"}".begin(),
            "{\"status\":\"error\",\"message\":\"Invalid camera ID\"}".end());
        return response;
    }
    
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    std::string config = getCameraConfig(cameraId);
    response.body = std::vector<uint8_t>(config.begin(), config.end());
    
    return response;
}

HttpResponse ConfigAPI::handlePutCameraConfig(const HttpRequest& request) {
    HttpResponse response;
    
    int cameraId = parseCameraId(request.path);
    
    if (cameraId < 0) {
        response.statusCode = 400;
        response.statusText = "Bad Request";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"error\",\"message\":\"Invalid camera ID\"}".begin(),
            "{\"status\":\"error\",\"message\":\"Invalid camera ID\"}".end());
        return response;
    }
    
    std::string config(request.body.begin(), request.body.end());
    
    if (updateCameraConfig(cameraId, config)) {
        response.statusCode = 200;
        response.statusText = "OK";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"ok\"}".begin(), "{\"status\":\"ok\"}".end());
    } else {
        response.statusCode = 500;
        response.statusText = "Internal Server Error";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"error\",\"message\":\"Failed to update config\"}".begin(),
            "{\"status\":\"error\",\"message\":\"Failed to update config\"}".end());
    }
    
    return response;
}

HttpResponse ConfigAPI::handleGetTagsConfig(const HttpRequest& request) {
    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    std::string config = getTagsConfig();
    response.body = std::vector<uint8_t>(config.begin(), config.end());
    
    return response;
}

HttpResponse ConfigAPI::handlePutTagsConfig(const HttpRequest& request) {
    HttpResponse response;
    
    std::string config(request.body.begin(), request.body.end());
    
    if (updateTagsConfig(config)) {
        response.statusCode = 200;
        response.statusText = "OK";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"ok\"}".begin(), "{\"status\":\"ok\"}".end());
    } else {
        response.statusCode = 500;
        response.statusText = "Internal Server Error";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"error\",\"message\":\"Failed to update config\"}".begin(),
            "{\"status\":\"error\",\"message\":\"Failed to update config\"}".end());
    }
    
    return response;
}

HttpResponse ConfigAPI::handleGetGlobalConfig(const HttpRequest& request) {
    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    std::string config = getGlobalConfig();
    response.body = std::vector<uint8_t>(config.begin(), config.end());
    
    return response;
}

HttpResponse ConfigAPI::handlePutGlobalConfig(const HttpRequest& request) {
    HttpResponse response;
    
    std::string config(request.body.begin(), request.body.end());
    
    if (updateGlobalConfig(config)) {
        response.statusCode = 200;
        response.statusText = "OK";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"ok\"}".begin(), "{\"status\":\"ok\"}".end());
    } else {
        response.statusCode = 500;
        response.statusText = "Internal Server Error";
        response.headers["Content-Type"] = "application/json";
        response.body = std::vector<uint8_t>(
            "{\"status\":\"error\",\"message\":\"Failed to update config\"}".begin(),
            "{\"status\":\"error\",\"message\":\"Failed to update config\"}".end());
    }
    
    return response;
}

HttpResponse ConfigAPI::handlePostReloadConfig(const HttpRequest& request) {
    HttpResponse response;
    
    reloadAllConfig();
    
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    response.body = std::vector<uint8_t>(
        "{\"status\":\"ok\",\"message\":\"Configuration reloaded\"}".begin(),
        "{\"status\":\"ok\",\"message\":\"Configuration reloaded\"}".end());
    
    return response;
}

int ConfigAPI::parseCameraId(const std::string& path) const {
    size_t pos = path.find("cameras/");
    if (pos == std::string::npos) {
        return -1;
    }
    
    std::string idStr = path.substr(pos + 8);
    
    // Remove trailing slash or .json
    size_t endPos = idStr.find_first_of("/.");
    if (endPos != std::string::npos) {
        idStr = idStr.substr(0, endPos);
    }
    
    try {
        return std::stoi(idStr);
    } catch (...) {
        return -1;
    }
}
