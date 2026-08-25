#include "PoseSolver.hpp"
#include "../common/config/GlobalConfig.hpp"
#include "../common/config/ConfigManager.hpp"
#include "../common/nt4/NT4Client.hpp"
#include "../common/nt4/NT4Publisher.hpp"
#include "../common/ipc/SharedMemoryFrameBuffer.hpp"
#include "../common/ipc/HealthCheck.hpp"
#include "../common/ipc/HeartbeatSender.hpp"
#include "../common/logging/Logger.hpp"
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
    // Initialize logging
    Logger::initialize("./logs", "pose_finder", LogLevel::INFO);
    LOG_INFO("Starting pose finder");
    
    // Initialize global config
    GlobalConfig::initialize("config/system.json");
    
    // Load tag configuration
    ConfigManager::TagsConfig tagsConfig = 
        ConfigManager::loadTagsConfig("config/tags.json");
    
    // Create pose solver
    PoseSolver poseSolver;
    
    // Add known tags
    for (const auto& tag : tagsConfig.tags) {
        TagPose tagPose;
        tagPose.translation = tag.translation;
        tagPose.rotation = tag.rotation;
        tagPose.size = tag.size;
        poseSolver.addKnownTag(tag.id, tagPose);
        LOG_INFO_F("Added tag %d at (%f, %f, %f)", tag.id,
                   tag.translation[0], tag.translation[1], tag.translation[2]);
    }
    
    // Create NT4 client and publisher
    NT4Client nt4Client;
    NT4Publisher nt4Publisher(&nt4Client);
    
    // Connect to NT4 server
    if (!nt4Client.connect("127.0.0.1", 5810)) {
        LOG_WARN("Failed to connect to NT4 server, will retry...");
    }
    
    // Start NT4 client thread
    nt4Client.start();
    
    // Create health monitor and heartbeat
    static HealthMonitor healthMonitor;
    HeartbeatSender heartbeat(&healthMonitor, "pose_finder", 1000);
    heartbeat.start();
    
    // Register process with health monitor
    healthMonitor.registerProcess("pose_finder", static_cast<int>(getpid()));
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    
    // Main loop
    while (!g_shutdown) {
        // Collect detections from all cameras
        std::map<int, std::vector<AprilTagDetection>> allDetections;
        std::map<int, CameraIntrinsics> cameraIntrinsics;
        std::map<int, CameraExtrinsics> cameraExtrinsics;
        
        // Get camera configurations
        auto cameraConfigs = ConfigManager::loadAllCameraConfigs();
        
        // Read from shared memory buffers
        for (const auto& config : cameraConfigs) {
            int cameraId = config.cameraId;
            
            auto frameBuffer = SharedMemoryFrameBufferManager::getBuffer(
                cameraId, 1920, 1080, 3);
            
            CameraFrame frame;
            if (frameBuffer && frameBuffer->tryReadFrame(frame)) {
                // In a real implementation, we'd have a way to get
                // detections from the camera detector
                // For now, we'll just note that we got a frame
                
                // For demo purposes, add a dummy detection
                // In production, this would come from the camera detector
                AprilTagDetection detection;
                detection.id = 0;  // Tag ID
                detection.centerX = frame.width / 2.0;
                detection.centerY = frame.height / 2.0;
                detection.timestamp = frame.timestamp;
                detection.tagSize = GlobalConfig::TAG_SIZE;
                detection.confidence = 0.8;
                
                // Add corners (simplified)
                double halfSize = 50.0;  // pixels
                detection.corners[0][0] = frame.width / 2.0 - halfSize;
                detection.corners[0][1] = frame.height / 2.0 - halfSize;
                detection.corners[1][0] = frame.width / 2.0 + halfSize;
                detection.corners[1][1] = frame.height / 2.0 - halfSize;
                detection.corners[2][0] = frame.width / 2.0 + halfSize;
                detection.corners[2][1] = frame.height / 2.0 + halfSize;
                detection.corners[3][0] = frame.width / 2.0 - halfSize;
                detection.corners[3][1] = frame.height / 2.0 + halfSize;
                
                allDetections[cameraId].push_back(detection);
                cameraIntrinsics[cameraId] = config.intrinsics;
                cameraExtrinsics[cameraId] = config.extrinsics;
            }
        }
        
        // Solve combined pose
        PoseResult pose = poseSolver.solveCombinedPose(
            allDetections, cameraIntrinsics, cameraExtrinsics);
        
        // Get detection counts
        auto detectionCounts = poseSolver.getDetectionCounts(allDetections);
        
        // Publish combined robot position
        if (pose.confidence > 0.1) {
            nt4Publisher.publishCombinedRobotPosition(
                pose.translation,
                pose.euler,
                pose.confidence,
                static_cast<int>(pose.usedCameraIds.size())
            );
            
            LOG_DEBUG_F("Published robot position: (%f, %f, %f), confidence: %f",
                       pose.translation[0], pose.translation[1], pose.translation[2],
                       pose.confidence);
        }
        
        // Publish detection counts for each camera
        for (const auto& count : detectionCounts) {
            nt4Publisher.publishDetectionCount(count.first, count.second);
            LOG_DEBUG_F("Published detection count for camera %d: %d", 
                       count.first, count.second);
        }
        
        // Try to reconnect if disconnected
        if (!nt4Client.isConnected()) {
            LOG_WARN("NT4 disconnected, attempting to reconnect...");
            nt4Client.connect("127.0.0.1", 5810);
        }
        
        TimeUtils::sleepMs(50);
    }
    
    // Cleanup
    heartbeat.stop();
    healthMonitor.unregisterProcess("pose_finder");
    nt4Client.stop();
    nt4Client.disconnect();
    
    LOG_INFO("Pose finder shutting down");
    
    return 0;
}
