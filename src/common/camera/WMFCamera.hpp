#pragma once

#include "CameraInterface.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

// Windows Media Foundation headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfcaptureengine.h>

/**
 * @brief WMF Camera implementation for Windows
 */
class WMFCamera : public CameraInterface {
public:
    WMFCamera(int cameraId = 0);
    ~WMFCamera() override;
    
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
    
    // MF objects
    IMFAttributes* m_pAttributes;
    IMFActivate** m_pDevices;
    UINT32 m_deviceCount;
    IMFSourceReader* m_pReader;
    IMFMediaType* m_pMediaType;
    
    std::function<void(const CameraFrame&)> m_callback;
    std::thread m_captureThread;
    std::atomic<bool> m_capturing;
    std::atomic<bool> m_running;
    
    /**
     * @brief Initialize MF
     * @return true if successful
     */
    bool initMF();
    
    /**
     * @brief Initialize camera
     * @return true if successful
     */
    bool initCamera();
    
    /**
     * @brief Select camera device
     * @param index Device index
     * @return true if successful
     */
    bool selectDevice(UINT32 index);
    
    /**
     * @brief Setup media type
     * @return true if successful
     */
    bool setupMediaType();
    
    /**
     * @brief Capture thread function
     */
    void captureThread();
    
    /**
     * @brief Process sample
     * @param pSample Sample to process
     */
    void processSample(IMFSample* pSample);
    
    /**
     * @brief Convert MF pixel format to RGB24
     * @param pData Input data
     * @param width Width
     * @param height Height
     * @param stride Stride
     * @param frame Output frame
     */
    void convertToRGB24(BYTE* pData, int width, int height, int stride, CameraFrame& frame);
    
    /**
     * @brief Release all MF objects
     */
    void releaseMFObjects();
};
