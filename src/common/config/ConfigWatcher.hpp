#pragma once

#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

/**
 * @brief Watches configuration files for changes and notifies callbacks
 */
class ConfigWatcher {
public:
    using ConfigCallback = std::function<void(const std::string&, const std::string&)>;
    
    /**
     * @brief Constructor
     */
    ConfigWatcher();
    
    /**
     * @brief Destructor
     */
    ~ConfigWatcher();
    
    /**
     * @brief Watch a file for changes
     * @param filePath Path to the file to watch
     * @param callback Callback to invoke when file changes
     */
    void watch(const std::string& filePath, ConfigCallback callback);
    
    /**
     * @brief Unwatch a file
     * @param filePath Path to the file to stop watching
     */
    void unwatch(const std::string& filePath);
    
    /**
     * @brief Unwatch all files
     */
    void unwatchAll();
    
    /**
     * @brief Start the watcher thread
     */
    void start();
    
    /**
     * @brief Stop the watcher thread
     */
    void stop();
    
    /**
     * @brief Check all watched files for changes (call from main thread)
     */
    void check();
    
    /**
     * @brief Set the polling interval in milliseconds
     * @param interval Polling interval
     */
    void setPollingInterval(int interval);
    
private:
    struct WatchedFile {
        std::string path;
        ConfigCallback callback;
        uint64_t lastModified;
    };
    
    std::vector<WatchedFile> m_watchedFiles;
    std::mutex m_mutex;
    std::thread m_thread;
    std::atomic<bool> m_running;
    std::condition_variable m_cv;
    int m_pollingInterval;
    
    /**
     * @brief Thread function for watching files
     */
    void watchThread();
    
    /**
     * @brief Check if a file has changed
     * @param file Watched file info
     * @return true if file has changed
     */
    bool hasChanged(WatchedFile& file);
    
    /**
     * @brief Get current modification time of a file
     * @param filePath Path to the file
     * @return Modification time as timestamp
     */
    uint64_t getLastModified(const std::string& filePath);
};
