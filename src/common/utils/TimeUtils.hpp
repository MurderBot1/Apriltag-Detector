#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace TimeUtils {
    
    // Get current timestamp in microseconds
    uint64_t getCurrentTimestampUs();
    
    // Get current timestamp in milliseconds
    uint64_t getCurrentTimestampMs();
    
    // Get current timestamp in seconds
    uint64_t getCurrentTimestampS();
    
    // Get current time as formatted string
    std::string getCurrentTimeString();
    
    // Get time since epoch in various formats
    std::string getIso8601Timestamp();
    
    // Sleep for milliseconds
    void sleepMs(uint32_t milliseconds);
    
    // Sleep for microseconds
    void sleepUs(uint32_t microseconds);
    
    // High resolution timer
    class HighResTimer {
    public:
        HighResTimer();
        
        // Start or restart the timer
        void start();
        
        // Stop the timer
        void stop();
        
        // Get elapsed time in nanoseconds
        uint64_t getElapsedNs() const;
        
        // Get elapsed time in microseconds
        uint64_t getElapsedUs() const;
        
        // Get elapsed time in milliseconds
        uint64_t getElapsedMs() const;
        
        // Get elapsed time in seconds
        double getElapsedS() const;
        
        // Check if timer is running
        bool isRunning() const;
        
    private:
        std::chrono::high_resolution_clock::time_point m_start;
        std::chrono::high_resolution_clock::time_point m_end;
        bool m_running;
    };
    
    // Scoped timer for measuring function execution time
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& name);
        ~ScopedTimer();
        
    private:
        std::string m_name;
        HighResTimer m_timer;
    };
    
} // namespace TimeUtils
