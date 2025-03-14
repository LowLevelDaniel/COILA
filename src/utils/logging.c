/**
 * @file logging.c
 * @brief Logging utilities
 * @details Implementation of logging utilities for the COIL assembler.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "coil-assembler/diagnostics.h"

/* Maximum log message length */
#define MAX_LOG_MSG_LENGTH 2048

/* Log level */
typedef enum {
  COIL_LOG_LEVEL_TRACE = 0,
  COIL_LOG_LEVEL_DEBUG = 1,
  COIL_LOG_LEVEL_INFO = 2,
  COIL_LOG_LEVEL_WARNING = 3,
  COIL_LOG_LEVEL_ERROR = 4,
  COIL_LOG_LEVEL_FATAL = 5,
  COIL_LOG_LEVEL_NONE = 6
} coil_log_level_t;

/* Log destination */
typedef enum {
  COIL_LOG_DEST_STDOUT = 0,  /* Standard output */
  COIL_LOG_DEST_STDERR = 1,  /* Standard error */
  COIL_LOG_DEST_FILE = 2,    /* Log file */
  COIL_LOG_DEST_CALLBACK = 3 /* Custom callback */
} coil_log_destination_t;

/* Log configuration */
typedef struct {
  coil_log_level_t level;        /* Current log level */
  coil_log_destination_t dest;   /* Log destination */
  FILE* log_file;                /* Log file handle (if dest is FILE) */
  void (*callback)(const char*); /* Log callback (if dest is CALLBACK) */
  int include_timestamp;         /* Whether to include timestamp */
  int include_level;             /* Whether to include level */
  char* prefix;                  /* Log message prefix */
} coil_log_config_t;

/* Global log configuration */
static coil_log_config_t log_config = {
  .level = COIL_LOG_LEVEL_INFO,
  .dest = COIL_LOG_DEST_STDERR,
  .log_file = NULL,
  .callback = NULL,
  .include_timestamp = 1,
  .include_level = 1,
  .prefix = NULL
};

/* Level names */
static const char* level_names[] = {
  "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};

/**
 * @brief Set the log level
 * @param level The new log level
 */
void coil_log_set_level(coil_log_level_t level) {
  log_config.level = level;
}

/**
 * @brief Get the current log level
 * @return Current log level
 */
coil_log_level_t coil_log_get_level(void) {
  return log_config.level;
}

/**
 * @brief Set the log destination to standard output
 */
void coil_log_to_stdout(void) {
  if (log_config.dest == COIL_LOG_DEST_FILE && log_config.log_file) {
    fclose(log_config.log_file);
    log_config.log_file = NULL;
  }
  log_config.dest = COIL_LOG_DEST_STDOUT;
}

/**
 * @brief Set the log destination to standard error
 */
void coil_log_to_stderr(void) {
  if (log_config.dest == COIL_LOG_DEST_FILE && log_config.log_file) {
    fclose(log_config.log_file);
    log_config.log_file = NULL;
  }
  log_config.dest = COIL_LOG_DEST_STDERR;
}

/**
 * @brief Set the log destination to a file
 * @param filename Path to the log file
 * @return 0 on success, non-zero on failure
 */
int coil_log_to_file(const char* filename) {
  if (!filename) {
    return -1;
  }
  
  /* Close previous log file if any */
  if (log_config.dest == COIL_LOG_DEST_FILE && log_config.log_file) {
    fclose(log_config.log_file);
    log_config.log_file = NULL;
  }
  
  /* Open new log file */
  FILE* file = fopen(filename, "a");
  if (!file) {
    return -1;
  }
  
  log_config.dest = COIL_LOG_DEST_FILE;
  log_config.log_file = file;
  
  return 0;
}

/**
 * @brief Set the log destination to a callback function
 * @param callback Function to call with log messages
 * @return 0 on success, non-zero on failure
 */
int coil_log_to_callback(void (*callback)(const char*)) {
  if (!callback) {
    return -1;
  }
  
  /* Close previous log file if any */
  if (log_config.dest == COIL_LOG_DEST_FILE && log_config.log_file) {
    fclose(log_config.log_file);
    log_config.log_file = NULL;
  }
  
  log_config.dest = COIL_LOG_DEST_CALLBACK;
  log_config.callback = callback;
  
  return 0;
}

/**
 * @brief Set whether to include timestamp in log messages
 * @param include_timestamp Non-zero to include timestamp
 */
void coil_log_include_timestamp(int include_timestamp) {
  log_config.include_timestamp = include_timestamp;
}

/**
 * @brief Set whether to include level in log messages
 * @param include_level Non-zero to include level
 */
void coil_log_include_level(int include_level) {
  log_config.include_level = include_level;
}

/**
 * @brief Set a prefix for log messages
 * @param prefix Prefix string
 */
void coil_log_set_prefix(const char* prefix) {
  /* Free previous prefix if any */
  if (log_config.prefix) {
    free(log_config.prefix);
    log_config.prefix = NULL;
  }
  
  /* Set new prefix */
  if (prefix) {
    log_config.prefix = strdup(prefix);
  }
}

/**
 * @brief Format and output a log message
 * @param level Log level
 * @param fmt Format string
 * @param ... Format arguments
 * @return 0 on success, non-zero on failure
 */
int coil_log_message(coil_log_level_t level, const char* fmt, ...) {
  /* Check if message should be logged */
  if (level < log_config.level || level >= COIL_LOG_LEVEL_NONE) {
    return 0;
  }
  
  /* Format message */
  char message[MAX_LOG_MSG_LENGTH];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, MAX_LOG_MSG_LENGTH, fmt, args);
  va_end(args);
  
  /* Construct final log entry */
  char log_entry[MAX_LOG_MSG_LENGTH];
  size_t offset = 0;
  
  /* Add timestamp if enabled */
  if (log_config.include_timestamp) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    offset += snprintf(log_entry + offset, MAX_LOG_MSG_LENGTH - offset, 
                      "[%s] ", timestamp);
  }
  
  /* Add level if enabled */
  if (log_config.include_level && level < COIL_LOG_LEVEL_NONE) {
    offset += snprintf(log_entry + offset, MAX_LOG_MSG_LENGTH - offset, 
                      "[%s] ", level_names[level]);
  }
  
  /* Add prefix if any */
  if (log_config.prefix) {
    offset += snprintf(log_entry + offset, MAX_LOG_MSG_LENGTH - offset, 
                      "%s ", log_config.prefix);
  }
  
  /* Add message */
  snprintf(log_entry + offset, MAX_LOG_MSG_LENGTH - offset, "%s", message);
  
  /* Output log entry */
  switch (log_config.dest) {
    case COIL_LOG_DEST_STDOUT:
      fprintf(stdout, "%s\n", log_entry);
      fflush(stdout);
      break;
      
    case COIL_LOG_DEST_STDERR:
      fprintf(stderr, "%s\n", log_entry);
      fflush(stderr);
      break;
      
    case COIL_LOG_DEST_FILE:
      if (log_config.log_file) {
        fprintf(log_config.log_file, "%s\n", log_entry);
        fflush(log_config.log_file);
      }
      break;
      
    case COIL_LOG_DEST_CALLBACK:
      if (log_config.callback) {
        log_config.callback(log_entry);
      }
      break;
      
    default:
      return -1;
  }
  
  /* Convert to diagnostic if needed */
  coil_diagnostic_severity_t severity;
  switch (level) {
    case COIL_LOG_LEVEL_TRACE:
    case COIL_LOG_LEVEL_DEBUG:
    case COIL_LOG_LEVEL_INFO:
      severity = COIL_DIAG_INFO;
      break;
    case COIL_LOG_LEVEL_WARNING:
      severity = COIL_DIAG_WARNING;
      break;
    case COIL_LOG_LEVEL_ERROR:
      severity = COIL_DIAG_ERROR;
      break;
    case COIL_LOG_LEVEL_FATAL:
      severity = COIL_DIAG_FATAL;
      break;
    default:
      severity = COIL_DIAG_INFO;
      break;
  }
  
  /* Report to diagnostic system */
  coil_diagnostics_report(NULL, severity, COIL_DIAG_CATEGORY_GENERAL, 
                        (uint32_t)level, message);
  
  return 0;
}

/**
 * @brief Log a message with TRACE level
 * @param fmt Format string
 * @param ... Format arguments
 */
void coil_log_trace(const char* fmt, ...) {
  if (COIL_LOG_LEVEL_TRACE < log_config.level) {
    return;
  }
  
  char message[MAX_LOG_MSG_LENGTH];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, MAX_LOG_MSG_LENGTH, fmt, args);
  va_end(args);
  
  coil_log_message(COIL_LOG_LEVEL_TRACE, "%s", message);
}

/**
 * @brief Log a message with DEBUG level
 * @param fmt Format string
 * @param ... Format arguments
 */
void coil_log_debug(const char* fmt, ...) {
  if (COIL_LOG_LEVEL_DEBUG < log_config.level) {
    return;
  }
  
  char message[MAX_LOG_MSG_LENGTH];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, MAX_LOG_MSG_LENGTH, fmt, args);
  va_end(args);
  
  coil_log_message(COIL_LOG_LEVEL_DEBUG, "%s", message);
}

/**
 * @brief Log a message with INFO level
 * @param fmt Format string
 * @param ... Format arguments
 */
void coil_log_info(const char* fmt, ...) {
  if (COIL_LOG_LEVEL_INFO < log_config.level) {
    return;
  }
  
  char message[MAX_LOG_MSG_LENGTH];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, MAX_LOG_MSG_LENGTH, fmt, args);
  va_end(args);
  
  coil_log_message(COIL_LOG_LEVEL_INFO, "%s", message);
}

/**
 * @brief Log a message with WARNING level
 * @param fmt Format string
 * @param ... Format arguments
 */
void coil_log_warning(const char* fmt, ...) {
  if (COIL_LOG_LEVEL_WARNING < log_config.level) {
    return;
  }
  
  char message[MAX_LOG_MSG_LENGTH];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, MAX_LOG_MSG_LENGTH, fmt, args);
  va_end(args);
  
  coil_log_message(COIL_LOG_LEVEL_WARNING, "%s", message);
}

/**
 * @brief Log a message with ERROR level
 * @param fmt Format string
 * @param ... Format arguments
 */
void coil_log_error(const char* fmt, ...) {
  if (COIL_LOG_LEVEL_ERROR < log_config.level) {
    return;
  }
  
  char message[MAX_LOG_MSG_LENGTH];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, MAX_LOG_MSG_LENGTH, fmt, args);
  va_end(args);
  
  coil_log_message(COIL_LOG_LEVEL_ERROR, "%s", message);
}

/**
 * @brief Log a message with FATAL level
 * @param fmt Format string
 * @param ... Format arguments
 */
void coil_log_fatal(const char* fmt, ...) {
  if (COIL_LOG_LEVEL_FATAL < log_config.level) {
    return;
  }
  
  char message[MAX_LOG_MSG_LENGTH];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, MAX_LOG_MSG_LENGTH, fmt, args);
  va_end(args);
  
  coil_log_message(COIL_LOG_LEVEL_FATAL, "%s", message);
}

/**
 * @brief Clean up logging resources
 */
void coil_log_cleanup(void) {
  /* Close log file if open */
  if (log_config.dest == COIL_LOG_DEST_FILE && log_config.log_file) {
    fclose(log_config.log_file);
    log_config.log_file = NULL;
  }
  
  /* Free prefix if any */
  if (log_config.prefix) {
    free(log_config.prefix);
    log_config.prefix = NULL;
  }
}

/**
 * @brief Initialize logging with default configuration
 * @return 0 on success, non-zero on failure
 */
int coil_log_init(void) {
  /* Set default configuration */
  log_config.level = COIL_LOG_LEVEL_INFO;
  log_config.dest = COIL_LOG_DEST_STDERR;
  log_config.log_file = NULL;
  log_config.callback = NULL;
  log_config.include_timestamp = 1;
  log_config.include_level = 1;
  log_config.prefix = NULL;
  
  /* Log initialization */
  coil_log_debug("Logging initialized");
  
  return 0;
}

/**
 * @brief Create a snapshot of the current log configuration
 * @return Copy of current log configuration
 */
coil_log_config_t coil_log_get_config(void) {
  return log_config;
}

/**
 * @brief Restore a previously saved log configuration
 * @param config Log configuration to restore
 * @return 0 on success, non-zero on failure
 */
int coil_log_set_config(coil_log_config_t config) {
  /* Clean up current configuration */
  coil_log_cleanup();
  
  /* Copy configuration */
  log_config = config;
  
  /* Make a new copy of prefix if any */
  if (config.prefix) {
    log_config.prefix = strdup(config.prefix);
    if (!log_config.prefix) {
      return -1;
    }
  }
  
  /* Open log file if needed */
  if (config.dest == COIL_LOG_DEST_FILE && config.log_file) {
    /* We can't copy FILE pointers, so this is a limitation */
    coil_log_error("Cannot copy FILE pointer in log configuration");
    log_config.log_file = NULL;
    return -1;
  }
  
  return 0;
}