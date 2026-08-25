#include "FileSystem.hpp"
#include "Platform.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace FileSystem {
    
    bool createDirectory(const std::string& path) {
        try {
            return fs::create_directory(path);
        } catch (...) {
            return false;
        }
    }
    
    bool createDirectories(const std::string& path) {
        try {
            return fs::create_directories(path);
        } catch (...) {
            return false;
        }
    }
    
    bool exists(const std::string& path) {
        try {
            return fs::exists(path);
        } catch (...) {
            return false;
        }
    }
    
    bool isDirectory(const std::string& path) {
        try {
            return fs::is_directory(path);
        } catch (...) {
            return false;
        }
    }
    
    bool isFile(const std::string& path) {
        try {
            return fs::is_regular_file(path);
        } catch (...) {
            return false;
        }
    }
    
    std::string getParentDirectory(const std::string& path) {
        try {
            return fs::path(path).parent_path().string();
        } catch (...) {
            return "";
        }
    }
    
    std::string getCurrentWorkingDirectory() {
        try {
            return fs::current_path().string();
        } catch (...) {
            return "";
        }
    }
    
    std::string getAbsolutePath(const std::string& path) {
        try {
            return fs::absolute(path).string();
        } catch (...) {
            return path;
        }
    }
    
    std::string joinPaths(const std::string& path1, const std::string& path2) {
        try {
            return (fs::path(path1) / fs::path(path2)).string();
        } catch (...) {
            return path1 + Platform::getPathSeparator() + path2;
        }
    }
    
    std::string joinPaths(const std::vector<std::string>& paths) {
        if (paths.empty()) return "";
        fs::path result = fs::path(paths[0]);
        for (size_t i = 1; i < paths.size(); ++i) {
            result /= fs::path(paths[i]);
        }
        return result.string();
    }
    
    std::string readFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return "";
        }
        
        std::ostringstream oss;
        oss << file.rdbuf();
        return oss.str();
    }
    
    bool writeFile(const std::string& path, const std::string& content) {
        try {
            // Create parent directory if it doesn't exist
            fs::path p(path);
            if (p.has_parent_path()) {
                createDirectories(p.parent_path().string());
            }
            
            std::ofstream file(path, std::ios::binary);
            if (!file) {
                return false;
            }
            file << content;
            return file.good();
        } catch (...) {
            return false;
        }
    }
    
    bool appendFile(const std::string& path, const std::string& content) {
        try {
            std::ofstream file(path, std::ios::binary | std::ios::app);
            if (!file) {
                return false;
            }
            file << content;
            return file.good();
        } catch (...) {
            return false;
        }
    }
    
    uint64_t getFileSize(const std::string& path) {
        try {
            return static_cast<uint64_t>(fs::file_size(path));
        } catch (...) {
            return 0;
        }
    }
    
    uint64_t getLastWriteTime(const std::string& path) {
        try {
            auto ftime = fs::last_write_time(path);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            auto epoch = sctp.time_since_epoch();
            return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count());
        } catch (...) {
            return 0;
        }
    }
    
    std::vector<std::string> listFiles(const std::string& directory) {
        std::vector<std::string> files;
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path().string());
                }
            }
        } catch (...) {
            // Ignore errors
        }
        return files;
    }
    
    std::vector<std::string> listDirectories(const std::string& directory) {
        std::vector<std::string> dirs;
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_directory()) {
                    dirs.push_back(entry.path().string());
                }
            }
        } catch (...) {
            // Ignore errors
        }
        return dirs;
    }
    
    std::string getFilename(const std::string& path) {
        try {
            return fs::path(path).filename().string();
        } catch (...) {
            return "";
        }
    }
    
    std::string getStem(const std::string& path) {
        try {
            return fs::path(path).stem().string();
        } catch (...) {
            return "";
        }
    }
    
    std::string getExtension(const std::string& path) {
        try {
            return fs::path(path).extension().string();
        } catch (...) {
            return "";
        }
    }
    
    bool removeFile(const std::string& path) {
        try {
            return fs::remove(path);
        } catch (...) {
            return false;
        }
    }
    
    bool removeDirectory(const std::string& path) {
        try {
            return fs::remove_all(path) > 0;
        } catch (...) {
            return false;
        }
    }
    
    bool renameFile(const std::string& oldPath, const std::string& newPath) {
        try {
            fs::rename(oldPath, newPath);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool copyFile(const std::string& source, const std::string& destination) {
        try {
            fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    std::string getTimestampDirectoryName() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::tm tm_struct;
        #if PLATFORM_WINDOWS
            localtime_s(&tm_struct, &in_time_t);
        #else
            localtime_r(&in_time_t, &tm_struct);
        #endif
        
        std::ostringstream oss;
        oss << std::put_time(&tm_struct, "%Y-%m-%d_%H-%M-%S");
        return oss.str();
    }
    
} // namespace FileSystem
