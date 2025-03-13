/**
 * @file error_handling.h
 * @brief Error handling utilities for the COIL assembler
 * 
 * This module provides error handling utilities, including error codes,
 * error messages, and error reporting functions.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Error codes used throughout the COIL assembler
 */
typedef enum {
  ERROR_NONE = 0,              /**< No error */
  ERROR_INVALID_ARGUMENT,      /**< Invalid argument */
  ERROR_MEMORY,                /**< Memory allocation failure */
  ERROR_FILE_IO,               /**< File I/O error */
  ERROR_INVALID_FORMAT,        /**< Invalid format in input */
  ERROR_NOT_FOUND,             /**< Item not found */
  ERROR_UNSUPPORTED,           /**< Unsupported operation or feature */
  ERROR_CONFIG,                /**< Configuration error */
  ERROR_TRANSLATION,           /**< Translation error */
  ERROR_OPTIMIZATION,          /**< Optimization error */
  ERROR_GENERATION,            /**< Code generation error */
  ERROR_INTERNAL               /**< Internal error */
} error_t;

/**
 * @brief Error context containing detailed error information
 */
typedef struct {
  error_t code;                /**< Error code */
  const char* message;         /**< Error message */
  const char* file;            /**< Source file where error occurred */
  int line;                    /**< Line number where error occurred */
  const char* function;        /**< Function where error occurred */
} error_context_t;

/**
 * @brief Error callback function type
 */
typedef void (*error_callback_t)(const error_context_t* context, void* user_data);

/**
 * @brief Gets a string description for an error code
 *
 * @param[in] error Error code
 * @return Null-terminated string describing the error
 */
const char* error_message(error_t error);

/**
 * @brief Sets an error callback function
 *
 * The callback will be called whenever an error is reported.
 *
 * @param[in] callback Callback function to set
 * @param[in] user_data User data to pass to the callback
 */
void error_set_callback(error_callback_t callback, void* user_data);

/**
 * @brief Reports an error with context information
 *
 * @param[in] code Error code
 * @param[in] message Error message
 * @param[in] file Source file where error occurred
 * @param[in] line Line number where error occurred
 * @param[in] function Function where error occurred
 */
void error_report(
  error_t code,
  const char* message,
  const char* file,
  int line,
  const char* function
);

/**
 * @brief Gets the last error reported
 *
 * @param[out] context Pointer to receive error context
 * @return true if an error was retrieved, false otherwise
 */
bool error_get_last(error_context_t* context);

/**
 * @brief Clears the last error
 */
void error_clear(void);

/**
 * @brief Convenience macro for reporting errors
 */
#define ERROR_REPORT(code, message) \
  error_report(code, message, __FILE__, __LINE__, __func__)

#endif /* ERROR_HANDLING_H */