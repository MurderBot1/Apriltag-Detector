#include "CameraFactory.hpp"
#include "CameraInterface.hpp"
#include "../utils/Platform.hpp"
#include "../utils/FileSystem.hpp"
#include <vector>
#include <memory>

// Forward declarations for platform-specific cameras
class V4L2Camera;
class WMFCamera;
class AVCaptureCamera;

std::unique_ptr<CameraInterface> CameraFactory::create(const std::string& type, int cameraId) {
    std::string lowerType = type;
    for (char& c : lowerType) {
        c = static_cast<char>(tolower(c));
    }
    
    if (lowerType == "v4l2" || lowerType == "linux") {
        #if PLATFORM_LINUX
            return std::make_unique<V4L2Camera>(cameraId);
        #else
            return nullptr;
        #endif
    } else if (lowerType == "wmf" || lowerType == "windows") {
        #if PLATFORM_WINDOWS
            return std::make_unique<WMFCamera>(cameraId);
        #else
            return nullptr;
        #endif
    } else if (lowerType == "avfoundation" || lowerType == "avcapture" || lowerType == "macos" || lowerType == "mac") {
        #if PLATFORM_MACOS
            return std::make_unique<AVCaptureCamera>(cameraId);
        #else
            return nullptr;
        #endif
    } else if (lowerType == "auto" || lowerType.empty()) {
        return createAuto(cameraId);
    }
    
    return nullptr;
}

std::unique_ptr<CameraInterface> CameraFactory::createAuto(int cameraId) {
    #if PLATFORM_LINUX
        return std::make_unique<V4L2Camera>(cameraId);
    #elif PLATFORM_WINDOWS
        return std::make_unique<WMFCamera>(cameraId);
    #elif PLATFORM_MACOS
        return std::make_unique<AVCaptureCamera>(cameraId);
    #else
        return nullptr;
    #endif
}

std::vector<std::string> CameraFactory::getAvailableTypes() {
    std::vector<std::string> types;
    
    #if PLATFORM_LINUX
        types.push_back("v4l2");
    #endif
    
    #if PLATFORM_WINDOWS
        types.push_back("wmf");
    #endif
    
    #if PLATFORM_MACOS
        types.push_back("avfoundation");
    #endif
    
    types.push_back("auto");
    return types;
}

std::vector<std::string> CameraFactory::listCameras() {
    std::vector<std::string> cameras;
    
    #if PLATFORM_LINUX
        // List V4L2 cameras
        for (int i = 0; i < 10; ++i) {
            std::string path = "/dev/video" + std::to_string(i);
            if (FileSystem::exists(path)) {
                cameras.push_back(path);
            }
        }
    #elif PLATFORM_WINDOWS
        // On Windows, we can't easily list cameras without MF
        cameras.push_back("Camera 0 (auto-detected)");
        cameras.push_back("Camera 1 (auto-detected)");
    #elif PLATFORM_MACOS
        // On macOS, we can't easily list cameras without AVFoundation
        cameras.push_back("Default Camera");
    #else
        cameras.push_back("Unknown platform");
    #endif
    
    return cameras;
}
