#pragma once

#include "CameraInterface.hpp"
#include <string>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

/**
 * @brief V4L2 Camera implementation for Linux
 */
class V4L2Camera : public CameraInterface {
public:
    V4L2Camera(int cameraId = 0);
    ~V4L2Camera() override;
    
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
    int m_fd;
    std::string m_devicePath;
    int m_width;
    int m_height;
    int m_fps;
    CameraIntrinsics m_intrinsics;
    CameraExtrinsics m_extrinsics;
    CameraSettings m_settings;
    
    struct Buffer {
        void* start;
        size_t length;
    };
    
    std::vector<Buffer> m_buffers;
    std::function<void(const CameraFrame&)> m_callback;
    bool m_capturing;
    
    /**
     * @brief Initialize V4L2
     * @return true if successful
     */
    bool initV4L2();
    
    /**
     * @brief Initialize memory mapping
     * @return true if successful
     */
    bool initMmap();
    
    /**
     * @brief Initialize camera format
     * @return true if successful
     */
    bool initFormat();
    
    /**
     * @brief Set V4L2 control
     * @param id Control ID
     * @param value Control value
     * @return true if successful
     */
    bool setControl(uint32_t id, int value);
    
    /**
     * @brief Get V4L2 control
     * @param id Control ID
     * @param value Output value
     * @return true if successful
     */
    bool getControl(uint32_t id, int& value) const;
    
    /**
     * @brief Query control
     * @param id Control ID
     * @return true if control is supported
     */
    bool queryControl(uint32_t id) const;
    
    /**
     * @brief Process frame from buffer
     * @param buffer Buffer to process
     * @param frame Output frame
     * @return true if successful
     */
    bool processFrame(Buffer& buffer, CameraFrame& frame);
    
    /**
     * @brief Capture thread function
     */
    void captureThread();
};
