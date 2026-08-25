#include "../common/ipc/ProcessManager.hpp"
#include "../common/ipc/HealthCheck.hpp"
#include "../common/ipc/HeartbeatSender.hpp"
#include "../common/config/GlobalConfig.hpp"
#include "../common/config/ConfigManager.hpp"
#include "../common/logging/Logger.hpp"
#include "../common/utils/TimeUtils.hpp"
#include "../common/utils/Platform.hpp"
#include "../common/utils/FileSystem.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

// Global signal flag
static std::atomic<bool> g_shutdown(false);

// Signal handler
void signalHandler(int signal) {
    g_shutdown = true;
}

int main(int argc, char* argv[]) {
    // Get launch timestamp for log directory
    std::string launchTime = TimeUtils::getCurrentTimestampStr();
    
    // Initialize logging with timestamp directory
    Logger::initialize("./logs/" + launchTime + "/system", "launcher", LogLevel::INFO);
    LOG_INFO_F("Starting AprilTag Detector System (Launch: %s)", launchTime.c_str());
    
    // Initialize global config
    GlobalConfig::initialize("config/system.json");
    LOG_INFO_F("Tag family: %s, Tag size: %f", 
               GlobalConfig::TAG_FAMILY.c_str(), GlobalConfig::TAG_SIZE);
    
    // Create process manager
    ProcessManager processManager;
    
    // Create health monitor
    HealthMonitor healthMonitor;
    
    // Create heartbeat sender
    HeartbeatSender heartbeat(&healthMonitor, "launcher", 1000);
    heartbeat.start();
    
    // Register launcher process
    healthMonitor.registerProcess("launcher", static_cast<int>(getpid()));
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    
    // Start process monitoring
    processManager.startMonitoring();
    
    // Load system configuration
    ConfigManager::SystemConfig systemConfig = 
        ConfigManager::loadSystemConfig("config/system.json");
    
    // Load camera configurations
    auto cameraConfigs = ConfigManager::loadAllCameraConfigs();
    
    // Start web UI
    ProcessConfig webUiConfig;
    webUiConfig.name = "web_ui";
    webUiConfig.executable = "./bin/web_ui";
    webUiConfig.arguments = {"--port", "8080"};
    webUiConfig.restartPolicy = 1;  // Always restart
    webUiConfig.restartDelayMs = 1000;
    webUiConfig.maxRestartAttempts = 5;
    
    if (!processManager.startProcess(webUiConfig)) {
        LOG_ERROR("Failed to start web UI");
    }
    
    // Start pose finder
    ProcessConfig poseFinderConfig;
    poseFinderConfig.name = "pose_finder";
    poseFinderConfig.executable = "./bin/pose_finder";
    poseFinderConfig.restartPolicy = 1;  // Always restart
    poseFinderConfig.restartDelayMs = 1000;
    poseFinderConfig.maxRestartAttempts = 5;
    
    if (!processManager.startProcess(poseFinderConfig)) {
        LOG_ERROR("Failed to start pose finder");
    }
    
    // Start camera detectors
    for (const auto& cameraConfig : cameraConfigs) {
        ProcessConfig detectorConfig;
        detectorConfig.name = "camera_detector_" + std::to_string(cameraConfig.cameraId);
        detectorConfig.executable = "./bin/camera_detector";
        detectorConfig.arguments = {
            "--camera", std::to_string(cameraConfig.cameraId),
            "--config", "config/cameras/camera_" + std::to_string(cameraConfig.cameraId) + ".json"
        };
        detectorConfig.restartPolicy = 1;  // Always restart
        detectorConfig.restartDelayMs = 1000;
        detectorConfig.maxRestartAttempts = 5;
        
        if (!processManager.startProcess(detectorConfig)) {
            LOG_ERROR_F("Failed to start camera detector for camera %d", 
                        cameraConfig.cameraId);
        }
    }
    
    // Register status callback
    processManager.registerStatusCallback(
        [&](const std::string& name, const ProcessManager::ProcessStatus& status) {
            if (status.isRunning) {
                LOG_DEBUG_F("Process '%s' is running (PID: %d)", name.c_str(), status.pid);
            } else {
                LOG_WARN_F("Process '%s' stopped (Exit code: %d, Restarts: %d)", 
                          name.c_str(), status.exitCode, status.restartCount);
            }
        }
    );
    
    // Main loop
    LOG_INFO("All processes started. Monitoring...");
    
    while (!g_shutdown) {
        // Print status every 5 seconds
        static uint64_t lastStatusTime = TimeUtils::getCurrentTimestampMs();
        uint64_t now = TimeUtils::getCurrentTimestampMs();
        
        if (now - lastStatusTime >= 5000) {
            lastStatusTime = now;
            
            auto allStatus = processManager.getAllStatus();
            LOG_INFO_F("Process status: %zu processes running", allStatus.size());
            
            for (const auto& pair : allStatus) {
                LOG_INFO_F("  %s: %s (PID: %d)", 
                           pair.first.c_str(),
                           pair.second.isRunning ? "Running" : "Stopped",
                           pair.second.pid);
            }
        }
        
        TimeUtils::sleepMs(100);
    }
    
    // Cleanup
    LOG_INFO("Shutting down all processes...");
    
    processManager.stopMonitoring();
    processManager.stopAll();
    
    heartbeat.stop();
    healthMonitor.unregisterProcess("launcher");
    
    LOG_INFO("Launcher shutting down");
    
    return 0;
}
