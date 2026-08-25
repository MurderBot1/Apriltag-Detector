#include "ConfigWatcher.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/TimeUtils.hpp"
#include <chrono>

ConfigWatcher::ConfigWatcher() 
    : m_running(false), m_pollingInterval(100) { // Default 100ms polling
}

ConfigWatcher::~ConfigWatcher() {
    stop();
}

void ConfigWatcher::watch(const std::string& filePath, ConfigCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if already watching this file
    for (auto& file : m_watchedFiles) {
        if (file.path == filePath) {
            file.callback = callback;
            file.lastModified = getLastModified(filePath);
            return;
        }
    }
    
    // Add new watched file
    WatchedFile file;
    file.path = filePath;
    file.callback = callback;
    file.lastModified = getLastModified(filePath);
    m_watchedFiles.push_back(file);
}

void ConfigWatcher::unwatch(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = std::remove_if(m_watchedFiles.begin(), m_watchedFiles.end(),
        [&filePath](const WatchedFile& f) { return f.path == filePath; });
    
    if (it != m_watchedFiles.end()) {
        m_watchedFiles.erase(it, m_watchedFiles.end());
    }
}

void ConfigWatcher::unwatchAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchedFiles.clear();
}

void ConfigWatcher::start() {
    if (m_running) return;
    
    m_running = true;
    m_thread = std::thread(&ConfigWatcher::watchThread, this);
}

void ConfigWatcher::stop() {
    if (!m_running) return;
    
    m_running = false;
    m_cv.notify_all();
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ConfigWatcher::check() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& file : m_watchedFiles) {
        if (hasChanged(file)) {
            try {
                std::string content = FileSystem::readFile(file.path);
                file.callback(file.path, content);
                file.lastModified = getLastModified(file.path);
            } catch (...) {
                // Ignore errors
            }
        }
    }
}

void ConfigWatcher::setPollingInterval(int interval) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pollingInterval = interval;
}

void ConfigWatcher::watchThread() {
    while (m_running) {
        check();
        
        // Sleep for polling interval
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait_for(lock, std::chrono::milliseconds(m_pollingInterval),
            [this]() { return !m_running; });
    }
}

bool ConfigWatcher::hasChanged(WatchedFile& file) {
    uint64_t currentMod = getLastModified(file.path);
    if (currentMod == 0) {
        // File doesn't exist or error
        return false;
    }
    
    if (file.lastModified == 0) {
        // First check - file just started being watched
        file.lastModified = currentMod;
        return false;
    }
    
    return currentMod > file.lastModified;
}

uint64_t ConfigWatcher::getLastModified(const std::string& filePath) {
    if (!FileSystem::exists(filePath)) {
        return 0;
    }
    
    return FileSystem::getLastWriteTime(filePath);
}
