/**
 * @file coil_utils.c
 * @brief Utility functions for COIL assembler.
 */

#include "coil_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/**
 * @brief Global log level
 */
static int g_log_level = 1;  // 0=none, 1=error, 2=warning, 3=info, 4=debug

/**
 * @brief Set the log level
 * 
 * @param level Log level (0-4)
 */
void coil_set_log_level(int level) {
  if (level >= 0 && level <= 4) {
    g_log_level = level;
  }
}

/**
 * @brief Log a message at the specified level
 * 
 * @param level Log level (1-4)
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_log(int level, const char* format, ...) {
  if (level > g_log_level) {
    return;
  }
  
  const char* prefix = "";
  FILE* output = stdout;
  
  switch (level) {
    case 1:  // Error
      prefix = "[ERROR] ";
      output = stderr;
      break;
    case 2:  // Warning
      prefix = "[WARNING] ";
      break;
    case 3:  // Info
      prefix = "[INFO] ";
      break;
    case 4:  // Debug
      prefix = "[DEBUG] ";
      break;
    default:
      return;  // Invalid level
  }
  
  // Get current time
  time_t now = time(NULL);
  struct tm* tm_info = localtime(&now);
  char time_str[20];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
  
  // Print timestamp and prefix
  fprintf(output, "%s %s", time_str, prefix);
  
  // Print formatted message
  va_list args;
  va_start(args, format);
  vfprintf(output, format, args);
  va_end(args);
  
  // Add newline if not already present
  if (format[0] != '\0' && format[strlen(format) - 1] != '\n') {
    fprintf(output, "\n");
  }
}

/**
 * @brief Log an error message
 * 
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_error(const char* format, ...) {
  if (g_log_level < 1) {
    return;
  }
  
  fprintf(stderr, "[ERROR] ");
  
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  
  // Add newline if not already present
  if (format[0] != '\0' && format[strlen(format) - 1] != '\n') {
    fprintf(stderr, "\n");
  }
}

/**
 * @brief Log a warning message
 * 
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_warning(const char* format, ...) {
  if (g_log_level < 2) {
    return;
  }
  
  fprintf(stdout, "[WARNING] ");
  
  va_list args;
  va_start(args, format);
  vfprintf(stdout, format, args);
  va_end(args);
  
  // Add newline if not already present
  if (format[0] != '\0' && format[strlen(format) - 1] != '\n') {
    fprintf(stdout, "\n");
  }
}

/**
 * @brief Log an info message
 * 
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_info(const char* format, ...) {
  if (g_log_level < 3) {
    return;
  }
  
  fprintf(stdout, "[INFO] ");
  
  va_list args;
  va_start(args, format);
  vfprintf(stdout, format, args);
  va_end(args);
  
  // Add newline if not already present
  if (format[0] != '\0' && format[strlen(format) - 1] != '\n') {
    fprintf(stdout, "\n");
  }
}

/**
 * @brief Log a debug message
 * 
 * @param format Format string
 * @param ... Variable arguments
 */
void coil_debug(const char* format, ...) {
  if (g_log_level < 4) {
    return;
  }
  
  fprintf(stdout, "[DEBUG] ");
  
  va_list args;
  va_start(args, format);
  vfprintf(stdout, format, args);
  va_end(args);
  
  // Add newline if not already present
  if (format[0] != '\0' && format[strlen(format) - 1] != '\n') {
    fprintf(stdout, "\n");
  }
}

/**
 * @brief Dump hex data to a string
 * 
 * @param data Data buffer
 * @param size Size of data
 * @param output Output string buffer
 * @param output_size Size of output buffer
 * @return Number of bytes written to output
 */
size_t coil_hex_dump(const void* data, size_t size, char* output, size_t output_size) {
  const uint8_t* buf = (const uint8_t*)data;
  size_t offset = 0;
  size_t output_offset = 0;
  
  while (offset < size && output_offset + 80 < output_size) {
    // Print offset
    output_offset += snprintf(output + output_offset, output_size - output_offset,
                            "%08zx: ", offset);
    
    // Print hex bytes
    for (size_t i = 0; i < 16; i++) {
      if (offset + i < size) {
        output_offset += snprintf(output + output_offset, output_size - output_offset,
                                "%02x ", buf[offset + i]);
      } else {
        output_offset += snprintf(output + output_offset, output_size - output_offset,
                                "   ");
      }
      
      if (i == 7) {
        output_offset += snprintf(output + output_offset, output_size - output_offset, " ");
      }
    }
    
    // Print ASCII representation
    output_offset += snprintf(output + output_offset, output_size - output_offset, " |");
    
    for (size_t i = 0; i < 16; i++) {
      if (offset + i < size) {
        char c = buf[offset + i];
        if (c >= 32 && c <= 126) {
          output_offset += snprintf(output + output_offset, output_size - output_offset, "%c", c);
        } else {
          output_offset += snprintf(output + output_offset, output_size - output_offset, ".");
        }
      } else {
        output_offset += snprintf(output + output_offset, output_size - output_offset, " ");
      }
    }
    
    output_offset += snprintf(output + output_offset, output_size - output_offset, "|\n");
    
    offset += 16;
  }
  
  return output_offset;
}

/**
 * @brief Print a memory dump of a buffer
 * 
 * @param data Data buffer
 * @param size Size of data
 */
void coil_print_hex_dump(const void* data, size_t size) {
  if (g_log_level < 4) {
    return;  // Only print in debug mode
  }
  
  const uint8_t* buf = (const uint8_t*)data;
  size_t offset = 0;
  
  while (offset < size) {
    // Print offset
    printf("%08zx: ", offset);
    
    // Print hex bytes
    for (size_t i = 0; i < 16; i++) {
      if (offset + i < size) {
        printf("%02x ", buf[offset + i]);
      } else {
        printf("   ");
      }
      
      if (i == 7) {
        printf(" ");
      }
    }
    
    // Print ASCII representation
    printf(" |");
    
    for (size_t i = 0; i < 16; i++) {
      if (offset + i < size) {
        char c = buf[offset + i];
        if (c >= 32 && c <= 126) {
          printf("%c", c);
        } else {
          printf(".");
        }
      } else {
        printf(" ");
      }
    }
    
    printf("|\n");
    
    offset += 16;
  }
}