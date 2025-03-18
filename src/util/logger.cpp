#include "util/logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>

namespace coil {

// Global logger instance
std::unique_ptr<Logger> GlobalLogger::instance = std::make_unique<ConsoleLogger>();

// Console logger implementation
ConsoleLogger::ConsoleLogger(LogLevel level)
    : minLevel(level) {
}

void ConsoleLogger::log(LogLevel level, const std::string& message) {
    if (level < minLevel) {
        return;
    }
    
    // Get current time
    std::time_t now = std::time(nullptr);
    std::tm tm_now;
    
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    
    // Format time
    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &tm_now);
    
    // Select stream and log level prefix
    std::ostream& stream = (level >= LOG_ERROR) ? std::cerr : std::cout;
    const char* levelStr = "";
    
    switch (level) {
        case LOG_DEBUG:   levelStr = "DEBUG"; break;
        case LOG_INFO:    levelStr = "INFO"; break;
        case LOG_WARNING: levelStr = "WARNING"; break;
        case LOG_ERROR:   levelStr = "ERROR"; break;
        case LOG_FATAL:   levelStr = "FATAL"; break;
    }
    
    // Output the log message
    stream << "[" << timeBuffer << "] " << std::setw(7) << std::left << levelStr << " " << message << std::endl;
}

bool ConsoleLogger::isEnabled(LogLevel level) const {
    return level >= minLevel;
}

void ConsoleLogger::setMinLevel(LogLevel level) {
    minLevel = level;
}

// File logger implementation
FileLogger::FileLogger(const std::string& filename, LogLevel level)
    : minLevel(level), file(filename) {
}

FileLogger::~FileLogger() {
    if (file.is_open()) {
        file.close();
    }
}

void FileLogger::log(LogLevel level, const std::string& message) {
    if (level < minLevel || !file.is_open()) {
        return;
    }
    
    // Get current time
    std::time_t now = std::time(nullptr);
    std::tm tm_now;
    
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    
    // Format time
    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &tm_now);
    
    // Select log level prefix
    const char* levelStr = "";
    
    switch (level) {
        case LOG_DEBUG:   levelStr = "DEBUG"; break;
        case LOG_INFO:    levelStr = "INFO"; break;
        case LOG_WARNING: levelStr = "WARNING"; break;
        case LOG_ERROR:   levelStr = "ERROR"; break;
        case LOG_FATAL:   levelStr = "FATAL"; break;
    }
    
    // Output the log message
    file << "[" << timeBuffer << "] " << std::setw(7) << std::left << levelStr << " " << message << std::endl;
}

bool FileLogger::isEnabled(LogLevel level) const {
    return level >= minLevel && file.is_open();
}

void FileLogger::setMinLevel(LogLevel level) {
    minLevel = level;
}

// Global logger implementation
void GlobalLogger::setInstance(std::unique_ptr<Logger> logger) {
    instance = std::move(logger);
}

Logger* GlobalLogger::getInstance() {
    return instance.get();
}

void GlobalLogger::log(LogLevel level, const std::string& message) {
    if (instance) {
        instance->log(level, message);
    }
}

bool GlobalLogger::isEnabled(LogLevel level) {
    return instance && instance->isEnabled(level);
}

} // namespace coil