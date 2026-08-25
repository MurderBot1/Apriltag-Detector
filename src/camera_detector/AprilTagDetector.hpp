#pragma once

#include "../common/camera/CameraInterface.hpp"
#include "../common/config/GlobalConfig.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

/**
 * @brief AprilTag detection result
 */
struct AprilTagDetection {
    int id;                     // Tag ID
    double centerX, centerY;    // Image coordinates (pixels)
    double corners[4][2];       // Four corners (x,y for each)
    double tagSize;            // Detected tag size in meters
    double confidence;          // Detection confidence (0.0 to 1.0)
    uint64_t timestamp;         // Frame timestamp (microseconds)
    
    AprilTagDetection() 
        : id(-1), centerX(0), centerY(0), tagSize(0.0), 
          confidence(0.0), timestamp(0) {
        for (int i = 0; i < 4; ++i) {
            corners[i][0] = corners[i][1] = 0.0;
        }
    }
};

/**
 * @brief Frame with detections
 */
struct DetectionFrame {
    CameraFrame frame;
    std::vector<AprilTagDetection> detections;
    int cameraId;
    uint64_t frameTimestamp;
};

/**
 * @brief AprilTag detector
 * 
 * Detects AprilTags in camera frames using the configured tag family
 */
class AprilTagDetector {
public:
    
    /**
     * @brief Constructor
     * @param cameraId Camera ID
     */
    explicit AprilTagDetector(int cameraId);
    
    /**
     * @brief Destructor
     */
    ~AprilTagDetector();
    
    /**
     * @brief Initialize the detector
     * @param camera Camera interface
     * @return true if initialized successfully
     */
    bool initialize(std::unique_ptr<CameraInterface> camera);
    
    /**
     * @brief Process a frame
     * @param frame Input frame
     * @return DetectionFrame with detections
     */
    DetectionFrame processFrame(const CameraFrame& frame);
    
    /**
     * @brief Start continuous detection
     * @param callback Callback for detection results
     */
    void startDetection(std::function<void(const DetectionFrame&)> callback);
    
    /**
     * @brief Stop continuous detection
     */
    void stopDetection();
    
    /**
     * @brief Set the camera
     * @param camera Camera interface
     */
    void setCamera(std::unique_ptr<CameraInterface> camera);
    
    /**
     * @brief Get the camera
     * @return Camera interface
     */
    CameraInterface* getCamera();
    
    /**
     * @brief Get camera ID
     * @return Camera ID
     */
    int getCameraId() const;
    
    /**
     * @brief Get the number of detections in the last frame
     * @return Number of detections
     */
    int getDetectionCount() const;
    
    /**
     * @brief Get the last detection frame
     * @return Last detection frame
     */
    DetectionFrame getLastDetectionFrame() const;
    
    /**
     * @brief Set detector parameters
     * @param minSize Minimum tag size (meters)
     * @param maxSize Maximum tag size (meters)
     * @param threshold Detection threshold (0.0 to 1.0)
     */
    void setParameters(double minSize, double maxSize, double threshold);
    
    /**
     * @brief Get current parameters
     * @param minSize Output minimum size
     * @param maxSize Output maximum size
     * @param threshold Output threshold
     */
    void getParameters(double& minSize, double& maxSize, double& threshold) const;
    
    /**
     * @brief Check if detector is running
     * @return true if running
     */
    bool isRunning() const;
    
private:
    int m_cameraId;
    std::unique_ptr<CameraInterface> m_camera;
    
    // Detector parameters (from GlobalConfig)
    double m_minSize;
    double m_maxSize;
    double m_threshold;
    std::string m_tagFamily;
    
    // Detection results
    mutable std::mutex m_mutex;
    DetectionFrame m_lastFrame;
    int m_detectionCount;
    
    std::function<void(const DetectionFrame&)> m_callback;
    std::atomic<bool> m_running;
    
    /**
     * @brief Detect tags in a frame
     * @param frame Input frame
     * @return Vector of detections
     */
    std::vector<AprilTagDetection> detectTags(const CameraFrame& frame);
    
    /**
     * @brief Convert grayscale to binary threshold
     * @param frame Input grayscale frame
     * @param threshold Threshold value
     * @return Binary frame
     */
    std::vector<uint8_t> thresholdFrame(const std::vector<uint8_t>& frame, uint8_t threshold);
    
    /**
     * @brief Find tag candidates in binary image
     * @param binary Binary image
     * @param width Image width
     * @param height Image height
     * @return Vector of candidate regions
     */
    std::vector<std::vector<std::pair<int, int>>> findTagCandidates(
        const std::vector<uint8_t>& binary, int width, int height);
    
    /**
     * @brief Decode tag from image region
     * @param frame Frame data
     * @param width Image width
     * @param height Image height
     * @param corners Tag corners
     * @return Tag ID or -1 if not decoded
     */
    int decodeTag(const std::vector<uint8_t>& frame, int width, int height,
                  const std::vector<std::pair<int, int>>& corners);
    
    /**
     * @brief Calculate tag pose
     * @param detection Detection
     * @param intrinsics Camera intrinsics
     * @return true if pose calculated
     */
    bool calculateTagPose(AprilTagDetection& detection, 
                         const CameraIntrinsics& intrinsics);
    
    /**
     * @brief Estimate tag size from image
     * @param corners Tag corners
     * @param width Image width
     * @param height Image height
     * @return Estimated tag size in pixels
     */
    double estimateTagSize(const std::vector<std::pair<int, int>>& corners,
                          int width, int height);
};
