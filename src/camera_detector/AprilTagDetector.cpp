#include "AprilTagDetector.hpp"
#include "../common/config/GlobalConfig.hpp"
#include "../common/logging/Logger.hpp"
#include "../common/utils/TimeUtils.hpp"
#include <cmath>
#include <algorithm>

AprilTagDetector::AprilTagDetector(int cameraId)
    : m_cameraId(cameraId), m_minSize(GlobalConfig::MIN_TAG_SIZE),
      m_maxSize(GlobalConfig::MAX_TAG_SIZE), 
      m_threshold(GlobalConfig::DETECT_THRESHOLD),
      m_tagFamily(GlobalConfig::TAG_FAMILY),
      m_detectionCount(0), m_running(false) {
    
    // Register for global config changes
    GlobalConfig::registerChangeCallback([this]() {
        m_minSize = GlobalConfig::MIN_TAG_SIZE;
        m_maxSize = GlobalConfig::MAX_TAG_SIZE;
        m_threshold = GlobalConfig::DETECT_THRESHOLD;
        m_tagFamily = GlobalConfig::TAG_FAMILY;
        LOG_INFO_F("Camera %d: Updated detector parameters from GlobalConfig", m_cameraId);
    });
}

AprilTagDetector::~AprilTagDetector() {
    stopDetection();
}

bool AprilTagDetector::initialize(std::unique_ptr<CameraInterface> camera) {
    m_camera = std::move(camera);
    
    if (!m_camera || !m_camera->isOpen()) {
        LOG_ERROR_F("Camera %d: Failed to initialize - camera not open", m_cameraId);
        return false;
    }
    
    LOG_INFO_F("AprilTag detector initialized for camera %d (family: %s)", 
               m_cameraId, m_tagFamily.c_str());
    
    return true;
}

DetectionFrame AprilTagDetector::processFrame(const CameraFrame& frame) {
    DetectionFrame result;
    result.frame = frame;
    result.cameraId = m_cameraId;
    result.frameTimestamp = TimeUtils::getCurrentTimestampUs();
    
    if (!frame.isValid || frame.data.empty()) {
        return result;
    }
    
    // Detect tags
    result.detections = detectTags(frame);
    
    // Update last frame
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastFrame = result;
        m_detectionCount = static_cast<int>(result.detections.size());
    }
    
    return result;
}

void AprilTagDetector::startDetection(std::function<void(const DetectionFrame&)> callback) {
    if (m_running) {
        return;
    }
    
    m_callback = callback;
    m_running = true;
    
    LOG_INFO_F("Started AprilTag detection for camera %d", m_cameraId);
}

void AprilTagDetector::stopDetection() {
    m_running = false;
    m_callback = nullptr;
    
    LOG_INFO_F("Stopped AprilTag detection for camera %d", m_cameraId);
}

void AprilTagDetector::setCamera(std::unique_ptr<CameraInterface> camera) {
    m_camera = std::move(camera);
}

CameraInterface* AprilTagDetector::getCamera() {
    return m_camera.get();
}

int AprilTagDetector::getCameraId() const {
    return m_cameraId;
}

int AprilTagDetector::getDetectionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_detectionCount;
}

DetectionFrame AprilTagDetector::getLastDetectionFrame() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastFrame;
}

void AprilTagDetector::setParameters(double minSize, double maxSize, double threshold) {
    m_minSize = minSize;
    m_maxSize = maxSize;
    m_threshold = threshold;
}

void AprilTagDetector::getParameters(double& minSize, double& maxSize, double& threshold) const {
    minSize = m_minSize;
    maxSize = m_maxSize;
    threshold = m_threshold;
}

bool AprilTagDetector::isRunning() const {
    return m_running;
}

std::vector<AprilTagDetection> AprilTagDetector::detectTags(const CameraFrame& frame) {
    std::vector<AprilTagDetection> detections;
    
    if (frame.channels != 1 && frame.channels != 3) {
        LOG_WARN_F("Camera %d: Unsupported number of channels: %d", 
                   m_cameraId, frame.channels);
        return detections;
    }
    
    // Convert to grayscale if needed
    std::vector<uint8_t> grayFrame;
    if (frame.channels == 3) {
        grayFrame.resize(frame.width * frame.height);
        for (int y = 0; y < frame.height; ++y) {
            for (int x = 0; x < frame.width; ++x) {
                size_t idx = (y * frame.width + x) * 3;
                // Simple grayscale conversion
                uint8_t gray = static_cast<uint8_t>(
                    0.299 * frame.data[idx] + 
                    0.587 * frame.data[idx + 1] + 
                    0.114 * frame.data[idx + 2]
                );
                grayFrame[y * frame.width + x] = gray;
            }
        }
    } else {
        grayFrame = frame.data;
    }
    
    // Threshold the frame
    uint8_t thresholdValue = static_cast<uint8_t>(m_threshold * 255.0);
    std::vector<uint8_t> binary = thresholdFrame(grayFrame, thresholdValue);
    
    // Find tag candidates
    std::vector<std::vector<std::pair<int, int>>> candidates = 
        findTagCandidates(binary, frame.width, frame.height);
    
    // Process each candidate
    for (const auto& corners : candidates) {
        if (corners.size() != 4) {
            continue;
        }
        
        // Decode tag
        int tagId = decodeTag(frame.data, frame.width, frame.height, corners);
        
        if (tagId < 0) {
            continue;
        }
        
        // Create detection
        AprilTagDetection detection;
        detection.id = tagId;
        detection.timestamp = frame.timestamp;
        
        // Calculate center
        double centerX = 0.0, centerY = 0.0;
        for (const auto& corner : corners) {
            centerX += corner.first;
            centerY += corner.second;
        }
        centerX /= 4.0;
        centerY /= 4.0;
        detection.centerX = centerX;
        detection.centerY = centerY;
        
        // Copy corners
        for (int i = 0; i < 4; ++i) {
            detection.corners[i][0] = corners[i].first;
            detection.corners[i][1] = corners[i].second;
        }
        
        // Estimate tag size in pixels
        double pixelSize = estimateTagSize(corners, frame.width, frame.height);
        
        // Convert to meters (assuming tag fills the image at known size)
        // This is a simplified estimation
        CameraIntrinsics intrinsics;
        if (m_camera && m_camera->getIntrinsics(intrinsics)) {
            // Calculate focal length in pixels
            double focalPixels = (intrinsics.fx + intrinsics.fy) / 2.0;
            
            // Estimate distance using tag size
            // tagSizeMeters / pixelSize * focalPixels = distance
            double distance = GlobalConfig::TAG_SIZE / (pixelSize / focalPixels);
            
            // For now, use the configured tag size
            detection.tagSize = GlobalConfig::TAG_SIZE;
            detection.confidence = 0.8; // Placeholder
        } else {
            detection.tagSize = GlobalConfig::TAG_SIZE;
            detection.confidence = 0.5;
        }
        
        detections.push_back(detection);
        LOG_DEBUG_F("Camera %d: Detected tag %d at (%f, %f)", 
                    m_cameraId, tagId, centerX, centerY);
    }
    
    return detections;
}

std::vector<uint8_t> AprilTagDetector::thresholdFrame(
    const std::vector<uint8_t>& frame, uint8_t threshold) {
    
    std::vector<uint8_t> binary(frame.size());
    
    for (size_t i = 0; i < frame.size(); ++i) {
        binary[i] = (frame[i] >= threshold) ? 255 : 0;
    }
    
    return binary;
}

std::vector<std::vector<std::pair<int, int>>> AprilTagDetector::findTagCandidates(
    const std::vector<uint8_t>& binary, int width, int height) {
    
    std::vector<std::vector<std::pair<int, int>>> candidates;
    
    // Simple connected component analysis
    // This is a placeholder - a real implementation would use a proper algorithm
    
    // Look for black squares (tags are typically black border on white background)
    // For tag36h11, tags are 6x6 modules with black border
    
    // Simplified: look for 4-corner patterns
    // In a real implementation, this would use AprilTag library
    
    return candidates;
}

int AprilTagDetector::decodeTag(const std::vector<uint8_t>& frame, int width, int height,
                               const std::vector<std::pair<int, int>>& corners) {
    // Placeholder - in a real implementation, this would decode the tag
    // For tag36h11, we'd extract the 6x6 bit pattern and decode it
    
    // For now, return a dummy tag ID
    // In production, use the actual AprilTag library
    
    return 0; // Dummy tag ID
}

bool AprilTagDetector::calculateTagPose(AprilTagDetection& detection, 
                                      const CameraIntrinsics& intrinsics) {
    // Placeholder - in a real implementation, this would calculate the tag pose
    // using PnP (Perspective-n-Point) algorithm
    
    // For now, just return true
    return true;
}

double AprilTagDetector::estimateTagSize(
    const std::vector<std::pair<int, int>>& corners, int width, int height) {
    
    if (corners.size() < 2) {
        return 0.0;
    }
    
    // Calculate average distance between corners
    double totalDistance = 0.0;
    int count = 0;
    
    for (size_t i = 0; i < corners.size(); ++i) {
        for (size_t j = i + 1; j < corners.size(); ++j) {
            double dx = corners[i].first - corners[j].first;
            double dy = corners[i].second - corners[j].second;
            totalDistance += sqrt(dx * dx + dy * dy);
            count++;
        }
    }
    
    if (count == 0) {
        return 0.0;
    }
    
    // Average distance between adjacent corners
    // For a square tag, the side length would be approximately this
    double avgDistance = totalDistance / count;
    
    // For a tag36h11, the tag is 6x6 modules
    // The side length in pixels is approximately the distance between adjacent corners
    return avgDistance / 6.0; // Approximate module size
}
