#include "HealthCheck.hpp"
#include "../utils/TimeUtils.hpp"
#include "../logging/Logger.hpp"

HealthMonitor::HealthMonitor() {
}

HealthMonitor::~HealthMonitor() {
}

void HealthMonitor::registerProcess(const std::string& name, int pid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    ProcessInfo info;
    info.name = name;
    info.pid = pid;
    info.lastHeartbeat = TimeUtils::getCurrentTimestampMs();
    info.cpuUsage = 0.0;
    info.memoryUsage = 0.0;
    info.statusMessage = "Registered";
    
    m_processes[name] = info;
    LOG_INFO_F("Registered process '%s' (PID %d) for health monitoring", 
               name.c_str(), pid);
}

void HealthMonitor::unregisterProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_processes.find(name);
    if (it != m_processes.end()) {
        m_processes.erase(it);
        LOG_INFO_F("Unregistered process '%s' from health monitoring", name.c_str());
    }
}

void HealthMonitor::updateHeartbeat(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_processes.find(name);
    if (it != m_processes.end()) {
        it->second.lastHeartbeat = TimeUtils::getCurrentTimestampMs();
        it->second.isAlive = true;
    }
}

HealthMonitor::HealthStatus HealthMonitor::getStatus(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    HealthStatus status;
    
    auto it = m_processes.find(name);
    if (it != m_processes.end()) {
        const ProcessInfo& info = it->second;
        status.isAlive = info.lastHeartbeat > 0 && 
                        (TimeUtils::getCurrentTimestampMs() - info.lastHeartbeat) < 3000;
        status.lastHeartbeat = info.lastHeartbeat;
        status.cpuUsage = info.cpuUsage;
        status.memoryUsage = info.memoryUsage;
        status.statusMessage = info.statusMessage;
    }
    
    return status;
}

std::map<std::string, HealthMonitor::HealthStatus> HealthMonitor::getAllStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::map<std::string, HealthStatus> result;
    
    for (const auto& pair : m_processes) {
        result[pair.first] = getStatus(pair.first);
    }
    
    return result;
}

void HealthMonitor::checkHeartbeats(uint64_t timeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    uint64_t now = TimeUtils::getCurrentTimestampMs();
    
    for (auto& pair : m_processes) {
        ProcessInfo& info = pair.second;
        
        if (now - info.lastHeartbeat > timeoutMs) {
            info.isAlive = false;
            info.statusMessage = "No heartbeat - assumed dead";
            LOG_WARN_F("Process '%s' (PID %d) missed heartbeat", 
                       info.name.c_str(), info.pid);
            
            // Notify callbacks
            HealthStatus status = getStatus(info.name);
            notifyHealthCallbacks(info.name, status);
        }
    }
}

void HealthMonitor::setCpuUsage(const std::string& name, double cpuUsage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_processes.find(name);
    if (it != m_processes.end()) {
        it->second.cpuUsage = cpuUsage;
    }
}

void HealthMonitor::setMemoryUsage(const std::string& name, double memoryUsage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_processes.find(name);
    if (it != m_processes.end()) {
        it->second.memoryUsage = memoryUsage;
    }
}

void HealthMonitor::setStatusMessage(const std::string& name, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_processes.find(name);
    if (it != m_processes.end()) {
        it->second.statusMessage = message;
    }
}

void HealthMonitor::registerHealthCallback(
    std::function<void(const std::string&, const HealthStatus&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthCallbacks.push_back(callback);
}

size_t HealthMonitor::getProcessCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_processes.size();
}

size_t HealthMonitor::getAliveCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    size_t count = 0;
    uint64_t now = TimeUtils::getCurrentTimestampMs();
    
    for (const auto& pair : m_processes) {
        if (now - pair.second.lastHeartbeat < 3000) {
            count++;
        }
    }
    
    return count;
}

void HealthMonitor::notifyHealthCallbacks(const std::string& name, const HealthStatus& status) {
    for (auto& callback : m_healthCallbacks) {
        try {
            callback(name, status);
        } catch (...) {
            // Don't let callback errors affect others
        }
    }
}

// HeartbeatSender implementation

HeartbeatSender::HeartbeatSender(HealthMonitor* monitor, const std::string& processName, 
                               uint64_t intervalMs)
    : m_monitor(monitor), m_processName(processName), 
      m_intervalMs(intervalMs), m_running(false) {
    
    if (m_monitor) {
        // Register with monitor
        int pid = static_cast<int>(getpid());
        m_monitor->registerProcess(processName, pid);
    }
}

HeartbeatSender::~HeartbeatSender() {
    stop();
    
    if (m_monitor) {
        m_monitor->unregisterProcess(m_processName);
    }
}

void HeartbeatSender::sendHeartbeat() {
    if (m_monitor) {
        m_monitor->updateHeartbeat(m_processName);
    }
}

void HeartbeatSender::start() {
    if (m_running) return;
    
    m_running = true;
    m_thread = std::thread(&HeartbeatSender::heartbeatThread, this);
    LOG_INFO_F("Started heartbeat sender for '%s' (interval: %dms)", 
               m_processName.c_str(), static_cast<int>(m_intervalMs));
}

void HeartbeatSender::stop() {
    if (!m_running) return;
    
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    
    LOG_INFO_F("Stopped heartbeat sender for '%s'", m_processName.c_str());
}

void HeartbeatSender::heartbeatThread() {
    while (m_running) {
        sendHeartbeat();
        TimeUtils::sleepMs(static_cast<uint32_t>(m_intervalMs));
    }
}
