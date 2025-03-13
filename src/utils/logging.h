/**
 * @file logging.h
 * @brief Logging utilities for the COIL assembler
 * 
 * This module provides logging utilities, including log levels,
 * log message formatting, and log output configurations.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include "error_handling.h"

/**
 * @brief Log levels
 */
typedef enum {
  LOG_LEVEL_NONE = 0,          /**< No logging */
  LOG_LEVEL_ERROR = 1,         /**< Error messages only */
  LOG_LEVEL_WARNING = 2,       /**< Warnings and errors */
  LOG_LEVEL_INFO = 3,          /**< Informational messages, warnings, and errors */
  LOG_LEVEL_DEBUG = 4,         /**< Debug messages, informational messages, warnings, and errors */
  LOG_LEVEL_TRACE = 5          /**< Trace messages, debug messages, informational messages, warnings, and errors */
} log_level_t;

/**
 * @brief Log message callback function type
 */
typedef void (*log_callback_t)(
  log_level_t level,
  const char* file,
  int line,
  const char* function,
  const char* message,
  void* user_data
);

/**
 * @brief Initializes the logging system
 *
 * @param[in] verbosity Verbosity level (0-3, maps to log levels)
 * @return Error code indicating success or failure
 */
error_t log_init(int verbosity);

/**
 * @brief Shuts down the logging system
 *
 * @return Error code indicating success or failure
 */
error_t log_shutdown(void);

/**
 * @brief Sets the log level
 *
 * @param[in] level Log level to set
 */
void log_set_level(log_level_t level);

/**
 * @brief Gets the current log level
 *
 * @return Current log level
 */
log_level_t log_get_level(void);

/**
 * @brief Sets a log file
 *
 * @param[in] filename Path to log file, or NULL to disable file logging
 * @return Error code indicating success or failure
 */
error_t log_set_file(const char* filename);

/**
 * @brief Sets a log callback function
 *
 * @param[in] callback Callback function to set
 * @param[in] user_data User data to pass to the callback
 */
void log_set_callback(log_callback_t callback, void* user_data);

/**
 * @brief Enables or disables console logging
 *
 * @param[in] enable Whether to enable console logging
 */
void log_enable_console(bool enable);

/**
 * @brief Logs a message with the specified level
 *
 * @param[in] level Log level
 * @param[in] file Source file where log occurred
 * @param[in] line Line number where log occurred
 * @param[in] function Function where log occurred
 * @param[in] format Printf-style format string
 * @param[in] ... Format arguments
 */
void log_message(
  log_level_t level,
  const char* file,
  int line,
  const char* function,
  const char* format,
  ...
);

/**
 * @brief Logs a message with the specified level using va_list
 *
 * @param[in] level Log level
 * @param[in] file Source file where log occurred
 * @param[in] line Line number where log occurred
 * @param[in] function Function where log occurred
 * @param[in] format Printf-style format string
 * @param[in] args Format arguments as va_list
 */
void log_message_v(
  log_level_t level,
  const char* file,
  int line,
  const char* function,
  const char* format,
  va_list args
);

/**
 * @brief Convenience macro for error logging
 */
#define log_error(format, ...) \
  log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @brief Convenience macro for warning logging
 */
#define log_warning(format, ...) \
  log_message(LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @brief Convenience macro for info logging
 */
#define log_info(format, ...) \
  log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @brief Convenience macro for debug logging
 */
#define log_debug(format, ...) \
  log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @brief Convenience macro for trace logging
 */
#define log_trace(format, ...) \
  log_message(LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#endif /* LOGGING_H */