#ifndef COIL_UTIL_LOGGER_H
#define COIL_UTIL_LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <memory>

namespace coil {

/**
 * @brief Log level
 */
enum LogLevel {
    LOG_DEBUG,   // Debug messages (verbose)
    LOG_INFO,    // Informational messages
    LOG_WARNING, // Warning messages
    LOG_ERROR,   // Error messages
    LOG_FATAL    // Fatal error messages
};

/**
 * @brief Logger interface
 */
class Logger {
public:
    /**
     * @brief Destroy the Logger
     */
    virtual ~Logger() = default;
    
    /**
     * @brief Log a message
     * 
     * @param level Log level
     * @param message Message to log
     */
    virtual void log(LogLevel level, const std::string& message) = 0;
    
    /**
     * @brief Check if a log level is enabled
     * 
     * @param level Log level
     * @return true if enabled, false otherwise
     */
    virtual bool isEnabled(LogLevel level) const = 0;
    
    /**
     * @brief Set the minimum log level
     * 
     * @param level Minimum log level
     */
    virtual void setMinLevel(LogLevel level) = 0;
};

/**
 * @brief Console logger
 * 
 * Logs messages to the console.
 */
class ConsoleLogger : public Logger {
private:
    LogLevel minLevel;  // Minimum log level

public:
    /**
     * @brief Construct a new Console Logger
     * 
     * @param level Minimum log level
     */
    explicit ConsoleLogger(LogLevel level = LOG_INFO);
    
    void log(LogLevel level, const std::string& message) override;
    bool isEnabled(LogLevel level) const override;
    void setMinLevel(LogLevel level) override;
};

/**
 * @brief File logger
 * 
 * Logs messages to a file.
 */
class FileLogger : public Logger {
private:
    LogLevel minLevel;  // Minimum log level
    std::ofstream file; // Output file

public:
    /**
     * @brief Construct a new File Logger
     * 
     * @param filename Output filename
     * @param level Minimum log level
     */
    FileLogger(const std::string& filename, LogLevel level = LOG_INFO);
    
    /**
     * @brief Destroy the File Logger
     */
    ~FileLogger() override;
    
    void log(LogLevel level, const std::string& message) override;
    bool isEnabled(LogLevel level) const override;
    void setMinLevel(LogLevel level) override;
};

/**
 * @brief Global logger
 * 
 * Provides access to the global logger instance.
 */
class GlobalLogger {
private:
    static std::unique_ptr<Logger> instance;

public:
    /**
     * @brief Set the global logger instance
     * 
     * @param logger Logger instance
     */
    static void setInstance(std::unique_ptr<Logger> logger);
    
    /**
     * @brief Get the global logger instance
     * 
     * @return Logger instance
     */
    static Logger* getInstance();
    
    /**
     * @brief Log a message
     * 
     * @param level Log level
     * @param message Message to log
     */
    static void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Check if a log level is enabled
     * 
     * @param level Log level
     * @return true if enabled, false otherwise
     */
    static bool isEnabled(LogLevel level);
};

// Helper macros for logging
#define LOG_DEBUG(message) if (coil::GlobalLogger::isEnabled(coil::LOG_DEBUG)) coil::GlobalLogger::log(coil::LOG_DEBUG, message)
#define LOG_INFO(message) if (coil::GlobalLogger::isEnabled(coil::LOG_INFO)) coil::GlobalLogger::log(coil::LOG_INFO, message)
#define LOG_WARNING(message) if (coil::GlobalLogger::isEnabled(coil::LOG_WARNING)) coil::GlobalLogger::log(coil::LOG_WARNING, message)
#define LOG_ERROR(message) if (coil::GlobalLogger::isEnabled(coil::LOG_ERROR)) coil::GlobalLogger::log(coil::LOG_ERROR, message)
#define LOG_FATAL(message) if (coil::GlobalLogger::isEnabled(coil::LOG_FATAL)) coil::GlobalLogger::log(coil::LOG_FATAL, message)

} // namespace coil

#endif // COIL_UTIL_LOGGER_H