#pragma once

#include "../common/camera/CameraInterface.hpp"
#include "../camera_detector/AprilTagDetector.hpp"
#include <vector>
#include <map>
#include <mutex>

/**
 * @brief Known tag pose in world coordinates
 */
struct TagPose {
    std::vector<double> translation;  // [x, y, z] in meters
    std::vector<double> rotation;    // Quaternion [w, x, y, z]
    double size;                   // Tag size in meters
    
    TagPose() 
        : translation(3, 0.0), rotation(4, 0.0), size(GlobalConfig::TAG_SIZE) {
        rotation[0] = 1.0;  // Identity quaternion
    }
};

/**
 * @brief Pose estimation result
 */
struct PoseResult {
    std::vector<double> translation;  // [x, y, z] in meters
    std::vector<double> rotation;    // Quaternion [w, x, y, z]
    std::vector<double> euler;       // [roll, pitch, yaw] in radians
    double confidence;              // 0.0 to 1.0
    std::vector<int> usedCameraIds; // Which cameras contributed
    std::vector<int> usedTagIds;     // Which tags were used
    
    PoseResult() 
        : translation(3, 0.0), rotation(4, 0.0), euler(3, 0.0), confidence(0.0) {
        rotation[0] = 1.0;  // Identity quaternion
    }
};

/**
 * @brief Pose solver for estimating robot position from tag detections
 */
class PoseSolver {
public:
    
    /**
     * @brief Constructor
     */
    PoseSolver();
    
    /**
     * @brief Destructor
     */
    ~PoseSolver();
    
    /**
     * @brief Set known tag poses
     * @param tags Map of tag ID to TagPose
     */
    void setKnownTags(const std::map<int, TagPose>& tags);
    
    /**
     * @brief Add a known tag pose
     * @param tagId Tag ID
     * @param pose Tag pose
     */
    void addKnownTag(int tagId, const TagPose& pose);
    
    /**
     * @brief Solve for combined pose from all cameras
     * @param allDetections Map of camera ID to detections
     * @param cameraIntrinsics Map of camera ID to intrinsics
     * @param cameraExtrinsics Map of camera ID to extrinsics
     * @return PoseResult with combined pose
     */
    PoseResult solveCombinedPose(
        const std::map<int, std::vector<AprilTagDetection>>& allDetections,
        const std::map<int, CameraIntrinsics>& cameraIntrinsics,
        const std::map<int, CameraExtrinsics>& cameraExtrinsics);
    
    /**
     * @brief Solve pose from single camera detections
     * @param detections Detections from one camera
     * @param intrinsics Camera intrinsics
     * @param extrinsics Camera extrinsics
     * @return PoseResult for that camera
     */
    PoseResult solvePose(
        const std::vector<AprilTagDetection>& detections,
        const CameraIntrinsics& intrinsics,
        const CameraExtrinsics& extrinsics);
    
    /**
     * @brief Solve PnP (Perspective-n-Point) for a single tag
     * @param detection Tag detection
     * @param tagPose Known tag pose
     * @param intrinsics Camera intrinsics
     * @return Camera-to-tag transform (4x4 matrix)
     */
    std::vector<double> solvePnP(
        const AprilTagDetection& detection,
        const TagPose& tagPose,
        const CameraIntrinsics& intrinsics);
    
    /**
     * @brief Combine multiple poses using weighted averaging
     * @param poses Vector of poses with weights
     * @return Combined pose
     */
    PoseResult combinePoses(const std::vector<std::pair<PoseResult, double>>& poses);
    
    /**
     * @brief Get detection counts per camera
     * @param allDetections Map of camera ID to detections
     * @return Map of camera ID to detection count
     */
    std::map<int, int> getDetectionCounts(
        const std::map<int, std::vector<AprilTagDetection>>& allDetections);
    
    /**
     * @brief Set the default tag size
     * @param size Tag size in meters
     */
    void setDefaultTagSize(double size);
    
    /**
     * @brief Get the default tag size
     * @return Tag size in meters
     */
    double getDefaultTagSize() const;
    
private:
    std::map<int, TagPose> m_knownTags;
    double m_defaultTagSize;
    mutable std::mutex m_mutex;
    
    /**
     * @brief Convert quaternion to Euler angles
     * @param quaternion Quaternion [w, x, y, z]
     * @return Euler angles [roll, pitch, yaw] in radians
     */
    std::vector<double> quaternionToEuler(const std::vector<double>& quaternion) const;
    
    /**
     * @brief Convert Euler angles to quaternion
     * @param euler Euler angles [roll, pitch, yaw] in radians
     * @return Quaternion [w, x, y, z]
     */
    std::vector<double> eulerToQuaternion(const std::vector<double>& euler) const;
    
    /**
     * @brief Invert a 4x4 transform matrix
     * @param transform 4x4 matrix (column-major)
     * @return Inverted matrix
     */
    std::vector<double> invertTransform(const std::vector<double>& transform) const;
    
    /**
     * @brief Multiply two 4x4 matrices
     * @param a First matrix
     * @param b Second matrix
     * @return Result matrix
     */
    std::vector<double> multiplyTransforms(
        const std::vector<double>& a,
        const std::vector<double>& b) const;
    
    /**
     * @brief Calculate confidence from detection
     * @param detection Detection
     * @return Confidence value (0.0 to 1.0)
     */
    double calculateConfidence(const AprilTagDetection& detection) const;
};
