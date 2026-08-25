#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

/**
 * @brief Manages child processes with self-healing capabilities
 */
class ProcessManager {
public:
    
    /**
     * @brief Process configuration
     */
    struct ProcessConfig {
        std::string name;              // Process name
        std::string executable;        // Executable path
        std::vector<std::string> arguments;  // Command line arguments
        std::string workingDirectory;  // Working directory
        int restartPolicy;             // 0=none, 1=always, 2=on_failure
        int maxRestartAttempts;       // Maximum restart attempts
        int restartDelayMs;           // Delay between restarts in ms
    };
    
    /**
     * @brief Process status
     */
    struct ProcessStatus {
        bool isRunning;
        int pid;
        int exitCode;
        int restartCount;
        uint64_t lastStartTime;
        uint64_t lastExitTime;
        std::string statusMessage;
    };
    
    /**
     * @brief Constructor
     */
    ProcessManager();
    
    /**
     * @brief Destructor
     */
    ~ProcessManager();
    
    /**
     * @brief Start a process
     * @param config Process configuration
     * @return true if started successfully
     */
    bool startProcess(const ProcessConfig& config);
    
    /**
     * @brief Start multiple processes
     * @param configs Vector of process configurations
     */
    void startProcesses(const std::vector<ProcessConfig>& configs);
    
    /**
     * @brief Stop a process by name
     * @param name Process name
     * @return true if stopped successfully
     */
    bool stopProcess(const std::string& name);
    
    /**
     * @brief Stop all processes
     */
    void stopAll();
    
    /**
     * @brief Check if a process is running
     * @param name Process name
     * @return true if running
     */
    bool isRunning(const std::string& name) const;
    
    /**
     * @brief Get process ID by name
     * @param name Process name
     * @return Process ID or -1 if not found
     */
    int getPid(const std::string& name) const;
    
    /**
     * @brief Get process status by name
     * @param name Process name
     * @return ProcessStatus
     */
    ProcessStatus getStatus(const std::string& name) const;
    
    /**
     * @brief Get all process statuses
     * @return Map of process name to status
     */
    std::map<std::string, ProcessStatus> getAllStatus() const;
    
    /**
     * @brief Start the monitoring thread
     */
    void startMonitoring();
    
    /**
     * @brief Stop the monitoring thread
     */
    void stopMonitoring();
    
    /**
     * @brief Set global restart policy
     * @param policy 0=none, 1=always, 2=on_failure
     */
    void setGlobalRestartPolicy(int policy);
    
    /**
     * @brief Set global restart delay
     * @param delayMs Delay in milliseconds
     */
    void setGlobalRestartDelay(int delayMs);
    
    /**
     * @brief Set global max restart attempts
     * @param maxAttempts Maximum attempts
     */
    void setGlobalMaxRestartAttempts(int maxAttempts);
    
    /**
     * @brief Register a callback for process status changes
     * @param callback Callback function
     */
    void registerStatusCallback(std::function<void(const std::string&, const ProcessStatus&)> callback);
    
private:
    struct ProcessInfo {
        ProcessConfig config;
        int pid;
        int exitCode;
        int restartCount;
        uint64_t lastStartTime;
        uint64_t lastExitTime;
        bool isRunning;
        bool shouldRestart;
        std::string statusMessage;
    };
    
    std::map<std::string, ProcessInfo> m_processes;
    std::mutex m_mutex;
    std::thread m_monitorThread;
    std::atomic<bool> m_monitoring;
    std::condition_variable m_cv;
    int m_globalRestartPolicy;
    int m_globalRestartDelayMs;
    int m_globalMaxRestartAttempts;
    
    std::vector<std::function<void(const std::string&, const ProcessStatus&)>> m_statusCallbacks;
    
    /**
     * @brief Start a single process
     * @param info Process info
     * @return true if started
     */
    bool startProcessInternal(ProcessInfo& info);
    
    /**
     * @brief Stop a single process
     * @param info Process info
     */
    void stopProcessInternal(ProcessInfo& info);
    
    /**
     * @brief Monitor thread function
     */
    void monitorThread();
    
    /**
     * @brief Check if a process needs to be restarted
     * @param info Process info
     */
    void checkRestart(ProcessInfo& info);
    
    /**
     * @brief Notify status callbacks
     * @param name Process name
     * @param status Process status
     */
    void notifyStatusCallbacks(const std::string& name, const ProcessStatus& status);
    
    /**
     * @brief Generate unique process name
     * @param baseName Base name
     * @return Unique name
     */
    std::string generateUniqueName(const std::string& baseName);
};
