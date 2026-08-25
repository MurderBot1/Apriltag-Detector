#include "SharedMemoryFrameBuffer.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/Platform.hpp"
#include "../logging/Logger.hpp"
#include <cstring>

// Static members for manager
std::map<int, std::shared_ptr<SharedMemoryFrameBuffer>> 
    SharedMemoryFrameBufferManager::s_buffers;
std::mutex SharedMemoryFrameBufferManager::s_mutex;

SharedMemoryFrameBuffer::SharedMemoryFrameBuffer(const std::string& name, 
                                               int maxWidth, int maxHeight, int maxChannels)
    : m_name(name), m_maxWidth(maxWidth), m_maxHeight(maxHeight),
      m_maxChannels(maxChannels), m_hasData(false) {
    
    m_maxFrameSize = static_cast<size_t>(maxWidth) * maxHeight * maxChannels;
    
    #if PLATFORM_WINDOWS
        m_hMapFile = INVALID_HANDLE_VALUE;
        m_pBuffer = nullptr;
    #else
        m_fd = -1;
        m_pBuffer = nullptr;
        m_created = false;
    #endif
    
    if (!createSharedMemory()) {
        LOG_ERROR_F("Failed to create shared memory for buffer '%s'", name.c_str());
    }
}

SharedMemoryFrameBuffer::~SharedMemoryFrameBuffer() {
    destroySharedMemory();
}

bool SharedMemoryFrameBuffer::createSharedMemory() {
    #if PLATFORM_WINDOWS
        // Create file mapping
        m_hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // Use paging file
            nullptr,                  // Default security
            PAGE_READWRITE,          // Read/write access
            0,                       // Maximum object size (high-order DWORD)
            static_cast<DWORD>(sizeof(SharedData) + m_maxFrameSize - 1), // Low-order DWORD
            m_name.c_str()            // Name of mapping object
        );
        
        if (m_hMapFile == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            LOG_ERROR_F("CreateFileMapping failed for '%s': error %d", 
                       m_name.c_str(), error);
            return false;
        }
        
        // Map view of file
        m_pBuffer = MapViewOfFile(
            m_hMapFile,               // Handle to map object
            FILE_MAP_ALL_ACCESS,     // Read/write permission
            0,
            0,
            sizeof(SharedData) + m_maxFrameSize
        );
        
        if (m_pBuffer == nullptr) {
            DWORD error = GetLastError();
            LOG_ERROR_F("MapViewOfFile failed for '%s': error %d", 
                       m_name.c_str(), error);
            CloseHandle(m_hMapFile);
            m_hMapFile = INVALID_HANDLE_VALUE;
            return false;
        }
        
        // Initialize shared data
        SharedData* data = getSharedData();
        data->width = 0;
        data->height = 0;
        data->channels = 0;
        data->timestamp = 0;
        data->isValid = false;
        data->dataSize = 0;
        
    #else
        // Create shared memory segment
        std::string shmPath = "/dev/shm/" + m_name;
        
        // Try to create with O_CREAT | O_EXCL first
        m_fd = shm_open(m_name.c_str(), O_CREAT | O_RDWR | O_EXCL, 0666);
        
        if (m_fd < 0) {
            // Already exists, try to open existing
            m_fd = shm_open(m_name.c_str(), O_RDWR, 0666);
            if (m_fd < 0) {
                LOG_ERROR_F("shm_open failed for '%s': %s", 
                           m_name.c_str(), strerror(errno));
                return false;
            }
        } else {
            m_created = true;
        }
        
        // Set size
        size_t totalSize = sizeof(SharedData) + m_maxFrameSize - 1;
        if (ftruncate(m_fd, totalSize) < 0) {
            LOG_ERROR_F("ftruncate failed for '%s': %s", 
                       m_name.c_str(), strerror(errno));
            close(m_fd);
            m_fd = -1;
            return false;
        }
        
        // Map memory
        m_pBuffer = mmap(
            nullptr,                   // Let system choose address
            totalSize,                 // Size
            PROT_READ | PROT_WRITE,    // Read/write
            MAP_SHARED,                // Shared mapping
            m_fd,                      // File descriptor
            0                         // Offset
        );
        
        if (m_pBuffer == MAP_FAILED) {
            LOG_ERROR_F("mmap failed for '%s': %s", 
                       m_name.c_str(), strerror(errno));
            close(m_fd);
            m_fd = -1;
            return false;
        }
        
        // Initialize shared data
        SharedData* data = getSharedData();
        data->width = 0;
        data->height = 0;
        data->channels = 0;
        data->timestamp = 0;
        data->isValid = false;
        data->dataSize = 0;
        
    #endif
    
    return true;
}

void SharedMemoryFrameBuffer::destroySharedMemory() {
    #if PLATFORM_WINDOWS
        if (m_pBuffer) {
            UnmapViewOfFile(m_pBuffer);
            m_pBuffer = nullptr;
        }
        
        if (m_hMapFile != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hMapFile);
            m_hMapFile = INVALID_HANDLE_VALUE;
        }
    #else
        if (m_pBuffer && m_pBuffer != MAP_FAILED) {
            munmap(m_pBuffer, sizeof(SharedData) + m_maxFrameSize - 1);
            m_pBuffer = nullptr;
        }
        
        if (m_fd >= 0) {
            close(m_fd);
            m_fd = -1;
        }
        
        // Only remove if we created it
        if (m_created) {
            shm_unlink(m_name.c_str());
            m_created = false;
        }
    #endif
}

SharedMemoryFrameBuffer::SharedData* SharedMemoryFrameBuffer::getSharedData() {
    return static_cast<SharedData*>(m_pBuffer);
}

char* SharedMemoryFrameBuffer::getFrameData() {
    SharedData* data = getSharedData();
    return data->data;
}

bool SharedMemoryFrameBuffer::writeFrame(const CameraFrame& frame, uint32_t timeoutMs) {
    if (!m_pBuffer) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(m_writeMutex);
    
    // Wait for read to finish
    if (!m_writeCV.wait_for(lock, std::chrono::milliseconds(timeoutMs),
        [this]() { return !m_hasData; })) {
        LOG_WARN("Write timeout - buffer still has data");
        return false;
    }
    
    // Check frame size
    size_t requiredSize = frame.width * frame.height * frame.channels;
    if (requiredSize > m_maxFrameSize) {
        LOG_ERROR_F("Frame too large: %zu > %zu", requiredSize, m_maxFrameSize);
        return false;
    }
    
    // Copy frame data
    SharedData* data = getSharedData();
    char* frameData = getFrameData();
    
    std::memcpy(frameData, frame.data.data(), requiredSize);
    
    // Update metadata
    data->width = frame.width;
    data->height = frame.height;
    data->channels = frame.channels;
    data->timestamp = frame.timestamp;
    data->isValid = frame.isValid;
    data->dataSize = requiredSize;
    
    // Signal that data is available
    {
        std::lock_guard<std::mutex> readLock(m_readMutex);
        m_hasData = true;
    }
    m_readCV.notify_one();
    
    return true;
}

bool SharedMemoryFrameBuffer::readFrame(CameraFrame& frame, uint32_t timeoutMs) {
    if (!m_pBuffer) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(m_readMutex);
    
    // Wait for data
    if (!m_readCV.wait_for(lock, std::chrono::milliseconds(timeoutMs),
        [this]() { return m_hasData; })) {
        LOG_WARN("Read timeout - no data available");
        return false;
    }
    
    // Read frame data
    SharedData* data = getSharedData();
    char* frameData = getFrameData();
    
    frame.width = data->width;
    frame.height = data->height;
    frame.channels = data->channels;
    frame.timestamp = data->timestamp;
    frame.isValid = data->isValid;
    frame.data.resize(data->dataSize);
    
    std::memcpy(frame.data.data(), frameData, data->dataSize);
    
    // Signal that we've read
    {
        std::lock_guard<std::mutex> writeLock(m_writeMutex);
        m_hasData = false;
    }
    m_writeCV.notify_one();
    
    return true;
}

bool SharedMemoryFrameBuffer::tryWriteFrame(const CameraFrame& frame) {
    if (!m_pBuffer) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(m_writeMutex, std::try_to_lock);
    if (!lock.owns_lock() || m_hasData) {
        return false;
    }
    
    // Check frame size
    size_t requiredSize = frame.width * frame.height * frame.channels;
    if (requiredSize > m_maxFrameSize) {
        return false;
    }
    
    // Copy frame data
    SharedData* data = getSharedData();
    char* frameData = getFrameData();
    
    std::memcpy(frameData, frame.data.data(), requiredSize);
    
    // Update metadata
    data->width = frame.width;
    data->height = frame.height;
    data->channels = frame.channels;
    data->timestamp = frame.timestamp;
    data->isValid = frame.isValid;
    data->dataSize = requiredSize;
    
    // Signal that data is available
    {
        std::lock_guard<std::mutex> readLock(m_readMutex);
        m_hasData = true;
    }
    m_readCV.notify_one();
    
    return true;
}

bool SharedMemoryFrameBuffer::tryReadFrame(CameraFrame& frame) {
    if (!m_pBuffer) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(m_readMutex, std::try_to_lock);
    if (!lock.owns_lock() || !m_hasData) {
        return false;
    }
    
    // Read frame data
    SharedData* data = getSharedData();
    char* frameData = getFrameData();
    
    frame.width = data->width;
    frame.height = data->height;
    frame.channels = data->channels;
    frame.timestamp = data->timestamp;
    frame.isValid = data->isValid;
    frame.data.resize(data->dataSize);
    
    std::memcpy(frame.data.data(), frameData, data->dataSize);
    
    // Signal that we've read
    {
        std::lock_guard<std::mutex> writeLock(m_writeMutex);
        m_hasData = false;
    }
    m_writeCV.notify_one();
    
    return true;
}

bool SharedMemoryFrameBuffer::isWriteReady() const {
    std::lock_guard<std::mutex> lock(m_writeMutex);
    return !m_hasData;
}

bool SharedMemoryFrameBuffer::isReadReady() const {
    std::lock_guard<std::mutex> lock(m_readMutex);
    return m_hasData;
}

const std::string& SharedMemoryFrameBuffer::getName() const {
    return m_name;
}

size_t SharedMemoryFrameBuffer::getMaxFrameSize() const {
    return m_maxFrameSize;
}

size_t SharedMemoryFrameBuffer::getCurrentFrameSize() const {
    SharedData* data = getSharedData();
    return data ? static_cast<size_t>(data->dataSize) : 0;
}

void SharedMemoryFrameBuffer::clear() {
    std::lock_guard<std::mutex> writeLock(m_writeMutex);
    std::lock_guard<std::mutex> readLock(m_readMutex);
    
    SharedData* data = getSharedData();
    if (data) {
        data->width = 0;
        data->height = 0;
        data->channels = 0;
        data->timestamp = 0;
        data->isValid = false;
        data->dataSize = 0;
    }
    
    m_hasData = false;
    m_writeCV.notify_all();
    m_readCV.notify_all();
}

// Manager implementation

std::shared_ptr<SharedMemoryFrameBuffer> SharedMemoryFrameBufferManager::getBuffer(
    int cameraId, int maxWidth, int maxHeight, int maxChannels) {
    
    std::lock_guard<std::mutex> lock(s_mutex);
    
    std::string name = "apriltag_frame_buffer_" + std::to_string(cameraId);
    
    auto it = s_buffers.find(cameraId);
    if (it != s_buffers.end()) {
        return it->second;
    }
    
    auto buffer = std::make_shared<SharedMemoryFrameBuffer>(name, maxWidth, maxHeight, maxChannels);
    s_buffers[cameraId] = buffer;
    
    return buffer;
}

void SharedMemoryFrameBufferManager::releaseBuffer(int cameraId) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_buffers.erase(cameraId);
}

void SharedMemoryFrameBufferManager::releaseAllBuffers() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_buffers.clear();
}
