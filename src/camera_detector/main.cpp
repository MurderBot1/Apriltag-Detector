#include "AprilTagDetector.hpp"
#include "DetectorConfig.hpp"
#include "../common/camera/CameraFactory.hpp"
#include "../common/config/GlobalConfig.hpp"
#include "../common/config/ConfigManager.hpp"
#include "../common/logging/Logger.hpp"
#include "../common/ipc/SharedMemoryFrameBuffer.hpp"
#include "../common/ipc/HealthCheck.hpp"
#include "../common/ipc/HeartbeatSender.hpp"
#include "../common/utils/TimeUtils.hpp"
#include <iostream>
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
    int cameraId = 0;
    std::string configPath;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--camera" || arg == "-c") {
            if (i + 1 < argc) {
                cameraId = std::stoi(argv[++i]);
            }
        } else if (arg == "--config" || arg == "-f") {
            if (i + 1 < argc) {
                configPath = argv[++i];
            }
        }
    }
    
    // Initialize logging
    std::string processName = "camera_detector_" + std::to_string(cameraId);
    Logger::initialize("./logs", processName, LogLevel::INFO);
    LOG_INFO_F("Starting camera detector for camera %d", cameraId);
    
    // Initialize global config
    GlobalConfig::initialize("config/system.json");
    
    // Load camera configuration
    ConfigManager::CameraConfig cameraConfig;
    if (!configPath.empty()) {
        cameraConfig = ConfigManager::loadCameraConfig(cameraId);
    } else {
        cameraConfig = ConfigManager::loadCameraConfig(cameraId);
    }
    
    LOG_INFO_F("Camera %d: Device path: %s, Type: %s", 
               cameraId, cameraConfig.devicePath.c_str(), cameraConfig.cameraType.c_str());
    
    // Create camera
    auto camera = CameraFactory::create(cameraConfig.cameraType, cameraId);
    if (!camera) {
        LOG_ERROR_F("Failed to create camera of type '%s' for camera %d", 
                    cameraConfig.cameraType.c_str(), cameraId);
        return 1;
    }
    
    // Configure camera
    camera->setSettings(cameraConfig.settings);
    camera->setIntrinsics(cameraConfig.intrinsics);
    camera->setExtrinsics(cameraConfig.extrinsics);
    
    // Open camera
    if (!camera->open(cameraId, "")) {
        LOG_ERROR_F("Failed to open camera %d", cameraId);
        return 1;
    }
    
    // Create detector
    AprilTagDetector detector(cameraId);
    if (!detector.initialize(std::move(camera))) {
        LOG_ERROR_F("Failed to initialize detector for camera %d", cameraId);
        return 1;
    }
    
    // Create shared memory buffer for IPC
    auto frameBuffer = SharedMemoryFrameBufferManager::getBuffer(
        cameraId, 1920, 1080, 3);
    
    // Create health monitor and heartbeat
    static HealthMonitor healthMonitor;
    HeartbeatSender heartbeat(&healthMonitor, processName, 1000);
    heartbeat.start();
    
    // Register process with health monitor
    healthMonitor.registerProcess(processName, static_cast<int>(getpid()));
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    
    // Start detection
    detector.startDetection([&](const DetectionFrame& detectionFrame) {
        // Send detections via shared memory
        // In a real implementation, we'd send the detections to the pose finder
        // via a dedicated IPC mechanism
        
        // For now, just write the frame to shared memory
        if (frameBuffer) {
            frameBuffer->writeFrame(detectionFrame.frame);
        }
        
        // Log detections
        if (!detectionFrame.detections.empty()) {
            LOG_DEBUG_F("Camera %d: %zu detections", 
                       cameraId, detectionFrame.detections.size());
        }
    });
    
    // Main loop
    while (!g_shutdown) {
        // Capture frame
        CameraFrame frame;
        if (detector.getCamera() && detector.getCamera()->captureFrame(frame)) {
            // Process frame
            DetectionFrame detectionFrame = detector.processFrame(frame);
            
            // If we have a callback, it's already been called
            // If we're using the startDetection callback, it's automatic
        }
        
        TimeUtils::sleepMs(10);
    }
    
    // Cleanup
    detector.stopDetection();
    heartbeat.stop();
    healthMonitor.unregisterProcess(processName);
    
    LOG_INFO_F("Camera detector %d shutting down", cameraId);
    
    return 0;
}
