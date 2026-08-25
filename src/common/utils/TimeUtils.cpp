#include "TimeUtils.hpp"
#include <iomanip>
#include <sstream>
#include <thread>

namespace TimeUtils {
    
    uint64_t getCurrentTimestampUs() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
    }
    
    uint64_t getCurrentTimestampMs() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }
    
    uint64_t getCurrentTimestampS() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
    }
    
    std::string getCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::tm tm_struct;
        #if defined(_WIN32)
            localtime_s(&tm_struct, &in_time_t);
        #else
            localtime_r(&in_time_t, &tm_struct);
        #endif
        
        std::ostringstream oss;
        oss << std::put_time(&tm_struct, "%H:%M:%S");
        return oss.str();
    }
    
    std::string getIso8601Timestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::tm tm_struct;
        #if defined(_WIN32)
            gmtime_s(&tm_struct, &in_time_t);
        #else
            gmtime_r(&in_time_t, &tm_struct);
        #endif
        
        std::ostringstream oss;
        oss << std::put_time(&tm_struct, "%Y-%m-%dT%H:%M:%S") << "." 
            << std::setw(3) << std::setfill('0') 
            << std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count() % 1000 << "Z";
        return oss.str();
    }
    
    void sleepMs(uint32_t milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    
    void sleepUs(uint32_t microseconds) {
        std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
    }
    
    HighResTimer::HighResTimer() : m_running(false) {}
    
    void HighResTimer::start() {
        m_start = std::chrono::high_resolution_clock::now();
        m_running = true;
    }
    
    void HighResTimer::stop() {
        m_end = std::chrono::high_resolution_clock::now();
        m_running = false;
    }
    
    uint64_t HighResTimer::getElapsedNs() const {
        auto end = m_running ? std::chrono::high_resolution_clock::now() : m_end;
        auto duration = end - m_start;
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
    }
    
    uint64_t HighResTimer::getElapsedUs() const {
        return getElapsedNs() / 1000;
    }
    
    uint64_t HighResTimer::getElapsedMs() const {
        return getElapsedNs() / 1000000;
    }
    
    double HighResTimer::getElapsedS() const {
        return static_cast<double>(getElapsedNs()) / 1000000000.0;
    }
    
    bool HighResTimer::isRunning() const {
        return m_running;
    }
    
    ScopedTimer::ScopedTimer(const std::string& name) : m_name(name) {
        m_timer.start();
    }
    
    ScopedTimer::~ScopedTimer() {
        m_timer.stop();
        // Log the timing (in a real implementation, this would use the Logger)
    }
    
} // namespace TimeUtils
