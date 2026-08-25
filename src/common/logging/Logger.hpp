#pragma once

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <sstream>
#include <iomanip>

/**
 * @brief Logging levels for the system
 */
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

/**
 * @brief Convert LogLevel to string
 */
const char* logLevelToString(LogLevel level);

/**
 * @brief Logger class for system-wide logging
 * 
 * Provides thread-safe logging with multiple sinks (console, file, etc.)
 * Logs are written to ./logs/<launch_time>/system/<process_name>_<pid>.log
 */
class Logger {
public:
    
    /**
     * @brief Initialize the logging system
     * @param logDir Base log directory (e.g., "./logs")
     * @param processName Name of the current process
     * @param level Minimum log level to output
     */
    static void initialize(const std::string& logDir, const std::string& processName = "", LogLevel level = LogLevel::INFO);
    
    /**
     * @brief Shutdown the logging system
     */
    static void shutdown();
    
    /**
     * @brief Set the minimum log level
     * @param level New minimum log level
     */
    static void setLevel(LogLevel level);
    
    /**
     * @brief Set the process name for logging
     * @param name Process name
     */
    static void setProcessName(const std::string& name);
    
    /**
     * @brief Set the log directory
     * @param logDir New log directory
     */
    static void setLogDirectory(const std::string& logDir);
    
    // Logging methods
    static void trace(const std::string& msg);
    static void debug(const std::string& msg);
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);
    static void critical(const std::string& msg);
    
    // Formatted logging
    template<typename... Args>
    static void trace(const char* fmt, Args... args) {
        log(LogLevel::TRACE, fmt, args...);
    }
    
    template<typename... Args>
    static void debug(const char* fmt, Args... args) {
        log(LogLevel::DEBUG, fmt, args...);
    }
    
    template<typename... Args>
    static void info(const char* fmt, Args... args) {
        log(LogLevel::INFO, fmt, args...);
    }
    
    template<typename... Args>
    static void warn(const char* fmt, Args... args) {
        log(LogLevel::WARN, fmt, args...);
    }
    
    template<typename... Args>
    static void error(const char* fmt, Args... args) {
        log(LogLevel::ERROR, fmt, args...);
    }
    
    template<typename... Args>
    static void critical(const char* fmt, Args... args) {
        log(LogLevel::CRITICAL, fmt, args...);
    }
    
    /**
     * @brief Add a log sink (output destination)
     * @param sink Shared pointer to a log sink
     */
    static void addSink(std::shared_ptr<class ILogSink> sink);
    
    /**
     * @brief Remove a log sink
     * @param sink Shared pointer to the log sink to remove
     */
    static void removeSink(std::shared_ptr<class ILogSink> sink);
    
private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Internal logging method
    template<typename... Args>
    static void log(LogLevel level, const char* fmt, Args... args);
    
    // Internal state
    struct State {
        std::string logDir;
        std::string processName;
        int processId;
        LogLevel level;
        std::vector<std::shared_ptr<class ILogSink>> sinks;
        std::mutex mutex;
        bool initialized;
    };
    
    static State& getState();
    static void ensureInitialized();
};

// Log sink interface
class ILogSink {
public:
    virtual ~ILogSink() = default;
    
    /**
     * @brief Write a log message
     * @param level Log level
     * @param timestamp Formatted timestamp
     * @param processName Process name
     * @param processId Process ID
     * @param message Log message
     */
    virtual void write(LogLevel level, const std::string& timestamp, 
                      const std::string& processName, int processId, 
                      const std::string& message) = 0;
    
    /**
     * @brief Flush any buffered output
     */
    virtual void flush() = 0;
};

// Macros for convenient logging with file and line info
#define LOG_TRACE(msg) Logger::trace("[%s:%d] %s", __FILE__, __LINE__, msg)
#define LOG_DEBUG(msg) Logger::debug("[%s:%d] %s", __FILE__, __LINE__, msg)
#define LOG_INFO(msg) Logger::info("[%s:%d] %s", __FILE__, __LINE__, msg)
#define LOG_WARN(msg) Logger::warn("[%s:%d] %s", __FILE__, __LINE__, msg)
#define LOG_ERROR(msg) Logger::error("[%s:%d] %s", __FILE__, __LINE__, msg)
#define LOG_CRITICAL(msg) Logger::critical("[%s:%d] %s", __FILE__, __LINE__, msg)

// Formatted logging macros
#define LOG_TRACE_F(fmt, ...) Logger::trace(fmt, ##__VA_ARGS__)
#define LOG_DEBUG_F(fmt, ...) Logger::debug(fmt, ##__VA_ARGS__)
#define LOG_INFO_F(fmt, ...) Logger::info(fmt, ##__VA_ARGS__)
#define LOG_WARN_F(fmt, ...) Logger::warn(fmt, ##__VA_ARGS__)
#define LOG_ERROR_F(fmt, ...) Logger::error(fmt, ##__VA_ARGS__)
#define LOG_CRITICAL_F(fmt, ...) Logger::critical(fmt, ##__VA_ARGS__)
