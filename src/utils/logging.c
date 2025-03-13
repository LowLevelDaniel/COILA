/**
 * @file logging.c
 * @brief Implementation of logging utilities for the COIL assembler
 * 
 * This module implements logging utilities, including log levels,
 * log message formatting, and log output configurations.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "logging.h"

/**
 * @brief Maximum log message length
 */
#define MAX_LOG_LENGTH 1024

/**
 * @brief String representations of log levels
 */
static const char* level_strings[] = {
  "NONE",    /* LOG_LEVEL_NONE */
  "ERROR",   /* LOG_LEVEL_ERROR */
  "WARNING", /* LOG_LEVEL_WARNING */
  "INFO",    /* LOG_LEVEL_INFO */
  "DEBUG",   /* LOG_LEVEL_DEBUG */
  "TRACE"    /* LOG_LEVEL_TRACE */
};

/**
 * @brief Current logging state
 */
static struct {
  log_level_t level;          /* Current log level */
  FILE* log_file;             /* Log file, if any */
  bool console_enabled;       /* Whether console logging is enabled */
  log_callback_t callback;    /* User-provided callback function */
  void* user_data;            /* User data for callback */
  bool initialized;           /* Whether logging is initialized */
} log_state = {
  LOG_LEVEL_NONE,  /* level */
  NULL,            /* log_file */
  true,            /* console_enabled */
  NULL,            /* callback */
  NULL,            /* user_data */
  false            /* initialized */
};

error_t log_init(int verbosity) {
  if (log_state.initialized) {
    return ERROR_NONE;  /* Already initialized */
  }
  
  /* Map verbosity (0-3) to log level */
  switch (verbosity) {
    case 0:
      log_state.level = LOG_LEVEL_ERROR;
      break;
    case 1:
      log_state.level = LOG_LEVEL_WARNING;
      break;
    case 2:
      log_state.level = LOG_LEVEL_INFO;
      break;
    case 3:
    default:
      log_state.level = LOG_LEVEL_DEBUG;
      break;
  }
  
  log_state.log_file = NULL;
  log_state.console_enabled = true;
  log_state.callback = NULL;
  log_state.user_data = NULL;
  log_state.initialized = true;
  
  return ERROR_NONE;
}

error_t log_shutdown(void) {
  if (!log_state.initialized) {
    return ERROR_NONE;  /* Not initialized */
  }
  
  /* Close log file if open */
  if (log_state.log_file != NULL) {
    fclose(log_state.log_file);
    log_state.log_file = NULL;
  }
  
  log_state.level = LOG_LEVEL_NONE;
  log_state.console_enabled = false;
  log_state.callback = NULL;
  log_state.user_data = NULL;
  log_state.initialized = false;
  
  return ERROR_NONE;
}

void log_set_level(log_level_t level) {
  log_state.level = level;
}

log_level_t log_get_level(void) {
  return log_state.level;
}

error_t log_set_file(const char* filename) {
  /* Close current log file if open */
  if (log_state.log_file != NULL) {
    fclose(log_state.log_file);
    log_state.log_file = NULL;
  }
  
  /* If filename is NULL, just disable file logging */
  if (filename == NULL) {
    return ERROR_NONE;
  }
  
  /* Open new log file */
  log_state.log_file = fopen(filename, "a");
  if (log_state.log_file == NULL) {
    return ERROR_FILE_IO;
  }
  
  /* Use line buffering for log file */
  setvbuf(log_state.log_file, NULL, _IOLBF, 0);
  
  return ERROR_NONE;
}

void log_set_callback(log_callback_t callback, void* user_data) {
  log_state.callback = callback;
  log_state.user_data = user_data;
}

void log_enable_console(bool enable) {
  log_state.console_enabled = enable;
}

/**
 * @brief Gets the current timestamp as a string
 *
 * @param[out] buffer Buffer to receive timestamp string
 * @param[in] size Size of buffer
 */
static void get_timestamp(char* buffer, size_t size) {
  time_t now;
  struct tm* time_info;
  
  time(&now);
  time_info = localtime(&now);
  
  strftime(buffer, size, "%Y-%m-%d %H:%M:%S", time_info);
}

void log_message_v(
  log_level_t level,
  const char* file,
  int line,
  const char* function,
  const char* format,
  va_list args
) {
  char message[MAX_LOG_LENGTH];
  char timestamp[20];
  
  /* Check if logging is initialized and level is enabled */
  if (!log_state.initialized || level > log_state.level || level == LOG_LEVEL_NONE) {
    return;
  }
  
  /* Format the log message */
  vsnprintf(message, sizeof(message), format, args);
  
  /* Get current timestamp */
  get_timestamp(timestamp, sizeof(timestamp));
  
  /* Call user callback if set */
  if (log_state.callback != NULL) {
    log_state.callback(level, file, line, function, message, log_state.user_data);
  }
  
  /* Format for output: timestamp, level, file:line, message */
  char full_message[MAX_LOG_LENGTH * 2];
  snprintf(
    full_message, 
    sizeof(full_message), 
    "[%s] [%s] %s:%d - %s\n",
    timestamp,
    level_strings[level],
    file,
    line,
    message
  );
  
  /* Write to console if enabled */
  if (log_state.console_enabled) {
    FILE* output = (level <= LOG_LEVEL_ERROR) ? stderr : stdout;
    fputs(full_message, output);
    fflush(output);
  }
  
  /* Write to log file if enabled */
  if (log_state.log_file != NULL) {
    fputs(full_message, log_state.log_file);
    fflush(log_state.log_file);
  }
}

void log_message(
  log_level_t level,
  const char* file,
  int line,
  const char* function,
  const char* format,
  ...
) {
  va_list args;
  
  va_start(args, format);
  log_message_v(level, file, line, function, format, args);
  va_end(args);
}