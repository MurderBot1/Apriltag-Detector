#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <mutex>
#include <functional>

/**
 * @brief Health monitoring for processes
 */
class HealthMonitor {
public:
    
    /**
     * @brief Health status for a process
     */
    struct HealthStatus {
        bool isAlive;
        uint64_t lastHeartbeat;
        double cpuUsage;
        double memoryUsage;
        std::string statusMessage;
        
        HealthStatus() 
            : isAlive(false), lastHeartbeat(0), 
              cpuUsage(0.0), memoryUsage(0.0) {}
    };
    
    /**
     * @brief Constructor
     */
    HealthMonitor();
    
    /**
     * @brief Destructor
     */
    ~HealthMonitor();
    
    /**
     * @brief Register a process for health monitoring
     * @param name Process name
     * @param pid Process ID
     */
    void registerProcess(const std::string& name, int pid);
    
    /**
     * @brief Unregister a process
     * @param name Process name
     */
    void unregisterProcess(const std::string& name);
    
    /**
     * @brief Update heartbeat for a process
     * @param name Process name
     */
    void updateHeartbeat(const std::string& name);
    
    /**
     * @brief Get health status for a process
     * @param name Process name
     * @return HealthStatus
     */
    HealthStatus getStatus(const std::string& name) const;
    
    /**
     * @brief Get all health statuses
     * @return Map of process name to HealthStatus
     */
    std::map<std::string, HealthStatus> getAllStatus() const;
    
    /**
     * @brief Check all heartbeats and mark stale processes
     * @param timeoutMs Heartbeat timeout in milliseconds
     */
    void checkHeartbeats(uint64_t timeoutMs = 3000);
    
    /**
     * @brief Set CPU usage for a process
     * @param name Process name
     * @param cpuUsage CPU usage percentage (0-100)
     */
    void setCpuUsage(const std::string& name, double cpuUsage);
    
    /**
     * @brief Set memory usage for a process
     * @param name Process name
     * @param memoryUsage Memory usage in MB
     */
    void setMemoryUsage(const std::string& name, double memoryUsage);
    
    /**
     * @brief Set status message for a process
     * @param name Process name
     * @param message Status message
     */
    void setStatusMessage(const std::string& name, const std::string& message);
    
    /**
     * @brief Register a callback for health status changes
     * @param callback Callback function
     */
    void registerHealthCallback(std::function<void(const std::string&, const HealthStatus&)> callback);
    
    /**
     * @brief Get the number of registered processes
     * @return Number of processes
     */
    size_t getProcessCount() const;
    
    /**
     * @brief Get the number of alive processes
     * @return Number of alive processes
     */
    size_t getAliveCount() const;
    
private:
    struct ProcessInfo {
        std::string name;
        int pid;
        uint64_t lastHeartbeat;
        double cpuUsage;
        double memoryUsage;
        std::string statusMessage;
    };
    
    std::map<std::string, ProcessInfo> m_processes;
    mutable std::mutex m_mutex;
    std::vector<std::function<void(const std::string&, const HealthStatus&)>> m_healthCallbacks;
    
    /**
     * @brief Notify health callbacks
     * @param name Process name
     * @param status Health status
     */
    void notifyHealthCallbacks(const std::string& name, const HealthStatus& status);
};

/**
 * @brief Heartbeat sender for processes
 * 
 * Each process should create one of these and call sendHeartbeat() periodically
 */
class HeartbeatSender {
public:
    /**
     * @brief Constructor
     * @param monitor HealthMonitor instance
     * @param processName Process name
     * @param intervalMs Heartbeat interval in milliseconds
     */
    HeartbeatSender(HealthMonitor* monitor, const std::string& processName, 
                   uint64_t intervalMs = 1000);
    
    /**
     * @brief Destructor
     */
    ~HeartbeatSender();
    
    /**
     * @brief Send a heartbeat
     */
    void sendHeartbeat();
    
    /**
     * @brief Start automatic heartbeat sending
     */
    void start();
    
    /**
     * @brief Stop automatic heartbeat sending
     */
    void stop();
    
private:
    HealthMonitor* m_monitor;
    std::string m_processName;
    uint64_t m_intervalMs;
    std::thread m_thread;
    std::atomic<bool> m_running;
    
    /**
     * @brief Heartbeat thread function
     */
    void heartbeatThread();
};
