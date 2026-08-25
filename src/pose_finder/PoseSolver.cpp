#include "PoseSolver.hpp"
#include "../common/config/GlobalConfig.hpp"
#include "../common/logging/Logger.hpp"
#include <cmath>
#include <algorithm>

PoseSolver::PoseSolver() : m_defaultTagSize(GlobalConfig::TAG_SIZE) {
}

PoseSolver::~PoseSolver() {
}

void PoseSolver::setKnownTags(const std::map<int, TagPose>& tags) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_knownTags = tags;
}

void PoseSolver::addKnownTag(int tagId, const TagPose& pose) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_knownTags[tagId] = pose;
}

PoseResult PoseSolver::solveCombinedPose(
    const std::map<int, std::vector<AprilTagDetection>>& allDetections,
    const std::map<int, CameraIntrinsics>& cameraIntrinsics,
    const std::map<int, CameraExtrinsics>& cameraExtrinsics) {
    
    PoseResult result;
    std::vector<std::pair<PoseResult, double>> cameraPoses;
    
    // Solve pose for each camera
    for (const auto& cameraPair : allDetections) {
        int cameraId = cameraPair.first;
        const auto& detections = cameraPair.second;
        
        if (detections.empty()) {
            continue;
        }
        
        auto intrinsicsIt = cameraIntrinsics.find(cameraId);
        auto extrinsicsIt = cameraExtrinsics.find(cameraId);
        
        if (intrinsicsIt == cameraIntrinsics.end() || 
            extrinsicsIt == cameraExtrinsics.end()) {
            LOG_WARN_F("Missing intrinsics or extrinsics for camera %d", cameraId);
            continue;
        }
        
        PoseResult cameraPose = solvePose(detections, intrinsicsIt->second, extrinsicsIt->second);
        
        if (cameraPose.confidence > 0.1) {
            // Use detection count as weight (more detections = more confident)
            double weight = static_cast<double>(detections.size());
            cameraPoses.emplace_back(cameraPose, weight);
        }
    }
    
    if (cameraPoses.empty()) {
        return result;
    }
    
    // Combine all camera poses
    result = combinePoses(cameraPoses);
    
    return result;
}

PoseResult PoseSolver::solvePose(
    const std::vector<AprilTagDetection>& detections,
    const CameraIntrinsics& intrinsics,
    const CameraExtrinsics& extrinsics) {
    
    PoseResult result;
    std::vector<std::pair<std::vector<double>, double>> tagPoses;
    
    for (const auto& detection : detections) {
        auto tagIt = m_knownTags.find(detection.id);
        if (tagIt == m_knownTags.end()) {
            LOG_WARN_F("Unknown tag ID: %d", detection.id);
            continue;
        }
        
        const TagPose& tagPose = tagIt->second;
        
        // Solve PnP for this tag
        std::vector<double> cameraToTag = solvePnP(detection, tagPose, intrinsics);
        
        if (cameraToTag.empty()) {
            continue;
        }
        
        // Convert camera-to-tag to robot-to-tag
        // Camera extrinsics is robot-to-camera
        // So robot-to-tag = robot-to-camera * camera-to-tag
        std::vector<double> robotToCamera = extrinsics.rotation;
        robotToCamera.insert(robotToCamera.end(), extrinsics.translation.begin(), 
                           extrinsics.translation.end());
        // Add last row
        robotToCamera.push_back(0.0);
        robotToCamera.push_back(0.0);
        robotToCamera.push_back(0.0);
        robotToCamera.push_back(1.0);
        
        std::vector<double> robotToTag = multiplyTransforms(robotToCamera, cameraToTag);
        
        // Extract position from robot-to-tag
        std::vector<double> position = {
            robotToTag[12], robotToTag[13], robotToTag[14]
        };
        
        // Extract rotation from robot-to-tag (first 4 elements of 4x4 matrix)
        std::vector<double> rotation = {
            robotToTag[0], robotToTag[1], robotToTag[2], robotToTag[3]
        };
        
        double confidence = calculateConfidence(detection);
        tagPoses.emplace_back(position, confidence);
    }
    
    if (tagPoses.empty()) {
        return result;
    }
    
    // Average all tag positions
    double totalWeight = 0.0;
    for (const auto& pose : tagPoses) {
        result.translation[0] += pose.first[0] * pose.second;
        result.translation[1] += pose.first[1] * pose.second;
        result.translation[2] += pose.first[2] * pose.second;
        totalWeight += pose.second;
    }
    
    if (totalWeight > 0) {
        result.translation[0] /= totalWeight;
        result.translation[1] /= totalWeight;
        result.translation[2] /= totalWeight;
        result.confidence = totalWeight / static_cast<double>(tagPoses.size());
    }
    
    // For now, use identity rotation
    result.rotation = {1.0, 0.0, 0.0, 0.0};
    result.euler = {0.0, 0.0, 0.0};
    
    return result;
}

std::vector<double> PoseSolver::solvePnP(
    const AprilTagDetection& detection,
    const TagPose& tagPose,
    const CameraIntrinsics& intrinsics) {
    
    // Simplified PnP implementation
    // In a real implementation, this would use OpenCV or a custom PnP solver
    
    // For now, we'll estimate the pose based on the tag center and size
    
    // Create a 4x4 identity matrix
    std::vector<double> transform(16, 0.0);
    for (int i = 0; i < 4; ++i) {
        transform[i * 4 + i] = 1.0;
    }
    
    // Estimate distance using tag size
    // tagSizeMeters / tagSizePixels * focalLengthPixels = distance
    double focalLength = (intrinsics.fx + intrinsics.fy) / 2.0;
    
    // Calculate tag size in pixels
    double tagSizePixels = 0.0;
    for (int i = 0; i < 4; ++i) {
        int j = (i + 1) % 4;
        double dx = detection.corners[i][0] - detection.corners[j][0];
        double dy = detection.corners[i][1] - detection.corners[j][1];
        tagSizePixels += std::sqrt(dx * dx + dy * dy);
    }
    tagSizePixels /= 4.0; // Average side length
    
    // For tag36h11, the tag has 6 modules per side
    // So module size in pixels is tagSizePixels / 6
    double moduleSizePixels = tagSizePixels / 6.0;
    
    // Use the configured tag size
    double tagSizeMeters = GlobalConfig::TAG_SIZE;
    
    // Estimate distance
    double distance = (tagSizeMeters * focalLength) / tagSizePixels;
    
    // Set translation (camera is looking at the tag)
    // The tag is at distance along the Z axis
    transform[12] = 0.0;  // X
    transform[13] = 0.0;  // Y
    transform[14] = -distance;  // Z (negative because camera looks along -Z)
    
    return transform;
}

PoseResult PoseSolver::combinePoses(
    const std::vector<std::pair<PoseResult, double>>& poses) {
    
    PoseResult result;
    
    if (poses.empty()) {
        return result;
    }
    
    // Weighted average of positions
    double totalWeight = 0.0;
    for (const auto& pose : poses) {
        double weight = pose.second * pose.first.confidence;
        result.translation[0] += pose.first.translation[0] * weight;
        result.translation[1] += pose.first.translation[1] * weight;
        result.translation[2] += pose.first.translation[2] * weight;
        totalWeight += weight;
    }
    
    if (totalWeight > 0) {
        result.translation[0] /= totalWeight;
        result.translation[1] /= totalWeight;
        result.translation[2] /= totalWeight;
        result.confidence = totalWeight / static_cast<double>(poses.size());
    }
    
    // For now, use identity rotation
    result.rotation = {1.0, 0.0, 0.0, 0.0};
    result.euler = {0.0, 0.0, 0.0};
    
    return result;
}

std::map<int, int> PoseSolver::getDetectionCounts(
    const std::map<int, std::vector<AprilTagDetection>>& allDetections) {
    
    std::map<int, int> counts;
    
    for (const auto& pair : allDetections) {
        counts[pair.first] = static_cast<int>(pair.second.size());
    }
    
    return counts;
}

void PoseSolver::setDefaultTagSize(double size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_defaultTagSize = size;
}

double PoseSolver::getDefaultTagSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_defaultTagSize;
}

std::vector<double> PoseSolver::quaternionToEuler(const std::vector<double>& quaternion) const {
    std::vector<double> euler(3, 0.0);
    
    if (quaternion.size() < 4) {
        return euler;
    }
    
    double w = quaternion[0], x = quaternion[1], y = quaternion[2], z = quaternion[3];
    
    // Roll (x-axis rotation)
    double sinr_cosp = 2.0 * (w * x + y * z);
    double cosr_cosp = 1.0 - 2.0 * (x * x + y * y);
    euler[0] = std::atan2(sinr_cosp, cosr_cosp);
    
    // Pitch (y-axis rotation)
    double sinp = 2.0 * (w * y - z * x);
    if (std::abs(sinp) >= 1.0) {
        euler[1] = std::copysign(M_PI / 2.0, sinp);
    } else {
        euler[1] = std::asin(sinp);
    }
    
    // Yaw (z-axis rotation)
    double siny_cosp = 2.0 * (w * z + x * y);
    double cosy_cosp = 1.0 - 2.0 * (y * y + z * z);
    euler[2] = std::atan2(siny_cosp, cosy_cosp);
    
    return euler;
}

std::vector<double> PoseSolver::eulerToQuaternion(const std::vector<double>& euler) const {
    std::vector<double> quaternion(4, 0.0);
    
    if (euler.size() < 3) {
        quaternion[0] = 1.0;
        return quaternion;
    }
    
    double roll = euler[0], pitch = euler[1], yaw = euler[2];
    
    double cy = std::cos(yaw * 0.5);
    double sy = std::sin(yaw * 0.5);
    double cp = std::cos(pitch * 0.5);
    double sp = std::sin(pitch * 0.5);
    double cr = std::cos(roll * 0.5);
    double sr = std::sin(roll * 0.5);
    
    quaternion[0] = cr * cp * cy + sr * sp * sy;
    quaternion[1] = sr * cp * cy - cr * sp * sy;
    quaternion[2] = cr * sp * cy + sr * cp * sy;
    quaternion[3] = cr * cp * sy - sr * sp * cy;
    
    return quaternion;
}

std::vector<double> PoseSolver::invertTransform(const std::vector<double>& transform) const {
    // Invert a 4x4 transform matrix
    // This is a simplified implementation
    
    if (transform.size() < 16) {
        return std::vector<double>();
    }
    
    std::vector<double> result(16, 0.0);
    
    // Extract rotation and translation
    std::vector<double> rotation(9);
    for (int i = 0; i < 9; ++i) {
        rotation[i] = transform[i];
    }
    
    std::vector<double> translation(3);
    for (int i = 0; i < 3; ++i) {
        translation[i] = transform[12 + i];
    }
    
    // Transpose rotation (for orthogonal matrices, inverse = transpose)
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            result[row * 4 + col] = rotation[col * 3 + row];
        }
    }
    
    // Calculate -R^T * t
    for (int row = 0; row < 3; ++row) {
        double sum = 0.0;
        for (int col = 0; col < 3; ++col) {
            sum += rotation[col * 3 + row] * translation[col];
        }
        result[12 + row] = -sum;
    }
    
    // Last row
    result[15] = 1.0;
    
    return result;
}

std::vector<double> PoseSolver::multiplyTransforms(
    const std::vector<double>& a,
    const std::vector<double>& b) const {
    
    if (a.size() < 16 || b.size() < 16) {
        return std::vector<double>();
    }
    
    std::vector<double> result(16, 0.0);
    
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            for (int k = 0; k < 4; ++k) {
                result[row * 4 + col] += a[row * 4 + k] * b[k * 4 + col];
            }
        }
    }
    
    return result;
}

double PoseSolver::calculateConfidence(const AprilTagDetection& detection) const {
    // Calculate confidence based on detection quality
    // This is a placeholder - in a real implementation, this would use
    // the detection algorithm's confidence metric
    
    // For now, use a fixed confidence
    return 0.8;
}
