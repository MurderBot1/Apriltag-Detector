#pragma once

#include <vector>
#include <memory>
#include <string>

/**
 * @brief Camera frame structure
 */
struct CameraFrame {
    std::vector<uint8_t> data;      // Image data (RGB24 or grayscale)
    int width;                      // Frame width in pixels
    int height;                     // Frame height in pixels
    int channels;                  // Number of channels (3 for RGB, 1 for grayscale)
    uint64_t timestamp;            // Frame timestamp in microseconds
    bool isValid;                   // Whether the frame is valid
    
    CameraFrame() : width(0), height(0), channels(0), timestamp(0), isValid(false) {}
    
    size_t getDataSize() const {
        return width * height * channels;
    }
};

/**
 * @brief Camera intrinsics structure
 */
struct CameraIntrinsics {
    double fx;                     // Focal length x
    double fy;                     // Focal length y
    double cx;                     // Principal point x
    double cy;                     // Principal point y
    std::vector<double> distortion; // Distortion coefficients (k1, k2, p1, p2, k3)
    
    CameraIntrinsics() 
        : fx(0.0), fy(0.0), cx(0.0), cy(0.0), distortion(5, 0.0) {}
};

/**
 * @brief Camera extrinsics structure
 */
struct CameraExtrinsics {
    std::vector<double> translation; // [x, y, z] in meters
    std::vector<double> rotation;   // 3x3 rotation matrix or quaternion
    
    CameraExtrinsics() 
        : translation(3, 0.0), rotation(9, 0.0) {
        // Initialize rotation as identity matrix
        rotation = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    }
};

/**
 * @brief Camera settings structure
 */
struct CameraSettings {
    int exposure;       // Exposure (0-100 or platform-specific range)
    int brightness;     // Brightness (0-100)
    int contrast;       // Contrast (0-100)
    int saturation;     // Saturation (0-100)
    int gain;           // Gain (0-100)
    int fps;            // Frames per second
    
    CameraSettings() 
        : exposure(50), brightness(50), contrast(50), 
          saturation(50), gain(0), fps(30) {}
};

/**
 * @brief Abstract camera interface
 * 
 * All camera implementations must inherit from this class.
 */
class CameraInterface {
public:
    virtual ~CameraInterface() = default;
    
    /**
     * @brief Open the camera
     * @param deviceIndex Device index or -1 for default
     * @param configPath Path to camera configuration file
     * @return true if successful
     */
    virtual bool open(int deviceIndex = 0, const std::string& configPath = "") = 0;
    
    /**
     * @brief Close the camera
     */
    virtual void close() = 0;
    
    /**
     * @brief Check if camera is open
     * @return true if camera is open
     */
    virtual bool isOpen() const = 0;
    
    /**
     * @brief Capture a frame
     * @param frame Output frame
     * @return true if successful
     */
    virtual bool captureFrame(CameraFrame& frame) = 0;
    
    /**
     * @brief Start continuous frame capture
     * @param callback Callback function for each frame
     */
    virtual void startCapture(std::function<void(const CameraFrame&)> callback) = 0;
    
    /**
     * @brief Stop continuous frame capture
     */
    virtual void stopCapture() = 0;
    
    /**
     * @brief Set camera resolution
     * @param width Width in pixels
     * @param height Height in pixels
     * @return true if successful
     */
    virtual bool setResolution(int width, int height) = 0;
    
    /**
     * @brief Set camera FPS
     * @param fps Frames per second
     * @return true if successful
     */
    virtual bool setFPS(int fps) = 0;
    
    /**
     * @brief Set exposure
     * @param value Exposure value (platform-specific range)
     * @return true if successful
     */
    virtual bool setExposure(int value) = 0;
    
    /**
     * @brief Set brightness
     * @param value Brightness value (0-100)
     * @return true if successful
     */
    virtual bool setBrightness(int value) = 0;
    
    /**
     * @brief Set contrast
     * @param value Contrast value (0-100)
     * @return true if successful
     */
    virtual bool setContrast(int value) = 0;
    
    /**
     * @brief Set saturation
     * @param value Saturation value (0-100)
     * @return true if successful
     */
    virtual bool setSaturation(int value) = 0;
    
    /**
     * @brief Set gain
     * @param value Gain value (0-100)
     * @return true if successful
     */
    virtual bool setGain(int value) = 0;
    
    /**
     * @brief Get camera intrinsics
     * @param intrinsics Output intrinsics
     * @return true if successful
     */
    virtual bool getIntrinsics(CameraIntrinsics& intrinsics) const = 0;
    
    /**
     * @brief Set camera intrinsics
     * @param intrinsics New intrinsics
     * @return true if successful
     */
    virtual bool setIntrinsics(const CameraIntrinsics& intrinsics) = 0;
    
    /**
     * @brief Get camera extrinsics
     * @param extrinsics Output extrinsics
     * @return true if successful
     */
    virtual bool getExtrinsics(CameraExtrinsics& extrinsics) const = 0;
    
    /**
     * @brief Set camera extrinsics
     * @param extrinsics New extrinsics
     * @return true if successful
     */
    virtual bool setExtrinsics(const CameraExtrinsics& extrinsics) = 0;
    
    /**
     * @brief Get camera settings
     * @param settings Output settings
     * @return true if successful
     */
    virtual bool getSettings(CameraSettings& settings) const = 0;
    
    /**
     * @brief Set camera settings
     * @param settings New settings
     * @return true if successful
     */
    virtual bool setSettings(const CameraSettings& settings) = 0;
    
    /**
     * @brief Get camera ID
     * @return Camera ID
     */
    virtual int getCameraId() const = 0;
    
    /**
     * @brief Get device path
     * @return Device path
     */
    virtual std::string getDevicePath() const = 0;
    
    /**
     * @brief Get camera type
     * @return Camera type ("v4l2", "wmf", "avfoundation")
     */
    virtual std::string getCameraType() const = 0;
    
    /**
     * @brief Get current resolution
     * @param width Output width
     * @param height Output height
     */
    virtual void getResolution(int& width, int& height) const = 0;
    
    /**
     * @brief Get current FPS
     * @return Current FPS
     */
    virtual int getFPS() const = 0;
    
    /**
     * @brief Check if camera supports setting
     * @param setting Setting name
     * @return true if supported
     */
    virtual bool supportsSetting(const std::string& setting) const = 0;
};

/**
 * @brief Camera factory for creating camera instances
 */
class CameraFactory {
public:
    /**
     * @brief Create a camera instance based on type
     * @param type Camera type ("v4l2", "wmf", "avfoundation", or "auto")
     * @param cameraId Camera ID
     * @return Unique pointer to camera interface
     */
    static std::unique_ptr<CameraInterface> create(const std::string& type, int cameraId = 0);
    
    /**
     * @brief Create a camera instance with auto-detection
     * @param cameraId Camera ID
     * @return Unique pointer to camera interface
     */
    static std::unique_ptr<CameraInterface> createAuto(int cameraId = 0);
    
    /**
     * @brief Get available camera types
     * @return Vector of available camera types
     */
    static std::vector<std::string> getAvailableTypes();
    
    /**
     * @brief Get list of available cameras
     * @return Vector of camera information strings
     */
    static std::vector<std::string> listCameras();
};
