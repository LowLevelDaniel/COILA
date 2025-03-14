/**
 * @file error_handling.c
 * @brief Error handling utilities
 * @details Implementation of error handling utilities for the COIL assembler.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "coil-assembler/diagnostics.h"

/* Maximum error message length */
#define MAX_ERROR_MSG_LENGTH 1024

/* Thread-local error message buffer */
static __thread char error_buffer[MAX_ERROR_MSG_LENGTH];

/**
 * @brief Error codes
 */
typedef enum {
  COIL_ERROR_NONE = 0,             /**< No error */
  COIL_ERROR_INVALID_ARGUMENT = 1, /**< Invalid argument */
  COIL_ERROR_OUT_OF_MEMORY = 2,    /**< Out of memory */
  COIL_ERROR_FILE_NOT_FOUND = 3,   /**< File not found */
  COIL_ERROR_INVALID_FORMAT = 4,   /**< Invalid format */
  COIL_ERROR_UNSUPPORTED = 5,      /**< Unsupported feature */
  COIL_ERROR_INTERNAL = 6,         /**< Internal error */
  COIL_ERROR_TARGET_SPECIFIC = 7   /**< Target-specific error */
} coil_error_code_t;

/**
 * @brief Set the last error message
 * @param code Error code
 * @param format Format string
 * @param ... Format arguments
 */
void coil_set_error(coil_error_code_t code, const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(error_buffer, MAX_ERROR_MSG_LENGTH, format, args);
  va_end(args);
  
  /* Report error via diagnostics system */
  coil_diagnostic_severity_t severity;
  
  /* Map error code to severity */
  switch (code) {
    case COIL_ERROR_NONE:
      severity = COIL_DIAG_INFO;
      break;
    case COIL_ERROR_INVALID_ARGUMENT:
    case COIL_ERROR_FILE_NOT_FOUND:
    case COIL_ERROR_INVALID_FORMAT:
    case COIL_ERROR_UNSUPPORTED:
      severity = COIL_DIAG_ERROR;
      break;
    case COIL_ERROR_OUT_OF_MEMORY:
    case COIL_ERROR_INTERNAL:
    case COIL_ERROR_TARGET_SPECIFIC:
      severity = COIL_DIAG_FATAL;
      break;
    default:
      severity = COIL_DIAG_ERROR;
      break;
  }
  
  coil_diagnostics_report(NULL, severity, COIL_DIAG_CATEGORY_GENERAL,
                         (uint32_t)code, error_buffer);
}

/**
 * @brief Get the last error message
 * @return Last error message
 */
const char* coil_get_error(void) {
  return error_buffer;
}

/**
 * @brief Clear the last error message
 */
void coil_clear_error(void) {
  error_buffer[0] = '\0';
}

/**
 * @brief Check if an error is set
 * @return true if an error is set, false otherwise
 */
bool coil_has_error(void) {
  return error_buffer[0] != '\0';
}

/**
 * @brief Format an error message with location information
 * @param code Error code
 * @param file Source file
 * @param line Line number
 * @param format Format string
 * @param ... Format arguments
 */
void coil_set_error_at(coil_error_code_t code, const char* file, uint32_t line,
                      const char* format, ...) {
  char message[MAX_ERROR_MSG_LENGTH];
  va_list args;
  va_start(args, format);
  vsnprintf(message, MAX_ERROR_MSG_LENGTH, format, args);
  va_end(args);
  
  /* Format with location information */
  snprintf(error_buffer, MAX_ERROR_MSG_LENGTH, "%s:%u: %s", 
           file, line, message);
  
  /* Report error via diagnostics system */
  coil_diagnostic_severity_t severity;
  
  /* Map error code to severity */
  switch (code) {
    case COIL_ERROR_NONE:
      severity = COIL_DIAG_INFO;
      break;
    case COIL_ERROR_INVALID_ARGUMENT:
    case COIL_ERROR_FILE_NOT_FOUND:
    case COIL_ERROR_INVALID_FORMAT:
    case COIL_ERROR_UNSUPPORTED:
      severity = COIL_DIAG_ERROR;
      break;
    case COIL_ERROR_OUT_OF_MEMORY:
    case COIL_ERROR_INTERNAL:
    case COIL_ERROR_TARGET_SPECIFIC:
      severity = COIL_DIAG_FATAL;
      break;
    default:
      severity = COIL_DIAG_ERROR;
      break;
  }
  
  coil_diagnostics_report_at(NULL, severity, COIL_DIAG_CATEGORY_GENERAL,
                            (uint32_t)code, message, file, line, 0);
}

/**
 * @brief Handle an error and exit if fatal
 * @param code Error code
 * @param context Context string
 * @param fatal Whether the error is fatal
 * @return true if the error was fatal, false otherwise
 */
bool coil_handle_error(coil_error_code_t code, const char* context, bool fatal) {
  if (code == COIL_ERROR_NONE) {
    return false;
  }
  
  /* Append context to error message if provided */
  if (context != NULL && context[0] != '\0') {
    char temp[MAX_ERROR_MSG_LENGTH];
    snprintf(temp, MAX_ERROR_MSG_LENGTH, "%s: %s", context, error_buffer);
    strncpy(error_buffer, temp, MAX_ERROR_MSG_LENGTH);
    error_buffer[MAX_ERROR_MSG_LENGTH - 1] = '\0';
  }
  
  /* Exit if the error is fatal */
  if (fatal) {
    coil_diagnostics_report(NULL, COIL_DIAG_FATAL, COIL_DIAG_CATEGORY_GENERAL,
                           (uint32_t)code, error_buffer);
    exit(EXIT_FAILURE);
    return true; /* Never reached */
  }
  
  return false;
}

/**
 * @brief Format error message with an error code
 * @param code Error code
 * @return Error message string
 */
const char* coil_error_to_string(coil_error_code_t code) {
  switch (code) {
    case COIL_ERROR_NONE:
      return "No error";
    case COIL_ERROR_INVALID_ARGUMENT:
      return "Invalid argument";
    case COIL_ERROR_OUT_OF_MEMORY:
      return "Out of memory";
    case COIL_ERROR_FILE_NOT_FOUND:
      return "File not found";
    case COIL_ERROR_INVALID_FORMAT:
      return "Invalid format";
    case COIL_ERROR_UNSUPPORTED:
      return "Unsupported feature";
    case COIL_ERROR_INTERNAL:
      return "Internal error";
    case COIL_ERROR_TARGET_SPECIFIC:
      return "Target-specific error";
    default:
      return "Unknown error";
  }
}