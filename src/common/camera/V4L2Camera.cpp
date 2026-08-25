#include "V4L2Camera.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/TimeUtils.hpp"
#include "../logging/Logger.hpp"
#include <cstring>
#include <thread>
#include <atomic>

V4L2Camera::V4L2Camera(int cameraId) 
    : m_cameraId(cameraId), m_fd(-1), m_width(1280), m_height(720), 
      m_fps(30), m_capturing(false) {
    m_devicePath = "/dev/video" + std::to_string(cameraId);
    
    // Default intrinsics
    m_intrinsics.fx = 800.0;
    m_intrinsics.fy = 800.0;
    m_intrinsics.cx = 640.0;
    m_intrinsics.cy = 360.0;
    m_intrinsics.distortion = {0.0, 0.0, 0.0, 0.0, 0.0};
    
    // Default extrinsics (identity)
    m_extrinsics.translation = {0.0, 0.0, 0.0};
    m_extrinsics.rotation = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    
    // Default settings
    m_settings.exposure = 50;
    m_settings.brightness = 50;
    m_settings.contrast = 50;
    m_settings.saturation = 50;
    m_settings.gain = 0;
    m_settings.fps = 30;
}

V4L2Camera::~V4L2Camera() {
    close();
}

bool V4L2Camera::open(int deviceIndex, const std::string& configPath) {
    if (isOpen()) {
        close();
    }
    
    if (deviceIndex >= 0) {
        m_cameraId = deviceIndex;
        m_devicePath = "/dev/video" + std::to_string(deviceIndex);
    }
    
    // Try to open device
    m_fd = ::open(m_devicePath.c_str(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0) {
        LOG_ERROR_F("Failed to open V4L2 device: %s", m_devicePath.c_str());
        return false;
    }
    
    // Initialize V4L2
    if (!initV4L2()) {
        close();
        return false;
    }
    
    // Load configuration if provided
    if (!configPath.empty()) {
        // Would load from config file
    }
    
    LOG_INFO_F("Opened V4L2 camera: %s (%dx%d @ %dfps)", 
               m_devicePath.c_str(), m_width, m_height, m_fps);
    
    return true;
}

void V4L2Camera::close() {
    if (m_capturing) {
        stopCapture();
    }
    
    // Unmap buffers
    for (auto& buffer : m_buffers) {
        if (buffer.start) {
            munmap(buffer.start, buffer.length);
            buffer.start = nullptr;
        }
    }
    m_buffers.clear();
    
    // Close device
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    
    LOG_INFO_F("Closed V4L2 camera: %s", m_devicePath.c_str());
}

bool V4L2Camera::isOpen() const {
    return m_fd >= 0;
}

bool V4L2Camera::captureFrame(CameraFrame& frame) {
    if (!isOpen()) {
        return false;
    }
    
    // For single frame capture, we need to use read() or mmap
    // This is a simplified implementation
    
    v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(m_fd, VIDIOC_DQBUF, &buf) < 0) {
        LOG_ERROR("VIDIOC_DQBUF failed");
        return false;
    }
    
    // Process the frame
    if (buf.index >= 0 && buf.index < static_cast<int>(m_buffers.size())) {
        processFrame(m_buffers[buf.index], frame);
        frame.timestamp = TimeUtils::getCurrentTimestampUs();
        frame.isValid = true;
    }
    
    // Requeue buffer
    if (ioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
        LOG_ERROR("VIDIOC_QBUF failed");
    }
    
    return frame.isValid;
}

void V4L2Camera::startCapture(std::function<void(const CameraFrame&)> callback) {
    if (!isOpen()) {
        return;
    }
    
    if (m_capturing) {
        stopCapture();
    }
    
    m_callback = callback;
    m_capturing = true;
    
    // Start capture
    if (!initMmap()) {
        m_capturing = false;
        return;
    }
    
    // Start capture thread
    std::thread(&V4L2Camera::captureThread, this).detach();
}

void V4L2Camera::stopCapture() {
    m_capturing = false;
    m_callback = nullptr;
}

bool V4L2Camera::setResolution(int width, int height) {
    if (!isOpen()) {
        return false;
    }
    
    m_width = width;
    m_height = height;
    
    // Reinitialize format
    return initFormat();
}

bool V4L2Camera::setFPS(int fps) {
    if (!isOpen()) {
        return false;
    }
    
    m_fps = fps;
    m_settings.fps = fps;
    
    // Set frame rate
    v4l2_streamparm streamParam;
    memset(&streamParam, 0, sizeof(streamParam));
    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (ioctl(m_fd, VIDIOC_G_PARM, &streamParam) < 0) {
        LOG_WARN("Failed to get stream parameters");
        return false;
    }
    
    streamParam.parm.capture.timeperframe.numerator = 1;
    streamParam.parm.capture.timeperframe.denominator = fps;
    
    if (ioctl(m_fd, VIDIOC_S_PARM, &streamParam) < 0) {
        LOG_WARN("Failed to set frame rate");
        return false;
    }
    
    return true;
}

bool V4L2Camera::setExposure(int value) {
    if (!isOpen()) {
        return false;
    }
    
    // Clamp value to 0-100
    int clamped = std::max(0, std::min(100, value));
    
    if (setControl(V4L2_CID_EXPOSURE, clamped)) {
        m_settings.exposure = clamped;
        return true;
    }
    
    return false;
}

bool V4L2Camera::setBrightness(int value) {
    if (!isOpen()) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    
    if (setControl(V4L2_CID_BRIGHTNESS, clamped)) {
        m_settings.brightness = clamped;
        return true;
    }
    
    return false;
}

bool V4L2Camera::setContrast(int value) {
    if (!isOpen()) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    
    if (setControl(V4L2_CID_CONTRAST, clamped)) {
        m_settings.contrast = clamped;
        return true;
    }
    
    return false;
}

bool V4L2Camera::setSaturation(int value) {
    if (!isOpen()) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    
    if (setControl(V4L2_CID_SATURATION, clamped)) {
        m_settings.saturation = clamped;
        return true;
    }
    
    return false;
}

bool V4L2Camera::setGain(int value) {
    if (!isOpen()) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    
    if (setControl(V4L2_CID_GAIN, clamped)) {
        m_settings.gain = clamped;
        return true;
    }
    
    return false;
}

bool V4L2Camera::getIntrinsics(CameraIntrinsics& intrinsics) const {
    intrinsics = m_intrinsics;
    return true;
}

bool V4L2Camera::setIntrinsics(const CameraIntrinsics& intrinsics) {
    m_intrinsics = intrinsics;
    return true;
}

bool V4L2Camera::getExtrinsics(CameraExtrinsics& extrinsics) const {
    extrinsics = m_extrinsics;
    return true;
}

bool V4L2Camera::setExtrinsics(const CameraExtrinsics& extrinsics) {
    m_extrinsics = extrinsics;
    return true;
}

bool V4L2Camera::getSettings(CameraSettings& settings) const {
    settings = m_settings;
    return true;
}

bool V4L2Camera::setSettings(const CameraSettings& settings) {
    m_settings = settings;
    
    // Apply settings
    setExposure(settings.exposure);
    setBrightness(settings.brightness);
    setContrast(settings.contrast);
    setSaturation(settings.saturation);
    setGain(settings.gain);
    setFPS(settings.fps);
    
    return true;
}

int V4L2Camera::getCameraId() const {
    return m_cameraId;
}

std::string V4L2Camera::getDevicePath() const {
    return m_devicePath;
}

std::string V4L2Camera::getCameraType() const {
    return "v4l2";
}

void V4L2Camera::getResolution(int& width, int& height) const {
    width = m_width;
    height = m_height;
}

int V4L2Camera::getFPS() const {
    return m_fps;
}

bool V4L2Camera::supportsSetting(const std::string& setting) const {
    std::string lowerSetting = setting;
    for (char& c : lowerSetting) {
        c = static_cast<char>(tolower(c));
    }
    
    if (lowerSetting == "exposure") return queryControl(V4L2_CID_EXPOSURE);
    if (lowerSetting == "brightness") return queryControl(V4L2_CID_BRIGHTNESS);
    if (lowerSetting == "contrast") return queryControl(V4L2_CID_CONTRAST);
    if (lowerSetting == "saturation") return queryControl(V4L2_CID_SATURATION);
    if (lowerSetting == "gain") return queryControl(V4L2_CID_GAIN);
    if (lowerSetting == "fps" || lowerSetting == "framerate") return true;
    if (lowerSetting == "resolution") return true;
    
    return false;
}

bool V4L2Camera::initV4L2() {
    // Check capabilities
    v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        LOG_ERROR_F("VIDIOC_QUERYCAP failed for %s", m_devicePath.c_str());
        return false;
    }
    
    // Check if video capture is supported
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOG_ERROR_F("%s does not support video capture", m_devicePath.c_str());
        return false;
    }
    
    // Initialize format
    if (!initFormat()) {
        return false;
    }
    
    // Initialize memory mapping
    if (!initMmap()) {
        return false;
    }
    
    // Start streaming
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
        LOG_ERROR("VIDIOC_STREAMON failed");
        return false;
    }
    
    return true;
}

bool V4L2Camera::initMmap() {
    // Unmap existing buffers
    for (auto& buffer : m_buffers) {
        if (buffer.start) {
            munmap(buffer.start, buffer.length);
            buffer.start = nullptr;
        }
    }
    m_buffers.clear();
    
    // Request buffers
    v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;  // Number of buffers
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) < 0) {
        LOG_ERROR("VIDIOC_REQBUFS failed");
        return false;
    }
    
    // Allocate buffers
    for (unsigned int i = 0; i < req.count; ++i) {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            LOG_ERROR_F("VIDIOC_QUERYBUF failed for buffer %d", i);
            return false;
        }
        
        Buffer buffer;
        buffer.length = buf.length;
        buffer.start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, 
                         MAP_SHARED, m_fd, buf.m.offset);
        
        if (buffer.start == MAP_FAILED) {
            LOG_ERROR_F("mmap failed for buffer %d", i);
            return false;
        }
        
        m_buffers.push_back(buffer);
    }
    
    // Queue buffers
    for (unsigned int i = 0; i < m_buffers.size(); ++i) {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
            LOG_ERROR_F("VIDIOC_QBUF failed for buffer %d", i);
            return false;
        }
    }
    
    return true;
}

bool V4L2Camera::initFormat() {
    // Set format
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = m_width;
    fmt.fmt.pix.height = m_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    
    if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        LOG_ERROR("VIDIOC_S_FMT failed");
        return false;
    }
    
    // Get actual format
    if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) < 0) {
        LOG_ERROR("VIDIOC_G_FMT failed");
        return false;
    }
    
    m_width = fmt.fmt.pix.width;
    m_height = fmt.fmt.pix.height;
    
    return true;
}

bool V4L2Camera::setControl(uint32_t id, int value) {
    v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = id;
    ctrl.value = value;
    
    if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl) < 0) {
        return false;
    }
    
    return true;
}

bool V4L2Camera::getControl(uint32_t id, int& value) const {
    v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = id;
    
    if (ioctl(m_fd, VIDIOC_G_CTRL, &ctrl) < 0) {
        return false;
    }
    
    value = ctrl.value;
    return true;
}

bool V4L2Camera::queryControl(uint32_t id) const {
    v4l2_queryctrl query;
    memset(&query, 0, sizeof(query));
    query.id = id;
    
    if (ioctl(m_fd, VIDIOC_QUERYCTRL, &query) < 0) {
        return false;
    }
    
    return (query.flags & V4L2_CTRL_FLAG_DISABLED) == 0;
}

bool V4L2Camera::processFrame(Buffer& buffer, CameraFrame& frame) {
    if (!buffer.start || buffer.length == 0) {
        return false;
    }
    
    frame.width = m_width;
    frame.height = m_height;
    frame.channels = 3;  // RGB24
    frame.timestamp = TimeUtils::getCurrentTimestampUs();
    frame.isValid = true;
    
    // Copy frame data
    frame.data.resize(buffer.length);
    memcpy(frame.data.data(), buffer.start, buffer.length);
    
    return true;
}

void V4L2Camera::captureThread() {
    while (m_capturing && isOpen()) {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(m_fd, &fds);
        
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int r = select(m_fd + 1, &fds, nullptr, nullptr, &tv);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("select failed");
            break;
        }
        
        if (r == 0) {
            // Timeout
            continue;
        }
        
        if (ioctl(m_fd, VIDIOC_DQBUF, &buf) < 0) {
            LOG_ERROR("VIDIOC_DQBUF failed in capture thread");
            break;
        }
        
        // Process frame
        if (buf.index >= 0 && buf.index < static_cast<int>(m_buffers.size()) && m_callback) {
            CameraFrame frame;
            if (processFrame(m_buffers[buf.index], frame)) {
                m_callback(frame);
            }
        }
        
        // Requeue buffer
        if (ioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
            LOG_ERROR("VIDIOC_QBUF failed in capture thread");
            break;
        }
    }
    
    LOG_INFO("Capture thread stopped");
}
