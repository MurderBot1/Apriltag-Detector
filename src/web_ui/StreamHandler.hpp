#pragma once

#include "WebServer.hpp"
#include "../common/camera/CameraInterface.hpp"
#include "../common/ipc/SharedMemoryFrameBuffer.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>

/**
 * @brief MJPEG stream handler
 * 
 * Handles MJPEG streaming for camera feeds
 */
class StreamHandler {
public:
    
    /**
     * @brief Constructor
     */
    StreamHandler();
    
    /**
     * @brief Destructor
     */
    ~StreamHandler();
    
    /**
     * @brief Start streaming
     * @param cameraId Camera ID
     * @param width Stream width
     * @param height Stream height
     * @param quality JPEG quality (1-100)
     * @param fps Target frames per second
     */
    void startStream(int cameraId, int width = 640, int height = 480, 
                    int quality = 80, int fps = 30);
    
    /**
     * @brief Stop streaming for a camera
     * @param cameraId Camera ID
     */
    void stopStream(int cameraId);
    
    /**
     * @brief Stop all streams
     */
    void stopAllStreams();
    
    /**
     * @brief Get MJPEG stream URL for a camera
     * @param cameraId Camera ID
     * @return Stream URL
     */
    std::string getStreamUrl(int cameraId) const;
    
    /**
     * @brief Check if streaming for a camera
     * @param cameraId Camera ID
     * @return true if streaming
     */
    bool isStreaming(int cameraId) const;
    
    /**
     * @brief Get frame from camera
     * @param cameraId Camera ID
     * @return CameraFrame
     */
    CameraFrame getFrame(int cameraId);
    
    /**
     * @brief Set frame for a camera (from camera detector)
     * @param cameraId Camera ID
     * @param frame Frame
     */
    void setFrame(int cameraId, const CameraFrame& frame);
    
    /**
     * @brief Register stream with web server
     * @param server WebServer instance
     */
    void registerWithWebServer(WebServer* server);
    
    /**
     * @brief Set stream parameters
     * @param cameraId Camera ID
     * @param width Stream width
     * @param height Stream height
     * @param quality JPEG quality
     * @param fps Target FPS
     */
    void setStreamParameters(int cameraId, int width, int height, 
                             int quality, int fps);
    
private:
    struct StreamConfig {
        int width;
        int height;
        int quality;
        int fps;
        std::atomic<bool> streaming;
        
        StreamConfig() : width(640), height(480), quality(80), fps(30), streaming(false) {}
    };
    
    std::map<int, StreamConfig> m_streams;
    std::map<int, CameraFrame> m_frames;
    mutable std::mutex m_mutex;
    WebServer* m_webServer;
    
    /**
     * @brief Generate MJPEG frame
     * @param frame Camera frame
     * @param quality JPEG quality
     * @return JPEG encoded frame
     */
    std::vector<uint8_t> generateJpegFrame(const CameraFrame& frame, int quality);
    
    /**
     * @brief Resize frame
     * @param frame Input frame
     * @param width Target width
     * @param height Target height
     * @return Resized frame
     */
    CameraFrame resizeFrame(const CameraFrame& frame, int width, int height);
    
    /**
     * @brief Convert RGB to JPEG
     * @param frame RGB frame
     * @param quality JPEG quality
     * @return JPEG encoded data
     */
    std::vector<uint8_t> rgbToJpeg(const std::vector<uint8_t>& rgb, 
                                   int width, int height, int quality);
};

/**
 * @brief MJPEG stream request handler
 */
class MjpegStreamHandler {
public:
    
    /**
     * @brief Constructor
     * @param streamHandler StreamHandler instance
     * @param cameraId Camera ID
     */
    MjpegStreamHandler(StreamHandler* streamHandler, int cameraId);
    
    /**
     * @brief Handle MJPEG stream request
     * @param request HTTP request
     * @return HTTP response
     */
    HttpResponse handleRequest(const HttpRequest& request);
    
private:
    StreamHandler* m_streamHandler;
    int m_cameraId;
};
