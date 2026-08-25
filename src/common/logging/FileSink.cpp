#include "FileSink.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/Platform.hpp"
#include <algorithm>

FileSink::FileSink(const std::string& filePath, size_t maxSize, int maxFiles)
    : m_filePath(filePath), m_maxSize(maxSize), m_maxFiles(maxFiles), m_currentSize(0) {
    
    // Ensure directory exists
    std::string dir = FileSystem::getParentDirectory(filePath);
    if (!dir.empty()) {
        FileSystem::createDirectories(dir);
    }
    
    // Open file in append mode
    m_file.open(filePath, std::ios::app);
    if (m_file.is_open()) {
        m_currentSize = FileSystem::getFileSize(filePath);
    }
}

FileSink::~FileSink() {
    flush();
    if (m_file.is_open()) {
        m_file.close();
    }
}

void FileSink::write(LogLevel level, const std::string& timestamp, 
                   const std::string& processName, int processId, 
                   const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    checkRotation();
    
    if (!m_file.is_open()) {
        m_file.open(m_filePath, std::ios::app);
        if (!m_file.is_open()) {
            return;
        }
        m_currentSize = FileSystem::getFileSize(m_filePath);
    }
    
    std::string line = message + "\n";
    m_file << line;
    m_currentSize += line.size();
    
    // Check if we need to rotate after writing
    if (m_currentSize >= m_maxSize) {
        rotate();
    }
}

void FileSink::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) {
        m_file.flush();
    }
}

void FileSink::checkRotation() {
    // Check if file exists and get its size
    if (FileSystem::exists(m_filePath)) {
        size_t actualSize = FileSystem::getFileSize(m_filePath);
        if (actualSize >= m_maxSize) {
            rotate();
        }
    }
}

void FileSink::rotate() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close current file
    if (m_file.is_open()) {
        m_file.close();
    }
    
    // Generate rotated filename with timestamp
    auto timestamp = FileSystem::getTimestampDirectoryName();
    std::string dir = FileSystem::getParentDirectory(m_filePath);
    std::string baseName = FileSystem::getFilename(m_filePath);
    std::string ext = FileSystem::getExtension(m_filePath);
    std::string nameWithoutExt = FileSystem::getStem(m_filePath);
    
    // Create rotated filename: basename_YYYY-MM-DD_HH-MM-SS.ext
    std::string rotatedName = nameWithoutExt + "_" + timestamp + ext;
    std::string rotatedPath = FileSystem::joinPaths({dir, rotatedName});
    
    // Rename current file
    if (FileSystem::exists(m_filePath)) {
        FileSystem::renameFile(m_filePath, rotatedPath);
    }
    
    // Open new file
    m_file.open(m_filePath, std::ios::app);
    m_currentSize = 0;
    
    // Clean up old files
    cleanupOldFiles();
}

void FileSink::cleanupOldFiles() {
    std::string dir = FileSystem::getParentDirectory(m_filePath);
    std::string baseName = FileSystem::getStem(m_filePath);
    std::string ext = FileSystem::getExtension(m_filePath);
    
    auto files = FileSystem::listFiles(dir);
    std::vector<std::string> rotatedFiles;
    
    for (const auto& file : files) {
        std::string filename = FileSystem::getFilename(file);
        if (filename.find(baseName) == 0 && filename != (baseName + ext)) {
            rotatedFiles.push_back(file);
        }
    }
    
    // Sort by filename (which contains timestamp)
    std::sort(rotatedFiles.begin(), rotatedFiles.end());
    
    // Remove oldest files if we have too many
    while (rotatedFiles.size() > static_cast<size_t>(m_maxFiles)) {
        FileSystem::removeFile(rotatedFiles.front());
        rotatedFiles.erase(rotatedFiles.begin());
    }
}
