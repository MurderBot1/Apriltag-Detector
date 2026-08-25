#pragma once

#include "WebServer.hpp"
#include "../common/config/ConfigManager.hpp"
#include "../common/config/GlobalConfig.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>

/**
 * @brief Configuration API for the web interface
 * 
 * Provides REST endpoints for managing configuration
 */
class ConfigAPI {
public:
    
    /**
     * @brief Constructor
     * @param server WebServer instance
     */
    explicit ConfigAPI(WebServer* server);
    
    /**
     * @brief Destructor
     */
    ~ConfigAPI();
    
    /**
     * @brief Register all API endpoints
     */
    void registerEndpoints();
    
    /**
     * @brief Get system configuration
     * @return JSON string with system config
     */
    std::string getSystemConfig() const;
    
    /**
     * @brief Get camera configuration
     * @param cameraId Camera ID
     * @return JSON string with camera config
     */
    std::string getCameraConfig(int cameraId) const;
    
    /**
     * @brief Get all camera configurations
     * @return JSON string with all camera configs
     */
    std::string getAllCameraConfigs() const;
    
    /**
     * @brief Get tag configuration
     * @return JSON string with tag config
     */
    std::string getTagsConfig() const;
    
    /**
     * @brief Update system configuration
     * @param config JSON string with system config
     * @return true if updated successfully
     */
    bool updateSystemConfig(const std::string& config);
    
    /**
     * @brief Update camera configuration
     * @param cameraId Camera ID
     * @param config JSON string with camera config
     * @return true if updated successfully
     */
    bool updateCameraConfig(int cameraId, const std::string& config);
    
    /**
     * @brief Update tag configuration
     * @param config JSON string with tag config
     * @return true if updated successfully
     */
    bool updateTagsConfig(const std::string& config);
    
    /**
     * @brief Reload all configuration files
     */
    void reloadAllConfig();
    
    /**
     * @brief Get global configuration
     * @return JSON string with global config
     */
    std::string getGlobalConfig() const;
    
    /**
     * @brief Update global configuration
     * @param config JSON string with global config
     * @return true if updated successfully
     */
    bool updateGlobalConfig(const std::string& config);
    
private:
    WebServer* m_server;
    
    /**
     * @brief Create JSON response
     * @param status Status string
     * @param data Data to include
     * @return JSON string
     */
    std::string createJsonResponse(const std::string& status, 
                                  const std::string& data = "") const;
    
    /**
     * @brief Create error response
     * @param error Error message
     * @return JSON string
     */
    std::string createErrorResponse(const std::string& error) const;
    
    /**
     * @brief Handle GET /api/config/system
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handleGetSystemConfig(const HttpRequest& request);
    
    /**
     * @brief Handle PUT /api/config/system
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handlePutSystemConfig(const HttpRequest& request);
    
    /**
     * @brief Handle GET /api/config/cameras
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handleGetCamerasConfig(const HttpRequest& request);
    
    /**
     * @brief Handle GET /api/config/cameras/{id}
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handleGetCameraConfig(const HttpRequest& request);
    
    /**
     * @brief Handle PUT /api/config/cameras/{id}
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handlePutCameraConfig(const HttpRequest& request);
    
    /**
     * @brief Handle GET /api/config/tags
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handleGetTagsConfig(const HttpRequest& request);
    
    /**
     * @brief Handle PUT /api/config/tags
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handlePutTagsConfig(const HttpRequest& request);
    
    /**
     * @brief Handle GET /api/config/global
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handleGetGlobalConfig(const HttpRequest& request);
    
    /**
     * @brief Handle PUT /api/config/global
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handlePutGlobalConfig(const HttpRequest& request);
    
    /**
     * @brief Handle POST /api/config/reload
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handlePostReloadConfig(const HttpRequest& request);
    
    /**
     * @brief Parse camera ID from request path
     * @param path Request path
     * @return Camera ID or -1 if not found
     */
    int parseCameraId(const std::string& path) const;
};
