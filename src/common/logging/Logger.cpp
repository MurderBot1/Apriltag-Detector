#include "Logger.hpp"
#include "FileSink.hpp"
#include "../utils/FileSystem.hpp"
#include "../utils/TimeUtils.hpp"
#include "../utils/Platform.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// Static state
Logger::State& Logger::getState() {
    static State state;
    return state;
}

const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

void Logger::initialize(const std::string& logDir, const std::string& processName, LogLevel level) {
    State& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    
    if (state.initialized) {
        shutdown();
    }
    
    state.logDir = logDir;
    state.processName = processName;
    state.level = level;
    state.processId = static_cast<int>(Platform::getPathSeparator() == '\\' ? 
        static_cast<long long>(GetCurrentProcessId()) : 
        static_cast<long long>(getpid()));
    
    // Create log directory structure: ./logs/<timestamp>/system/
    std::string timestampDir = FileSystem::getTimestampDirectoryName();
    std::string systemDir = FileSystem::joinPaths({state.logDir, timestampDir, "system"});
    FileSystem::createDirectories(systemDir);
    
    // Create file sink
    std::string logFile = FileSystem::joinPaths({systemDir, 
        state.processName.empty() ? "application" : state.processName + 
        "_" + std::to_string(state.processId) + ".log"});
    
    auto fileSink = std::make_shared<FileSink>(logFile);
    state.sinks.push_back(fileSink);
    
    state.initialized = true;
}

void Logger::shutdown() {
    State& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    
    // Flush and clear all sinks
    for (auto& sink : state.sinks) {
        sink->flush();
    }
    state.sinks.clear();
    state.initialized = false;
}

void Logger::setLevel(LogLevel level) {
    State& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.level = level;
}

void Logger::setProcessName(const std::string& name) {
    State& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.processName = name;
}

void Logger::setLogDirectory(const std::string& logDir) {
    State& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.logDir = logDir;
}

void Logger::addSink(std::shared_ptr<ILogSink> sink) {
    State& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.sinks.push_back(sink);
}

void Logger::removeSink(std::shared_ptr<ILogSink> sink) {
    State& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    auto it = std::find(state.sinks.begin(), state.sinks.end(), sink);
    if (it != state.sinks.end()) {
        state.sinks.erase(it);
    }
}

void Logger::ensureInitialized() {
    State& state = getState();
    if (!state.initialized) {
        // Default initialization
        initialize("./logs", "", LogLevel::INFO);
    }
}

void Logger::trace(const std::string& msg) {
    log(LogLevel::TRACE, "%s", msg.c_str());
}

void Logger::debug(const std::string& msg) {
    log(LogLevel::DEBUG, "%s", msg.c_str());
}

void Logger::info(const std::string& msg) {
    log(LogLevel::INFO, "%s", msg.c_str());
}

void Logger::warn(const std::string& msg) {
    log(LogLevel::WARN, "%s", msg.c_str());
}

void Logger::error(const std::string& msg) {
    log(LogLevel::ERROR, "%s", msg.c_str());
}

void Logger::critical(const std::string& msg) {
    log(LogLevel::CRITICAL, "%s", msg.c_str());
}

// Helper function to format timestamp
static std::string formatTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm_struct;
    #if defined(_WIN32)
        localtime_s(&tm_struct, &in_time_t);
    #else
        localtime_r(&in_time_t, &tm_struct);
    #endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_struct, "%Y-%m-%d %H:%M:%S");
    
    // Add milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    oss << "." << std::setw(3) << std::setfill('0') << ms.count();
    
    return oss.str();
}

// Helper function to format log message
template<typename... Args>
static std::string formatMessage(const char* fmt, Args... args) {
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer), fmt, args...);
    if (len < 0 || static_cast<size_t>(len) >= sizeof(buffer)) {
        return std::string(fmt) + " (truncated)";
    }
    return std::string(buffer, len);
}

template<typename... Args>
void Logger::log(LogLevel level, const char* fmt, Args... args) {
    State& state = getState();
    
    if (level < state.level) {
        return; // Skip if below minimum level
    }
    
    ensureInitialized();
    
    std::string timestamp = formatTimestamp();
    std::string message = formatMessage(fmt, args...);
    std::string levelStr = logLevelToString(level);
    
    std::lock_guard<std::mutex> lock(state.mutex);
    
    for (auto& sink : state.sinks) {
        sink->write(level, timestamp, state.processName, state.processId, 
                    "[" + levelStr + "] [" + timestamp + "] [" + 
                    state.processName + ":" + std::to_string(state.processId) + "] " + message);
    }
}
