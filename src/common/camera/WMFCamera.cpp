#include "WMFCamera.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/TimeUtils.hpp"
#include "../logging/Logger.hpp"

// Link with MF libraries
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

WMFCamera::WMFCamera(int cameraId) 
    : m_cameraId(cameraId), m_width(1280), m_height(720), 
      m_fps(30), m_pAttributes(nullptr), m_pDevices(nullptr), 
      m_deviceCount(0), m_pReader(nullptr), m_pMediaType(nullptr),
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
    
    // Initialize MF
    initMF();
}

WMFCamera::~WMFCamera() {
    close();
}

bool WMFCamera::open(int deviceIndex, const std::string& configPath) {
    if (isOpen()) {
        close();
    }
    
    if (deviceIndex >= 0) {
        m_cameraId = deviceIndex;
    }
    
    // Select device
    if (!selectDevice(static_cast<UINT32>(m_cameraId))) {
        LOG_ERROR_F("Failed to select device %d", m_cameraId);
        return false;
    }
    
    // Initialize camera
    if (!initCamera()) {
        close();
        return false;
    }
    
    LOG_INFO_F("Opened WMF camera: %s (%dx%d @ %dfps)", 
               m_devicePath.c_str(), m_width, m_height, m_fps);
    
    return true;
}

void WMFCamera::close() {
    m_capturing = false;
    m_running = false;
    
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
    
    releaseMFObjects();
    
    LOG_INFO_F("Closed WMF camera: %s", m_devicePath.c_str());
}

bool WMFCamera::isOpen() const {
    return m_pReader != nullptr;
}

bool WMFCamera::captureFrame(CameraFrame& frame) {
    if (!isOpen()) {
        return false;
    }
    
    // Read a sample
    IMFSample* pSample = nullptr;
    DWORD flags = 0;
    
    HRESULT hr = m_pReader->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        nullptr,
        &flags,
        nullptr,
        &pSample
    );
    
    if (FAILED(hr)) {
        LOG_ERROR_F("ReadSample failed: 0x%08X", hr);
        return false;
    }
    
    if (pSample) {
        processSample(pSample);
        frame.timestamp = TimeUtils::getCurrentTimestampUs();
        frame.isValid = true;
        pSample->Release();
    }
    
    return frame.isValid;
}

void WMFCamera::startCapture(std::function<void(const CameraFrame&)> callback) {
    if (!isOpen()) {
        return;
    }
    
    if (m_capturing) {
        stopCapture();
    }
    
    m_callback = callback;
    m_capturing = true;
    m_running = true;
    
    // Start capture thread
    m_captureThread = std::thread(&WMFCamera::captureThread, this);
}

void WMFCamera::stopCapture() {
    m_capturing = false;
    m_running = false;
    m_callback = nullptr;
    
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
}

bool WMFCamera::setResolution(int width, int height) {
    if (!isOpen()) {
        return false;
    }
    
    m_width = width;
    m_height = height;
    
    // Would need to reinitialize media type
    // This is a simplified implementation
    
    return true;
}

bool WMFCamera::setFPS(int fps) {
    if (!isOpen()) {
        return false;
    }
    
    m_fps = fps;
    m_settings.fps = fps;
    
    // Would need to configure MF for FPS
    // Simplified implementation
    
    return true;
}

bool WMFCamera::setExposure(int value) {
    if (!isOpen()) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    m_settings.exposure = clamped;
    
    // Would need to set exposure via MF
    // Simplified implementation
    
    return true;
}

bool WMFCamera::setBrightness(int value) {
    if (!isOpen()) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    m_settings.brightness = clamped;
    return true;
}

bool WMFCamera::setContrast(int value) {
    if (!isOpen()) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    m_settings.contrast = clamped;
    return true;
}

bool WMFCamera::setSaturation(int value) {
    if (!isOpen()) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    m_settings.saturation = clamped;
    return true;
}

bool WMFCamera::setGain(int value) {
    if (!isOpen()) {
        return false;
    }
    
    int clamped = std::max(0, std::min(100, value));
    m_settings.gain = clamped;
    return true;
}

bool WMFCamera::getIntrinsics(CameraIntrinsics& intrinsics) const {
    intrinsics = m_intrinsics;
    return true;
}

bool WMFCamera::setIntrinsics(const CameraIntrinsics& intrinsics) {
    m_intrinsics = intrinsics;
    return true;
}

bool WMFCamera::getExtrinsics(CameraExtrinsics& extrinsics) const {
    extrinsics = m_extrinsics;
    return true;
}

bool WMFCamera::setExtrinsics(const CameraExtrinsics& extrinsics) {
    m_extrinsics = extrinsics;
    return true;
}

bool WMFCamera::getSettings(CameraSettings& settings) const {
    settings = m_settings;
    return true;
}

bool WMFCamera::setSettings(const CameraSettings& settings) {
    m_settings = settings;
    return true;
}

int WMFCamera::getCameraId() const {
    return m_cameraId;
}

std::string WMFCamera::getDevicePath() const {
    return m_devicePath;
}

std::string WMFCamera::getCameraType() const {
    return "wmf";
}

void WMFCamera::getResolution(int& width, int& height) const {
    width = m_width;
    height = m_height;
}

int WMFCamera::getFPS() const {
    return m_fps;
}

bool WMFCamera::supportsSetting(const std::string& setting) const {
    std::string lowerSetting = setting;
    for (char& c : lowerSetting) {
        c = static_cast<char>(tolower(c));
    }
    
    // WMF supports most settings through MF
    return true;
}

bool WMFCamera::initMF() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        LOG_ERROR_F("CoInitializeEx failed: 0x%08X", hr);
        return false;
    }
    
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        LOG_ERROR_F("MFStartup failed: 0x%08X", hr);
        CoUninitialize();
        return false;
    }
    
    return true;
}

bool WMFCamera::initCamera() {
    if (!m_pReader) {
        return false;
    }
    
    // Set up media type
    if (!setupMediaType()) {
        return false;
    }
    
    // Set current media type
    HRESULT hr = m_pReader->SetCurrentMediaType(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        nullptr,
        m_pMediaType
    );
    
    if (FAILED(hr)) {
        LOG_ERROR_F("SetCurrentMediaType failed: 0x%08X", hr);
        return false;
    }
    
    // Read first sample to start stream
    IMFSample* pSample = nullptr;
    DWORD flags = 0;
    
    hr = m_pReader->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        nullptr,
        &flags,
        nullptr,
        &pSample
    );
    
    if (pSample) {
        pSample->Release();
    }
    
    return true;
}

bool WMFCamera::selectDevice(UINT32 index) {
    // Enumerate video capture devices
    HRESULT hr = MFCreateAttributes(&m_pAttributes, 1);
    if (FAILED(hr)) {
        LOG_ERROR_F("MFCreateAttributes failed: 0x%08X", hr);
        return false;
    }
    
    hr = m_pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MFDevSourceType_VideoCapture
    );
    
    if (FAILED(hr)) {
        LOG_ERROR_F("SetGUID failed: 0x%08X", hr);
        m_pAttributes->Release();
        m_pAttributes = nullptr;
        return false;
    }
    
    // Get device count
    UINT32 count = 0;
    hr = MFEnumDeviceSources(m_pAttributes, &m_pDevices, &count);
    
    if (FAILED(hr)) {
        LOG_ERROR_F("MFEnumDeviceSources failed: 0x%08X", hr);
        m_pAttributes->Release();
        m_pAttributes = nullptr;
        return false;
    }
    
    m_deviceCount = count;
    
    if (index >= count) {
        LOG_ERROR_F("Device index %d out of range (0-%d)", index, count - 1);
        
        // Cleanup
        for (UINT32 i = 0; i < count; i++) {
            m_pDevices[i]->Release();
        }
        CoTaskMemFree(m_pDevices);
        m_pDevices = nullptr;
        m_deviceCount = 0;
        
        m_pAttributes->Release();
        m_pAttributes = nullptr;
        
        return false;
    }
    
    // Create source reader
    hr = MFCreateSourceReaderFromMediaSource(
        m_pDevices[index],
        m_pAttributes,
        &m_pReader
    );
    
    if (FAILED(hr)) {
        LOG_ERROR_F("MFCreateSourceReaderFromMediaSource failed: 0x%08X", hr);
        return false;
    }
    
    // Cleanup device list
    for (UINT32 i = 0; i < count; i++) {
        m_pDevices[i]->Release();
    }
    CoTaskMemFree(m_pDevices);
    m_pDevices = nullptr;
    m_deviceCount = 0;
    
    m_pAttributes->Release();
    m_pAttributes = nullptr;
    
    return true;
}

bool WMFCamera::setupMediaType() {
    if (!m_pReader) {
        return false;
    }
    
    // Get native media type
    IMFMediaType* pNativeType = nullptr;
    HRESULT hr = m_pReader->GetNativeMediaType(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        &pNativeType
    );
    
    if (FAILED(hr)) {
        LOG_ERROR_F("GetNativeMediaType failed: 0x%08X", hr);
        return false;
    }
    
    // Create RGB24 media type
    hr = MFCreateMediaType(&m_pMediaType);
    if (FAILED(hr)) {
        LOG_ERROR_F("MFCreateMediaType failed: 0x%08X", hr);
        pNativeType->Release();
        return false;
    }
    
    // Set major type
    hr = m_pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) {
        LOG_ERROR_F("SetGUID MFMediaType_Video failed: 0x%08X", hr);
        pNativeType->Release();
        m_pMediaType->Release();
        m_pMediaType = nullptr;
        return false;
    }
    
    // Set subtype
    hr = m_pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);
    if (FAILED(hr)) {
        LOG_ERROR_F("SetGUID MFVideoFormat_RGB24 failed: 0x%08X", hr);
        pNativeType->Release();
        m_pMediaType->Release();
        m_pMediaType = nullptr;
        return false;
    }
    
    // Set frame size
    MFSetAttributeSize(m_pMediaType, MF_MT_FRAME_SIZE, m_width, m_height);
    
    // Set frame rate
    MFSetAttributeRatio(m_pMediaType, MF_MT_FRAME_RATE, m_fps, 1);
    
    // Set interleaving mode
    hr = m_pMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (FAILED(hr)) {
        LOG_WARN_F("SetUINT32 MFVideoInterlace_Progressive failed: 0x%08X", hr);
    }
    
    pNativeType->Release();
    return true;
}

void WMFCamera::captureThread() {
    while (m_running && isOpen()) {
        IMFSample* pSample = nullptr;
        DWORD flags = 0;
        
        HRESULT hr = m_pReader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            nullptr,
            &flags,
            nullptr,
            &pSample
        );
        
        if (FAILED(hr)) {
            LOG_ERROR_F("ReadSample failed in capture thread: 0x%08X", hr);
            TimeUtils::sleepMs(100);
            continue;
        }
        
        if (pSample && m_callback) {
            processSample(pSample);
        }
        
        if (pSample) {
            pSample->Release();
        }
        
        if (!m_capturing) {
            break;
        }
    }
    
    LOG_INFO("WMF capture thread stopped");
}

void WMFCamera::processSample(IMFSample* pSample) {
    CameraFrame frame;
    frame.width = m_width;
    frame.height = m_height;
    frame.channels = 3;  // RGB24
    frame.timestamp = TimeUtils::getCurrentTimestampUs();
    frame.isValid = false;
    
    IMFMediaBuffer* pBuffer = nullptr;
    HRESULT hr = pSample->ConvertToContiguousBuffer(&pBuffer);
    
    if (FAILED(hr)) {
        LOG_ERROR_F("ConvertToContiguousBuffer failed: 0x%08X", hr);
        return;
    }
    
    BYTE* pData = nullptr;
    DWORD bufferLength = 0;
    
    hr = pBuffer->Lock(&pData, nullptr, &bufferLength);
    if (FAILED(hr)) {
        LOG_ERROR_F("Lock failed: 0x%08X", hr);
        pBuffer->Release();
        return;
    }
    
    // Process the frame
    int stride = m_width * 3;  // RGB24
    frame.data.resize(m_width * m_height * 3);
    convertToRGB24(pData, m_width, m_height, stride, frame);
    frame.isValid = true;
    
    pBuffer->Unlock();
    pBuffer->Release();
    
    if (m_callback) {
        m_callback(frame);
    }
}

void WMFCamera::convertToRGB24(BYTE* pData, int width, int height, int stride, CameraFrame& frame) {
    // If already RGB24, just copy
    // Otherwise, would need to convert from YUV or other format
    // This is a simplified implementation
    
    for (int y = 0; y < height; ++y) {
        BYTE* src = pData + y * stride;
        BYTE* dst = frame.data.data() + y * width * 3;
        memcpy(dst, src, width * 3);
    }
}

void WMFCamera::releaseMFObjects() {
    if (m_pMediaType) {
        m_pMediaType->Release();
        m_pMediaType = nullptr;
    }
    
    if (m_pReader) {
        m_pReader->Release();
        m_pReader = nullptr;
    }
    
    if (m_pDevices) {
        for (UINT32 i = 0; i < m_deviceCount; i++) {
            m_pDevices[i]->Release();
        }
        CoTaskMemFree(m_pDevices);
        m_pDevices = nullptr;
        m_deviceCount = 0;
    }
    
    if (m_pAttributes) {
        m_pAttributes->Release();
        m_pAttributes = nullptr;
    }
    
    MFShutdown();
    CoUninitialize();
}
