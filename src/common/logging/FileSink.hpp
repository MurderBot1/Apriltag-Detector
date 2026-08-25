#pragma once

#include "Logger.hpp"
#include <string>
#include <fstream>
#include <mutex>

/**
 * @brief File-based log sink
 * 
 * Writes log messages to a file with rotation support
 */
class FileSink : public ILogSink {
public:
    
    /**
     * @brief Constructor
     * @param filePath Path to the log file
     * @param maxSize Maximum file size in bytes before rotation (default: 10MB)
     * @param maxFiles Maximum number of rotated files to keep (default: 30)
     */
    FileSink(const std::string& filePath, size_t maxSize = 10 * 1024 * 1024, int maxFiles = 30);
    
    /**
     * @brief Destructor
     */
    ~FileSink() override;
    
    /**
     * @brief Write a log message to the file
     */
    void write(LogLevel level, const std::string& timestamp, 
               const std::string& processName, int processId, 
               const std::string& message) override;
    
    /**
     * @brief Flush the output buffer
     */
    void flush() override;
    
    /**
     * @brief Check if file needs rotation and rotate if necessary
     */
    void checkRotation();
    
private:
    std::string m_filePath;
    std::ofstream m_file;
    size_t m_maxSize;
    int m_maxFiles;
    size_t m_currentSize;
    std::mutex m_mutex;
    
    /**
     * @brief Rotate the log file
     */
    void rotate();
    
    /**
     * @brief Clean up old rotated files
     */
    void cleanupOldFiles();
};
