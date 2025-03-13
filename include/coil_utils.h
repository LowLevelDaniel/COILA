/**
 * @file coil_utils.h
 * @brief Utility functions for COIL assembler.
 */

#ifndef COIL_UTILS_H
#define COIL_UTILS_H

#include <stddef.h>

/**
 * @brief Set the log level
 * 
 * @param level Log level (0=none, 1=error, 2=warning, 3=info, 4=debug)
 */
void coil_set_log_level(int level);

/**
 * @brief Log a message at the specified level
 * 
 * @param level Log level (1-4)
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_log(int level, const char* format, ...);

/**
 * @brief Log an error message
 * 
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_error(const char* format, ...);

/**
 * @brief Log a warning message
 * 
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_warning(const char* format, ...);

/**
 * @brief Log an info message
 * 
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_info(const char* format, ...);

/**
 * @brief Log a debug message
 * 
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_debug(const char* format, ...);

/**
 * @brief Dump hex data to a string
 * 
 * @param data Data buffer
 * @param size Size of data
 * @param output Output string buffer
 * @param output_size Size of output buffer
 * @return Number of bytes written to output
 */
size_t coil_hex_dump(const void* data, size_t size, char* output, size_t output_size);

/**
 * @brief Print a memory dump of a buffer
 * 
 * @param data Data buffer
 * @param size Size of data
 */
void coil_print_hex_dump(const void* data, size_t size);

#endif /* COIL_UTILS_H */