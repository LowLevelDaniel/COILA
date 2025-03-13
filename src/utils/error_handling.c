/**
 * @file error_handling.c
 * @brief Implementation of error handling utilities for the COIL assembler
 * 
 * This module implements error handling utilities, including error codes,
 * error messages, and error reporting functions.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "error_handling.h"

/**
 * @brief Static error messages for error codes
 */
static const char* error_messages[] = {
  "No error",                       /* ERROR_NONE */
  "Invalid argument",               /* ERROR_INVALID_ARGUMENT */
  "Memory allocation failure",      /* ERROR_MEMORY */
  "File I/O error",                 /* ERROR_FILE_IO */
  "Invalid format in input",        /* ERROR_INVALID_FORMAT */
  "Item not found",                 /* ERROR_NOT_FOUND */
  "Unsupported operation or feature", /* ERROR_UNSUPPORTED */
  "Configuration error",            /* ERROR_CONFIG */
  "Translation error",              /* ERROR_TRANSLATION */
  "Optimization error",             /* ERROR_OPTIMIZATION */
  "Code generation error",          /* ERROR_GENERATION */
  "Internal error"                  /* ERROR_INTERNAL */
};

/** 
 * @brief Last reported error context
 */
static error_context_t last_error = {
  ERROR_NONE,  /* code */
  NULL,        /* message */
  NULL,        /* file */
  0,           /* line */
  NULL         /* function */
};

/**
 * @brief User-provided error callback and data
 */
static struct {
  error_callback_t callback;
  void* user_data;
} error_handler = {
  NULL,  /* callback */
  NULL   /* user_data */
};

const char* error_message(error_t error) {
  if (error < 0 || error >= sizeof(error_messages) / sizeof(error_messages[0])) {
    return "Unknown error";
  }
  
  return error_messages[error];
}

void error_set_callback(error_callback_t callback, void* user_data) {
  error_handler.callback = callback;
  error_handler.user_data = user_data;
}

void error_report(
  error_t code,
  const char* message,
  const char* file,
  int line,
  const char* function
) {
  /* First, update the last error context */
  last_error.code = code;
  last_error.message = message;
  last_error.file = file;
  last_error.line = line;
  last_error.function = function;
  
  /* Then, call the user-provided callback if set */
  if (error_handler.callback != NULL) {
    error_handler.callback(&last_error, error_handler.user_data);
  }
}

bool error_get_last(error_context_t* context) {
  if (context == NULL) {
    return false;
  }
  
  if (last_error.code == ERROR_NONE) {
    return false;
  }
  
  /* Copy the last error to the provided context */
  context->code = last_error.code;
  context->message = last_error.message;
  context->file = last_error.file;
  context->line = last_error.line;
  context->function = last_error.function;
  
  return true;
}

void error_clear(void) {
  last_error.code = ERROR_NONE;
  last_error.message = NULL;
  last_error.file = NULL;
  last_error.line = 0;
  last_error.function = NULL;
}