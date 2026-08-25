#pragma once

#include "CameraInterface.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

// macOS AVFoundation headers
#import <AVFoundation/AVFoundation.h>

/**
 * @brief AVFoundation Camera implementation for macOS
 */
class AVCaptureCamera : public CameraInterface {
public:
    AVCaptureCamera(int cameraId = 0);
    ~AVCaptureCamera() override;
    
    // CameraInterface implementation
    bool open(int deviceIndex = 0, const std::string& configPath = "") override;
    void close() override;
    bool isOpen() const override;
    bool captureFrame(CameraFrame& frame) override;
    void startCapture(std::function<void(const CameraFrame&)> callback) override;
    void stopCapture() override;
    bool setResolution(int width, int height) override;
    bool setFPS(int fps) override;
    bool setExposure(int value) override;
    bool setBrightness(int value) override;
    bool setContrast(int value) override;
    bool setSaturation(int value) override;
    bool setGain(int value) override;
    bool getIntrinsics(CameraIntrinsics& intrinsics) const override;
    bool setIntrinsics(const CameraIntrinsics& intrinsics) override;
    bool getExtrinsics(CameraExtrinsics& extrinsics) const override;
    bool setExtrinsics(const CameraExtrinsics& extrinsics) override;
    bool getSettings(CameraSettings& settings) const override;
    bool setSettings(const CameraSettings& settings) override;
    int getCameraId() const override;
    std::string getDevicePath() const override;
    std::string getCameraType() const override;
    void getResolution(int& width, int& height) const override;
    int getFPS() const override;
    bool supportsSetting(const std::string& setting) const override;
    
private:
    int m_cameraId;
    std::string m_devicePath;
    int m_width;
    int m_height;
    int m_fps;
    CameraIntrinsics m_intrinsics;
    CameraExtrinsics m_extrinsics;
    CameraSettings m_settings;
    
    // AVFoundation objects
    AVCaptureSession* m_captureSession;
    AVCaptureDevice* m_captureDevice;
    AVCaptureDeviceInput* m_captureInput;
    AVCaptureVideoDataOutput* m_captureOutput;
    dispatch_queue_t m_captureQueue;
    
    std::function<void(const CameraFrame&)> m_callback;
    std::atomic<bool> m_capturing;
    std::atomic<bool> m_running;
    
    /**
     * @brief Setup capture session
     * @return true if successful
     */
    bool setupCaptureSession();
    
    /**
     * @brief Configure capture device
     * @return true if successful
     */
    bool configureDevice();
    
    /**
     * @brief Start capture session
     * @return true if successful
     */
    bool startCaptureSession();
    
    /**
     * @brief Stop capture session
     */
    void stopCaptureSession();
    
    /**
     * @brief Capture callback
     * @param sampleBuffer Sample buffer from capture
     */
    void captureOutput(AVCaptureOutput* output, AVCaptureConnection* connection, 
                      id sampleBuffer, NSError* error);
    
    /**
     * @brief Process CMSampleBuffer to CameraFrame
     * @param sampleBuffer Sample buffer
     * @param frame Output frame
     */
    void processSampleBuffer(CMSampleBufferRef sampleBuffer, CameraFrame& frame);
    
    /**
     * @brief Convert CVPixelBuffer to RGB24
     * @param pixelBuffer Pixel buffer
     * @param frame Output frame
     */
    void convertToRGB24(CVPixelBufferRef pixelBuffer, CameraFrame& frame);
};
