#pragma once

#include "../camera/CameraInterface.hpp"
#include <string>
#include <cstdint>
#include <memory>
#include <mutex>
#include <condition_variable>

/**
 * @brief Shared memory buffer for inter-process frame communication
 * 
 * Allows efficient transfer of camera frames between processes
 */
class SharedMemoryFrameBuffer {
public:
    
    /**
     * @brief Constructor
     * @param name Unique name for the shared memory segment
     * @param maxWidth Maximum frame width
     * @param maxHeight Maximum frame height
     * @param maxChannels Maximum number of channels
     */
    SharedMemoryFrameBuffer(const std::string& name, 
                           int maxWidth = 1920, 
                           int maxHeight = 1080, 
                           int maxChannels = 3);
    
    /**
     * @brief Destructor
     */
    ~SharedMemoryFrameBuffer();
    
    /**
     * @brief Write a frame to shared memory
     * @param frame Frame to write
     * @param timeoutMs Timeout in milliseconds (0 = no wait)
     * @return true if successful
     */
    bool writeFrame(const CameraFrame& frame, uint32_t timeoutMs = 100);
    
    /**
     * @brief Read a frame from shared memory
     * @param frame Output frame
     * @param timeoutMs Timeout in milliseconds (0 = no wait)
     * @return true if successful
     */
    bool readFrame(CameraFrame& frame, uint32_t timeoutMs = 100);
    
    /**
     * @brief Try to write a frame without waiting
     * @param frame Frame to write
     * @return true if successful
     */
    bool tryWriteFrame(const CameraFrame& frame);
    
    /**
     * @brief Try to read a frame without waiting
     * @param frame Output frame
     * @return true if successful
     */
    bool tryReadFrame(CameraFrame& frame);
    
    /**
     * @brief Check if buffer is ready for writing
     * @return true if ready
     */
    bool isWriteReady() const;
    
    /**
     * @brief Check if buffer is ready for reading
     * @return true if ready
     */
    bool isReadReady() const;
    
    /**
     * @brief Get the name of the shared memory segment
     * @return Name
     */
    const std::string& getName() const;
    
    /**
     * @brief Get maximum frame size
     * @return Maximum frame size in bytes
     */
    size_t getMaxFrameSize() const;
    
    /**
     * @brief Get current frame size
     * @return Current frame size in bytes
     */
    size_t getCurrentFrameSize() const;
    
    /**
     * @brief Clear the buffer
     */
    void clear();
    
private:
    struct SharedData {
        int width;
        int height;
        int channels;
        uint64_t timestamp;
        bool isValid;
        size_t dataSize;
        char data[1];  // Flexible array member - actual data follows
    };
    
    std::string m_name;
    int m_maxWidth;
    int m_maxHeight;
    int m_maxChannels;
    size_t m_maxFrameSize;
    
    #if PLATFORM_WINDOWS
        HANDLE m_hMapFile;
        LPVOID m_pBuffer;
    #else
        int m_fd;
        void* m_pBuffer;
        bool m_created;
    #endif
    
    std::mutex m_writeMutex;
    std::mutex m_readMutex;
    std::condition_variable m_writeCV;
    std::condition_variable m_readCV;
    
    bool m_hasData;
    
    /**
     * @brief Create shared memory segment
     * @return true if successful
     */
    bool createSharedMemory();
    
    /**
     * @brief Destroy shared memory segment
     */
    void destroySharedMemory();
    
    /**
     * @brief Get shared data pointer
     * @return Pointer to shared data
     */
    SharedData* getSharedData();
    
    /**
     * @brief Get frame data pointer
     * @return Pointer to frame data
     */
    char* getFrameData();
};

/**
 * @brief Shared memory frame buffer manager
 * 
 * Manages multiple frame buffers for different cameras
 */
class SharedMemoryFrameBufferManager {
public:
    
    /**
     * @brief Get or create a frame buffer for a camera
     * @param cameraId Camera ID
     * @param maxWidth Maximum frame width
     * @param maxHeight Maximum frame height
     * @param maxChannels Maximum number of channels
     * @return Shared pointer to frame buffer
     */
    static std::shared_ptr<SharedMemoryFrameBuffer> getBuffer(
        int cameraId, 
        int maxWidth = 1920, 
        int maxHeight = 1080, 
        int maxChannels = 3);
    
    /**
     * @brief Release a frame buffer
     * @param cameraId Camera ID
     */
    static void releaseBuffer(int cameraId);
    
    /**
     * @brief Release all frame buffers
     */
    static void releaseAllBuffers();
    
private:
    static std::map<int, std::shared_ptr<SharedMemoryFrameBuffer>> s_buffers;
    static std::mutex s_mutex;
};
