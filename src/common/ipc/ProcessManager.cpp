#include "ProcessManager.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/TimeUtils.hpp"
#include "../logging/Logger.hpp"
#include "../utils/Platform.hpp"
#include <cstdlib>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#if PLATFORM_WINDOWS
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

ProcessManager::ProcessManager() 
    : m_monitoring(false), 
      m_globalRestartPolicy(2), // on_failure
      m_globalRestartDelayMs(1000),
      m_globalMaxRestartAttempts(5) {
}

ProcessManager::~ProcessManager() {
    stopMonitoring();
    stopAll();
}

bool ProcessManager::startProcess(const ProcessConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if already running
    if (m_processes.find(config.name) != m_processes.end()) {
        if (m_processes[config.name].isRunning) {
            LOG_WARN_F("Process '%s' is already running", config.name.c_str());
            return false;
        }
    }
    
    // Create process info
    ProcessInfo info;
    info.config = config;
    info.pid = -1;
    info.exitCode = 0;
    info.restartCount = 0;
    info.lastStartTime = 0;
    info.lastExitTime = 0;
    info.isRunning = false;
    info.shouldRestart = false;
    info.statusMessage = "Starting...";
    
    // Use global settings if not specified
    if (config.restartPolicy < 0) {
        info.config.restartPolicy = m_globalRestartPolicy;
    }
    if (config.restartDelayMs <= 0) {
        info.config.restartDelayMs = m_globalRestartDelayMs;
    }
    if (config.maxRestartAttempts <= 0) {
        info.config.maxRestartAttempts = m_globalMaxRestartAttempts;
    }
    
    // Start the process
    bool success = startProcessInternal(info);
    
    if (success) {
        m_processes[config.name] = info;
        LOG_INFO_F("Started process '%s' with PID %d", config.name.c_str(), info.pid);
    } else {
        LOG_ERROR_F("Failed to start process '%s'", config.name.c_str());
    }
    
    return success;
}

void ProcessManager::startProcesses(const std::vector<ProcessConfig>& configs) {
    for (const auto& config : configs) {
        startProcess(config);
    }
}

bool ProcessManager::stopProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_processes.find(name);
    if (it == m_processes.end()) {
        LOG_WARN_F("Process '%s' not found", name.c_str());
        return false;
    }
    
    ProcessInfo& info = it->second;
    stopProcessInternal(info);
    
    LOG_INFO_F("Stopped process '%s'", name.c_str());
    return true;
}

void ProcessManager::stopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& pair : m_processes) {
        stopProcessInternal(pair.second);
    }
    
    m_processes.clear();
    LOG_INFO("Stopped all processes");
}

bool ProcessManager::isRunning(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_processes.find(name);
    if (it == m_processes.end()) {
        return false;
    }
    
    return it->second.isRunning;
}

int ProcessManager::getPid(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_processes.find(name);
    if (it == m_processes.end()) {
        return -1;
    }
    
    return it->second.pid;
}

ProcessManager::ProcessStatus ProcessManager::getStatus(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    ProcessStatus status;
    status.isRunning = false;
    status.pid = -1;
    status.exitCode = 0;
    status.restartCount = 0;
    status.lastStartTime = 0;
    status.lastExitTime = 0;
    status.statusMessage = "Not found";
    
    auto it = m_processes.find(name);
    if (it != m_processes.end()) {
        const ProcessInfo& info = it->second;
        status.isRunning = info.isRunning;
        status.pid = info.pid;
        status.exitCode = info.exitCode;
        status.restartCount = info.restartCount;
        status.lastStartTime = info.lastStartTime;
        status.lastExitTime = info.lastExitTime;
        status.statusMessage = info.statusMessage;
    }
    
    return status;
}

std::map<std::string, ProcessManager::ProcessStatus> ProcessManager::getAllStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::map<std::string, ProcessStatus> result;
    
    for (const auto& pair : m_processes) {
        result[pair.first] = getStatus(pair.first);
    }
    
    return result;
}

void ProcessManager::startMonitoring() {
    if (m_monitoring) return;
    
    m_monitoring = true;
    m_monitorThread = std::thread(&ProcessManager::monitorThread, this);
    LOG_INFO("Started process monitoring thread");
}

void ProcessManager::stopMonitoring() {
    if (!m_monitoring) return;
    
    m_monitoring = false;
    m_cv.notify_all();
    
    if (m_monitorThread.joinable()) {
        m_monitorThread.join();
    }
    
    LOG_INFO("Stopped process monitoring thread");
}

void ProcessManager::setGlobalRestartPolicy(int policy) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_globalRestartPolicy = policy;
}

void ProcessManager::setGlobalRestartDelay(int delayMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_globalRestartDelayMs = delayMs;
}

void ProcessManager::setGlobalMaxRestartAttempts(int maxAttempts) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_globalMaxRestartAttempts = maxAttempts;
}

void ProcessManager::registerStatusCallback(
    std::function<void(const std::string&, const ProcessStatus&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_statusCallbacks.push_back(callback);
}

bool ProcessManager::startProcessInternal(ProcessInfo& info) {
    // Build command line
    std::string command = info.config.executable;
    for (const auto& arg : info.config.arguments) {
        command += " ";
        // Quote arguments with spaces
        if (arg.find(' ') != std::string::npos) {
            command += '"' + arg + '"';
        } else {
            command += arg;
        }
    }
    
    LOG_DEBUG_F("Starting process: %s", command.c_str());
    
    // Fork and exec
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        LOG_ERROR_F("fork() failed for '%s': %s", info.config.name.c_str(), strerror(errno));
        info.statusMessage = "Fork failed";
        return false;
    }
    
    if (pid == 0) {
        // Child process
        
        // Change working directory if specified
        if (!info.config.workingDirectory.empty()) {
            chdir(info.config.workingDirectory.c_str());
        }
        
        // Set process name for logging
        // Note: This is simplified; in production you'd use prctl or similar
        
        // Execute the command
        execlp(info.config.executable.c_str(), info.config.executable.c_str(), 
               nullptr);
        
        // If we get here, exec failed
        LOG_ERROR_F("exec() failed for '%s': %s", info.config.name.c_str(), strerror(errno));
        _exit(1);
    }
    
    // Parent process
    info.pid = pid;
    info.lastStartTime = TimeUtils::getCurrentTimestampMs();
    info.restartCount = 0;
    info.isRunning = true;
    info.shouldRestart = false;
    info.statusMessage = "Running";
    
    // Notify callbacks
    ProcessStatus status = getStatus(info.config.name);
    notifyStatusCallbacks(info.config.name, status);
    
    return true;
}

void ProcessManager::stopProcessInternal(ProcessInfo& info) {
    if (!info.isRunning || info.pid <= 0) {
        return;
    }
    
    // Send SIGTERM
    kill(info.pid, SIGTERM);
    
    // Wait for process to exit
    int status;
    pid_t result = waitpid(info.pid, &status, WNOHANG);
    
    if (result == 0) {
        // Process still running, give it a moment
        TimeUtils::sleepMs(100);
        result = waitpid(info.pid, &status, WNOHANG);
    }
    
    if (result == 0) {
        // Still running, send SIGKILL
        kill(info.pid, SIGKILL);
        waitpid(info.pid, &status, 0);
    }
    
    // Update info
    info.isRunning = false;
    info.pid = -1;
    info.exitCode = WEXITSTATUS(status);
    info.lastExitTime = TimeUtils::getCurrentTimestampMs();
    info.statusMessage = "Stopped";
    
    // Notify callbacks
    ProcessStatus status = getStatus(info.config.name);
    notifyStatusCallbacks(info.config.name, status);
}

void ProcessManager::monitorThread() {
    while (m_monitoring) {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // Check each process
        for (auto& pair : m_processes) {
            ProcessInfo& info = pair.second;
            
            if (!info.isRunning) {
                continue;
            }
            
            // Check if process is still alive
            int status;
            pid_t result = waitpid(info.pid, &status, WNOHANG);
            
            if (result == info.pid) {
                // Process exited
                info.isRunning = false;
                info.exitCode = WEXITSTATUS(status);
                info.lastExitTime = TimeUtils::getCurrentTimestampMs();
                info.restartCount++;
                
                // Determine if we should restart
                bool shouldRestart = false;
                if (info.config.restartPolicy == 1) {
                    // Always restart
                    shouldRestart = true;
                } else if (info.config.restartPolicy == 2) {
                    // Restart on failure (non-zero exit)
                    shouldRestart = (info.exitCode != 0);
                }
                
                // Check max attempts
                if (shouldRestart && info.restartCount >= info.config.maxRestartAttempts) {
                    shouldRestart = false;
                    info.statusMessage = "Max restart attempts reached";
                    LOG_ERROR_F("Process '%s' reached max restart attempts (%d)", 
                               info.config.name.c_str(), info.config.maxRestartAttempts);
                }
                
                if (shouldRestart) {
                    info.shouldRestart = true;
                    info.statusMessage = "Crashed, will restart";
                    LOG_WARN_F("Process '%s' crashed with exit code %d, restarting...", 
                              info.config.name.c_str(), info.exitCode);
                } else {
                    info.statusMessage = "Exited";
                    LOG_INFO_F("Process '%s' exited with code %d", 
                              info.config.name.c_str(), info.exitCode);
                }
                
                // Notify callbacks
                ProcessStatus status = getStatus(info.config.name);
                notifyStatusCallbacks(info.config.name, status);
            }
        }
        
        // Unlock while sleeping
        lock.unlock();
        
        // Sleep for a bit
        TimeUtils::sleepMs(100);
        
        // Lock again for next iteration
        lock.lock();
        
        // Check if any processes need restarting
        for (auto& pair : m_processes) {
            ProcessInfo& info = pair.second;
            
            if (info.shouldRestart && !info.isRunning) {
                // Wait for restart delay
                uint64_t elapsed = TimeUtils::getCurrentTimestampMs() - info.lastExitTime;
                if (elapsed >= static_cast<uint64_t>(info.config.restartDelayMs)) {
                    LOG_INFO_F("Restarting process '%s' (attempt %d)", 
                              info.config.name.c_str(), info.restartCount + 1);
                    
                    bool success = startProcessInternal(info);
                    if (success) {
                        info.shouldRestart = false;
                        info.statusMessage = "Running";
                    } else {
                        LOG_ERROR_F("Failed to restart process '%s'", 
                                   info.config.name.c_str());
                    }
                }
            }
        }
    }
    
    LOG_INFO("Process monitoring thread exiting");
}

void ProcessManager::checkRestart(ProcessInfo& info) {
    // This is handled in monitorThread
}

void ProcessManager::notifyStatusCallbacks(const std::string& name, const ProcessStatus& status) {
    for (auto& callback : m_statusCallbacks) {
        try {
            callback(name, status);
        } catch (...) {
            // Don't let callback errors affect others
        }
    }
}

std::string ProcessManager::generateUniqueName(const std::string& baseName) {
    // Check if name already exists
    std::string name = baseName;
    int suffix = 1;
    
    while (m_processes.find(name) != m_processes.end()) {
        name = baseName + "_" + std::to_string(suffix++);
    }
    
    return name;
}
