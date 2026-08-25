#include "AVCaptureCamera.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/TimeUtils.hpp"
#include "../logging/Logger.hpp"

AVCaptureCamera::AVCaptureCamera(int cameraId) 
    : m_cameraId(cameraId), m_width(1280), m_height(720), 
      m_fps(30), m_captureSession(nullptr), m_captureDevice(nullptr),
      m_captureInput(nullptr), m_captureOutput(nullptr), m_captureQueue(nullptr),
      m_capturing(false), m_running(false) {
    
    m_devicePath = "Camera " + std::to_string(cameraId);
    
    // Default intrinsics
    m_intrinsics.fx = 800.0;
    m_intrinsics.fy = 800.0;
    m_intrinsics.cx = 640.0;
    m_intrinsics.cy = 360.0;
    m_intrinsics.distortion = {0.0, 0.0, 0.0, 0.0, 0.0};
    
    // Default extrinsics
    m_extrinsics.translation = {0.0, 0.0, 0.0};
    m_extrinsics.rotation = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    
    // Default settings
    m_settings.exposure = 50;
    m_settings.brightness = 50;
    m_settings.contrast = 50;
    m_settings.saturation = 50;
    m_settings.gain = 0;
    m_settings.fps = 30;
    
    // Create capture queue
    m_captureQueue = dispatch_queue_create("com.apriltag.camera.capture", DISPATCH_QUEUE_SERIAL);
}

AVCaptureCamera::~AVCaptureCamera() {
    close();
    
    if (m_captureQueue) {
        dispatch_release(m_captureQueue);
        m_captureQueue = nullptr;
    }
}

bool AVCaptureCamera::open(int deviceIndex, const std::string& configPath) {
    if (isOpen()) {
        close();
    }
    
    if (deviceIndex >= 0) {
        m_cameraId = deviceIndex;
    }
    
    // Setup capture session
    if (!setupCaptureSession()) {
        return false;
    }
    
    // Configure device
    if (!configureDevice()) {
        close();
        return false;
    }
    
    // Start capture session
    if (!startCaptureSession()) {
        close();
        return false;
    }
    
    LOG_INFO_F("Opened AVCapture camera: %s (%dx%d @ %dfps)", 
               m_devicePath.c_str(), m_width, m_height, m_fps);
    
    return true;
}

void AVCaptureCamera::close() {
    m_capturing = false;
    m_running = false;
    
    stopCaptureSession();
    
    LOG_INFO_F("Closed AVCapture camera: %s", m_devicePath.c_str());
}

bool AVCaptureCamera::isOpen() const {
    return m_captureSession != nullptr && m_captureSession.running;
}

bool AVCaptureCamera::captureFrame(CameraFrame& frame) {
    if (!isOpen()) {
        return false;
    }
    
    // In async mode, we need to wait for a frame
    // This is a simplified implementation
    // In real implementation, we'd have a frame buffer
    
    return false;
}

void AVCaptureCamera::startCapture(std::function<void(const CameraFrame&)> callback) {
    if (!isOpen()) {
        return;
    }
    
    if (m_capturing) {
        stopCapture();
    }
    
    m_callback = callback;
    m_capturing = true;
    m_running = true;
}

void AVCaptureCamera::stopCapture() {
    m_capturing = false;
    m_running = false;
    m_callback = nullptr;
}

bool AVCaptureCamera::setResolution(int width, int height) {
    if (!isOpen()) {
        return false;
    }
    
    m_width = width;
    m_height = height;
    
    // Would need to reconfigure session
    // Simplified implementation
    
    return true;
}

bool AVCaptureCamera::setFPS(int fps) {
    if (!isOpen()) {
        return false;
    }
    
    m_fps = fps;
    m_settings.fps = fps;
    
    // Would need to reconfigure device
    // Simplified implementation
    
    return true;
}

bool AVCaptureCamera::setExposure(int value) {
    if (!isOpen() || !m_captureDevice) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    
    // Convert 0-100 to exposure duration
    // This is camera-specific
    float minExposure = m_captureDevice.activeFormat.minExposureDuration.value;
    float maxExposure = m_captureDevice.activeFormat.maxExposureDuration.value;
    float exposure = minExposure + (maxExposure - minExposure) * (clamped / 100.0f);
    
    NSError* error = nullptr;
    if ([m_captureDevice lockForConfiguration:&error]) {
        [m_captureDevice setExposureDuration:CMTimeMakeWithSeconds(exposure, 1000000000) completionHandler:nil];
        [m_captureDevice unlockForConfiguration];
        m_settings.exposure = clamped;
        return true;
    }
    
    return false;
}

bool AVCaptureCamera::setBrightness(int value) {
    if (!isOpen() || !m_captureDevice) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    m_settings.brightness = clamped;
    return true;
}

bool AVCaptureCamera::setContrast(int value) {
    if (!isOpen() || !m_captureDevice) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    m_settings.contrast = clamped;
    return true;
}

bool AVCaptureCamera::setSaturation(int value) {
    if (!isOpen() || !m_captureDevice) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    m_settings.saturation = clamped;
    return true;
}

bool AVCaptureCamera::setGain(int value) {
    if (!isOpen() || !m_captureDevice) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    m_settings.gain = clamped;
    return true;
}

bool AVCaptureCamera::getIntrinsics(CameraIntrinsics& intrinsics) const {
    intrinsics = m_intrinsics;
    return true;
}

bool AVCaptureCamera::setIntrinsics(const CameraIntrinsics& intrinsics) {
    m_intrinsics = intrinsics;
    return true;
}

bool AVCaptureCamera::getExtrinsics(CameraExtrinsics& extrinsics) const {
    extrinsics = m_extrinsics;
    return true;
}

bool AVCaptureCamera::setExtrinsics(const CameraExtrinsics& extrinsics) {
    m_extrinsics = extrinsics;
    return true;
}

bool AVCaptureCamera::getSettings(CameraSettings& settings) const {
    settings = m_settings;
    return true;
}

bool AVCaptureCamera::setSettings(const CameraSettings& settings) {
    m_settings = settings;
    return true;
}

int AVCaptureCamera::getCameraId() const {
    return m_cameraId;
}

std::string AVCaptureCamera::getDevicePath() const {
    return m_devicePath;
}

std::string AVCaptureCamera::getCameraType() const {
    return "avfoundation";
}

void AVCaptureCamera::getResolution(int& width, int& height) const {
    width = m_width;
    height = m_height;
}

int AVCaptureCamera::getFPS() const {
    return m_fps;
}

bool AVCaptureCamera::supportsSetting(const std::string& setting) const {
    std::string lowerSetting = setting;
    for (char& c : lowerSetting) {
        c = static_cast<char>(tolower(c));
    }
    
    // AVFoundation supports most settings
    return true;
}

bool AVCaptureCamera::setupCaptureSession() {
    // Create capture session
    m_captureSession = [[AVCaptureSession alloc] init];
    if (!m_captureSession) {
        LOG_ERROR("Failed to create AVCaptureSession");
        return false;
    }
    
    // Set session presets
    if ([m_captureSession canSetSessionPreset:AVCaptureSessionPreset1280x720]) {
        [m_captureSession setSessionPreset:AVCaptureSessionPreset1280x720];
    } else if ([m_captureSession canSetSessionPreset:AVCaptureSessionPresetHigh]) {
        [m_captureSession setSessionPreset:AVCaptureSessionPresetHigh];
    }
    
    return true;
}

bool AVCaptureCamera::configureDevice() {
    // Get default video device
    NSArray* devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    
    if (devices.count == 0) {
        LOG_ERROR("No video capture devices found");
        return false;
    }
    
    // Use specified camera or first available
    if (m_cameraId >= 0 && m_cameraId < static_cast<int>(devices.count)) {
        m_captureDevice = [devices objectAtIndex:m_cameraId];
    } else {
        m_captureDevice = [devices firstObject];
    }
    
    if (!m_captureDevice) {
        LOG_ERROR("Failed to get capture device");
        return false;
    }
    
    m_devicePath = [[m_captureDevice localizedName] UTF8String];
    
    // Configure format
    NSError* error = nullptr;
    if ([m_captureDevice lockForConfiguration:&error]) {
        // Set format
        AVCaptureDeviceFormat* bestFormat = nil;
        for (AVCaptureDeviceFormat* format in [m_captureDevice formats]) {
            CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
            if (dims.width == m_width && dims.height == m_height) {
                bestFormat = format;
                break;
            }
        }
        
        if (bestFormat) {
            [m_captureDevice setActiveFormat:bestFormat];
        }
        
        // Set frame rate
        [m_captureDevice setActiveVideoMinFrameDuration:CMTimeMake(1, m_fps)];
        [m_captureDevice setActiveVideoMaxFrameDuration:CMTimeMake(1, m_fps)];
        
        [m_captureDevice unlockForConfiguration];
    } else {
        LOG_WARN_F("Failed to lock device for configuration: %s", 
                   [[error localizedDescription] UTF8String]);
    }
    
    // Create input
    error = nullptr;
    m_captureInput = [AVCaptureDeviceInput deviceInputWithDevice:m_captureDevice error:&error];
    if (!m_captureInput) {
        LOG_ERROR_F("Failed to create capture input: %s", 
                    [[error localizedDescription] UTF8String]);
        return false;
    }
    
    // Add input to session
    if ([m_captureSession canAddInput:m_captureInput]) {
        [m_captureSession addInput:m_captureInput];
    } else {
        LOG_ERROR("Cannot add input to session");
        return false;
    }
    
    // Create output
    m_captureOutput = [[AVCaptureVideoDataOutput alloc] init];
    if (!m_captureOutput) {
        LOG_ERROR("Failed to create capture output");
        return false;
    }
    
    // Set pixel format
    [m_captureOutput setVideoSettings:@{(id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)}];
    
    // Add output to session
    if ([m_captureSession canAddOutput:m_captureOutput]) {
        [m_captureSession addOutput:m_captureOutput];
    } else {
        LOG_ERROR("Cannot add output to session");
        return false;
    }
    
    // Set delegate
    [m_captureOutput setSampleBufferDelegate:(id<AVCaptureVideoDataOutputSampleBufferDelegate>)self 
                                          queue:m_captureQueue];
    
    return true;
}

bool AVCaptureCamera::startCaptureSession() {
    NSError* error = nullptr;
    [m_captureSession startRunning];
    
    if (error) {
        LOG_ERROR_F("Failed to start capture session: %s", 
                    [[error localizedDescription] UTF8String]);
        return false;
    }
    
    return true;
}

void AVCaptureCamera::stopCaptureSession() {
    if (m_captureSession && m_captureSession.running) {
        [m_captureSession stopRunning];
    }
    
    // Cleanup
    if (m_captureOutput) {
        [m_captureOutput setSampleBufferDelegate:nil queue:nullptr];
        [m_captureOutput release];
        m_captureOutput = nullptr;
    }
    
    if (m_captureInput) {
        [m_captureSession removeInput:m_captureInput];
        [m_captureInput release];
        m_captureInput = nullptr;
    }
    
    if (m_captureDevice) {
        [m_captureDevice release];
        m_captureDevice = nullptr;
    }
    
    if (m_captureSession) {
        [m_captureSession release];
        m_captureSession = nullptr;
    }
}

void AVCaptureCamera::captureOutput(AVCaptureOutput* output, AVCaptureConnection* connection, 
                                  id sampleBuffer, NSError* error) {
    if (!m_capturing || !m_callback) {
        return;
    }
    
    CameraFrame frame;
    frame.width = m_width;
    frame.height = m_height;
    frame.channels = 3;  // RGB24
    frame.timestamp = TimeUtils::getCurrentTimestampUs();
    frame.isValid = false;
    
    processSampleBuffer((CMSampleBufferRef)sampleBuffer, frame);
    
    if (frame.isValid && m_callback) {
        m_callback(frame);
    }
}

void AVCaptureCamera::processSampleBuffer(CMSampleBufferRef sampleBuffer, CameraFrame& frame) {
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!imageBuffer) {
        return;
    }
    
    CVPixelBufferRef pixelBuffer = CVImageBufferGetCVPixelBuffer(imageBuffer);
    if (!pixelBuffer) {
        return;
    }
    
    // Get dimensions
    frame.width = static_cast<int>(CVPixelBufferGetWidth(pixelBuffer));
    frame.height = static_cast<int>(CVPixelBufferGetHeight(pixelBuffer));
    
    // Convert to RGB24
    convertToRGB24(pixelBuffer, frame);
    frame.isValid = true;
}

void AVCaptureCamera::convertToRGB24(CVPixelBufferRef pixelBuffer, CameraFrame& frame) {
    // Lock pixel buffer
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    
    void* baseAddress = CVPixelBufferGetBaseAddress(pixelBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    
    if (!baseAddress) {
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
        return;
    }
    
    // Get pixel format
    OSType pixelFormat = CVPixelBufferGetPixelFormatType(pixelBuffer);
    
    frame.data.resize(frame.width * frame.height * 3);
    
    if (pixelFormat == kCVPixelFormatType_32BGRA) {
        // Convert from BGRA to RGB24
        uint8_t* src = static_cast<uint8_t*>(baseAddress);
        uint8_t* dst = frame.data.data();
        
        for (int y = 0; y < frame.height; ++y) {
            for (int x = 0; x < frame.width; ++x) {
                // BGRA to RGB
                dst[0] = src[2];  // R
                dst[1] = src[1];  // G
                dst[2] = src[0];  // B
                
                src += 4;
                dst += 3;
            }
            src += bytesPerRow - frame.width * 4;
        }
    } else if (pixelFormat == kCVPixelFormatType_24RGB) {
        // Already RGB24
        uint8_t* src = static_cast<uint8_t*>(baseAddress);
        uint8_t* dst = frame.data.data();
        
        for (int y = 0; y < frame.height; ++y) {
            memcpy(dst, src, frame.width * 3);
            src += bytesPerRow;
            dst += frame.width * 3;
        }
    }
    
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
}
