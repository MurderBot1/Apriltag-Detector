#pragma once

#include "../common/camera/CameraInterface.hpp"
#include <string>

/**
 * @brief Configuration for a camera detector
 */
struct DetectorConfig {
    int cameraId;
    std::string devicePath;
    std::string cameraType;
    CameraIntrinsics intrinsics;
    CameraExtrinsics extrinsics;
    CameraSettings settings;
    
    // AprilTag detection parameters
    std::string tagFamily;
    double minTagSize;
    double maxTagSize;
    double detectThreshold;
    
    DetectorConfig() 
        : cameraId(0), 
          devicePath("/dev/video0"),
          cameraType("v4l2"),
          minTagSize(0.05),
          maxTagSize(10.0),
          detectThreshold(0.5),
          tagFamily("tag36h11") {
    }
};
