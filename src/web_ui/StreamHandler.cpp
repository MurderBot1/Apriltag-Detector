#include "StreamHandler.hpp"
#include "../common/logging/Logger.hpp"
#include "../common/utils/TimeUtils.hpp"
#include <cstring>
#include <algorithm>

// JPEG encoding (simplified - in production use libjpeg-turbo)
#include <vector>

StreamHandler::StreamHandler() : m_webServer(nullptr) {
}

StreamHandler::~StreamHandler() {
    stopAllStreams();
}

void StreamHandler::startStream(int cameraId, int width, int height, 
                               int quality, int fps) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    StreamConfig& config = m_streams[cameraId];
    config.width = width;
    config.height = height;
    config.quality = std::clamp(quality, 1, 100);
    config.fps = std::max(fps, 1);
    config.streaming = true;
    
    LOG_INFO_F("Started MJPEG stream for camera %d (%dx%d, %d%% quality, %dfps)",
               cameraId, width, height, quality, fps);
}

void StreamHandler::stopStream(int cameraId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_streams.find(cameraId);
    if (it != m_streams.end()) {
        it->second.streaming = false;
        LOG_INFO_F("Stopped MJPEG stream for camera %d", cameraId);
    }
}

void StreamHandler::stopAllStreams() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& pair : m_streams) {
        pair.second.streaming = false;
    }
    
    LOG_INFO("Stopped all MJPEG streams");
}

std::string StreamHandler::getStreamUrl(int cameraId) const {
    return "/stream/camera" + std::to_string(cameraId) + ".mjpeg";
}

bool StreamHandler::isStreaming(int cameraId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_streams.find(cameraId);
    if (it == m_streams.end()) {
        return false;
    }
    
    return it->second.streaming;
}

CameraFrame StreamHandler::getFrame(int cameraId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_frames.find(cameraId);
    if (it != m_frames.end()) {
        return it->second;
    }
    
    return CameraFrame();
}

void StreamHandler::setFrame(int cameraId, const CameraFrame& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frames[cameraId] = frame;
}

void StreamHandler::registerWithWebServer(WebServer* server) {
    m_webServer = server;
    
    if (server) {
        // Register MJPEG stream handler
        server->registerHandler("/stream/camera*.mjpeg", 
            [this](const HttpRequest& request) {
                // Extract camera ID from path
                std::string path = request.path;
                size_t start = path.find("camera") + 7;
                size_t end = path.find(".mjpeg");
                
                if (start != std::string::npos && end != std::string::npos) {
                    std::string idStr = path.substr(start, end - start);
                    try {
                        int cameraId = std::stoi(idStr);
                        MjpegStreamHandler handler(this, cameraId);
                        return handler.handleRequest(request);
                    } catch (...) {
                        // Invalid camera ID
                    }
                }
                
                HttpResponse response;
                response.statusCode = 404;
                response.statusText = "Not Found";
                return response;
            });
    }
}

void StreamHandler::setStreamParameters(int cameraId, int width, int height,
                                        int quality, int fps) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    StreamConfig& config = m_streams[cameraId];
    config.width = width;
    config.height = height;
    config.quality = std::clamp(quality, 1, 100);
    config.fps = std::max(fps, 1);
}

std::vector<uint8_t> StreamHandler::generateJpegFrame(const CameraFrame& frame, int quality) {
    // Simplified JPEG encoding
    // In a production implementation, use libjpeg-turbo or similar
    
    if (frame.data.empty() || frame.width <= 0 || frame.height <= 0) {
        return std::vector<uint8_t>();
    }
    
    // For now, return a placeholder JPEG
    // A real implementation would properly encode the frame
    
    // Simple grayscale JPEG (placeholder)
    std::vector<uint8_t> jpeg;
    
    // This is a minimal valid JPEG header (simplified)
    // In production, use a proper JPEG encoder
    
    return rgbToJpeg(frame.data, frame.width, frame.height, quality);
}

CameraFrame StreamHandler::resizeFrame(const CameraFrame& frame, int width, int height) {
    CameraFrame result;
    result.width = width;
    result.height = height;
    result.channels = frame.channels;
    result.timestamp = frame.timestamp;
    result.isValid = frame.isValid;
    
    if (frame.data.empty() || width <= 0 || height <= 0) {
        return result;
    }
    
    // Simple nearest-neighbor resize
    result.data.resize(width * height * frame.channels);
    
    double xRatio = static_cast<double>(frame.width) / width;
    double yRatio = static_cast<double>(frame.height) / height;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int srcX = static_cast<int>(x * xRatio);
            int srcY = static_cast<int>(y * yRatio);
            
            for (int c = 0; c < frame.channels; ++c) {
                size_t dstIdx = (y * width + x) * frame.channels + c;
                size_t srcIdx = (srcY * frame.width + srcX) * frame.channels + c;
                
                if (srcIdx < frame.data.size()) {
                    result.data[dstIdx] = frame.data[srcIdx];
                }
            }
        }
    }
    
    return result;
}

std::vector<uint8_t> StreamHandler::rgbToJpeg(const std::vector<uint8_t>& rgb,
                                            int width, int height, int quality) {
    // Placeholder - in production use libjpeg-turbo
    // For now, return a simple JPEG header (not a valid JPEG, but for testing)
    
    std::vector<uint8_t> jpeg;
    
    // Minimal JPEG header (simplified)
    // This is just for demonstration - a real implementation would use a library
    
    jpeg.push_back(0xFF);
    jpeg.push_back(0xD8);  // SOI
    jpeg.push_back(0xFF);
    jpeg.push_back(0xE0);  // APP0
    jpeg.push_back(0x00);
    jpeg.push_back(0x10);  // Length
    jpeg.push_back('J');
    jpeg.push_back('F');
    jpeg.push_back('I');
    jpeg.push_back('F');
    jpeg.push_back(0x00);
    jpeg.push_back(0x01);
    jpeg.push_back(0x01);
    jpeg.push_back(0x00);
    jpeg.push_back(0x00);
    jpeg.push_back(0x01);
    jpeg.push_back(0x00);
    jpeg.push_back(0x01);
    
    // Add placeholder image data
    for (size_t i = 0; i < rgb.size(); ++i) {
        jpeg.push_back(rgb[i]);
    }
    
    jpeg.push_back(0xFF);
    jpeg.push_back(0xD9);  // EOI
    
    return jpeg;
}

// MjpegStreamHandler implementation

MjpegStreamHandler::MjpegStreamHandler(StreamHandler* streamHandler, int cameraId)
    : m_streamHandler(streamHandler), m_cameraId(cameraId) {
}

HttpResponse MjpegStreamHandler::handleRequest(const HttpRequest& request) {
    HttpResponse response;
    
    if (!m_streamHandler || !m_streamHandler->isStreaming(m_cameraId)) {
        response.statusCode = 404;
        response.statusText = "Not Found";
        response.body = std::vector<uint8_t>("Stream not available".begin(), 
                                              "Stream not available".end());
        return response;
    }
    
    // Get the frame
    CameraFrame frame = m_streamHandler->getFrame(m_cameraId);
    
    if (!frame.isValid || frame.data.empty()) {
        response.statusCode = 503;
        response.statusText = "Service Unavailable";
        response.body = std::vector<uint8_t>("No frame available".begin(), 
                                              "No frame available".end());
        return response;
    }
    
    // Get stream parameters
    StreamHandler::StreamConfig config;
    {
        std::lock_guard<std::mutex> lock(m_streamHandler->m_mutex);
        auto it = m_streamHandler->m_streams.find(m_cameraId);
        if (it != m_streamHandler->m_streams.end()) {
            config = it->second;
        }
    }
    
    // Resize if needed
    if (frame.width != config.width || frame.height != config.height) {
        frame = m_streamHandler->resizeFrame(frame, config.width, config.height);
    }
    
    // Generate JPEG
    std::vector<uint8_t> jpeg = m_streamHandler->generateJpegFrame(frame, config.quality);
    
    // Create multipart response
    std::string boundary = "boundary_" + TimeUtils::getCurrentTimestampStr();
    
    std::ostringstream stream;
    stream << "--" << boundary << "\r\n";
    stream << "Content-Type: image/jpeg\r\n";
    stream << "Content-Length: " << jpeg.size() << "\r\n";
    stream << "\r\n";
    
    std::string header = stream.str();
    
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "multipart/x-mixed-replace; boundary=" + boundary;
    response.headers["Cache-Control"] = "no-cache";
    response.headers["Connection"] = "keep-alive";
    
    // Add header
    response.body.insert(response.body.end(), header.begin(), header.end());
    
    // Add JPEG data
    response.body.insert(response.body.end(), jpeg.begin(), jpeg.end());
    
    // Add boundary
    std::string footer = "\r\n--" + boundary + "--\r\n";
    response.body.insert(response.body.end(), footer.begin(), footer.end());
    
    return response;
}
