#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace FileSystem {
    
    // Create directory recursively
    bool createDirectory(const std::string& path);
    
    // Create directories recursively
    bool createDirectories(const std::string& path);
    
    // Check if path exists
    bool exists(const std::string& path);
    
    // Check if path is directory
    bool isDirectory(const std::string& path);
    
    // Check if path is file
    bool isFile(const std::string& path);
    
    // Get parent directory
    std::string getParentDirectory(const std::string& path);
    
    // Get current working directory
    std::string getCurrentWorkingDirectory();
    
    // Get absolute path
    std::string getAbsolutePath(const std::string& path);
    
    // Join paths
    std::string joinPaths(const std::string& path1, const std::string& path2);
    std::string joinPaths(const std::vector<std::string>& paths);
    
    // Read file content
    std::string readFile(const std::string& path);
    
    // Write file content
    bool writeFile(const std::string& path, const std::string& content);
    
    // Append to file
    bool appendFile(const std::string& path, const std::string& content);
    
    // Get file size
    uint64_t getFileSize(const std::string& path);
    
    // Get last write time
    uint64_t getLastWriteTime(const std::string& path);
    
    // List files in directory
    std::vector<std::string> listFiles(const std::string& directory);
    
    // List directories in directory
    std::vector<std::string> listDirectories(const std::string& directory);
    
    // Get filename from path
    std::string getFilename(const std::string& path);
    
    // Get filename without extension
    std::string getStem(const std::string& path);
    
    // Get file extension
    std::string getExtension(const std::string& path);
    
    // Remove file
    bool removeFile(const std::string& path);
    
    // Remove directory recursively
    bool removeDirectory(const std::string& path);
    
    // Rename file
    bool renameFile(const std::string& oldPath, const std::string& newPath);
    
    // Copy file
    bool copyFile(const std::string& source, const std::string& destination);
    
    // Get timestamp for log directory naming
    std::string getTimestampDirectoryName();
    
} // namespace FileSystem
