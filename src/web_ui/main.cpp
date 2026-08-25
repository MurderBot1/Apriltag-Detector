#include "WebServer.hpp"
#include "StreamHandler.hpp"
#include "ConfigAPI.hpp"
#include "../common/config/GlobalConfig.hpp"
#include "../common/config/ConfigManager.hpp"
#include "../common/logging/Logger.hpp"
#include "../common/ipc/HealthCheck.hpp"
#include "../common/ipc/HeartbeatSender.hpp"
#include "../common/utils/TimeUtils.hpp"
#include <csignal>
#include <atomic>

// Global signal flag
static std::atomic<bool> g_shutdown(false);

// Signal handler
void signalHandler(int signal) {
    g_shutdown = true;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = 8080;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" || arg == "-p") {
            if (i + 1 < argc) {
                port = std::stoi(argv[++i]);
            }
        }
    }
    
    // Initialize logging
    Logger::initialize("./logs", "web_ui", LogLevel::INFO);
    LOG_INFO_F("Starting web UI on port %d", port);
    
    // Initialize global config
    GlobalConfig::initialize("config/system.json");
    
    // Create web server
    WebServer server(port, 4);
    
    // Create stream handler
    StreamHandler streamHandler;
    
    // Create config API
    ConfigAPI configAPI(&server);
    
    // Register config API endpoints
    configAPI.registerEndpoints();
    
    // Register stream handler with web server
    streamHandler.registerWithWebServer(&server);
    
    // Register static directory for web assets
    server.registerStaticDirectory("/", "./web");
    server.registerStaticDirectory("/static", "./web/static");
    
    // Create health monitor and heartbeat
    static HealthMonitor healthMonitor;
    HeartbeatSender heartbeat(&healthMonitor, "web_ui", 1000);
    heartbeat.start();
    
    // Register process with health monitor
    healthMonitor.registerProcess("web_ui", static_cast<int>(getpid()));
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    
    // Start web server
    if (!server.start()) {
        LOG_ERROR("Failed to start web server");
        return 1;
    }
    
    // Start streams for all cameras
    auto cameraConfigs = ConfigManager::loadAllCameraConfigs();
    for (const auto& config : cameraConfigs) {
        streamHandler.startStream(config.cameraId);
        LOG_INFO_F("Started stream for camera %d", config.cameraId);
    }
    
    // Main loop
    while (!g_shutdown) {
        TimeUtils::sleepMs(100);
    }
    
    // Cleanup
    streamHandler.stopAllStreams();
    server.stop();
    heartbeat.stop();
    healthMonitor.unregisterProcess("web_ui");
    
    LOG_INFO("Web UI shutting down");
    
    return 0;
}
