/**
 * @file diagnostics.h
 * @brief Error and warning reporting
 * @details This file defines the diagnostics system for the COIL assembler,
 *          providing error and warning reporting capabilities.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_DIAGNOSTICS_H
#define COIL_DIAGNOSTICS_H

#include <stdint.h>

/**
 * @brief Diagnostic severity levels
 */
typedef enum {
  COIL_DIAG_INFO = 0,     /**< Informational message */
  COIL_DIAG_WARNING = 1,  /**< Warning message */
  COIL_DIAG_ERROR = 2,    /**< Error message */
  COIL_DIAG_FATAL = 3     /**< Fatal error message */
} coil_diagnostic_severity_t;

/**
 * @brief Diagnostic categories
 */
typedef enum {
  COIL_DIAG_CATEGORY_GENERAL = 0,  /**< General diagnostic */
  COIL_DIAG_CATEGORY_PARSER = 1,   /**< Parser diagnostic */
  COIL_DIAG_CATEGORY_TYPE = 2,     /**< Type system diagnostic */
  COIL_DIAG_CATEGORY_INSTRUCTION = 3, /**< Instruction diagnostic */
  COIL_DIAG_CATEGORY_TARGET = 4,   /**< Target-specific diagnostic */
  COIL_DIAG_CATEGORY_OPTIMIZER = 5, /**< Optimizer diagnostic */
  COIL_DIAG_CATEGORY_GENERATOR = 6 /**< Code generator diagnostic */
} coil_diagnostic_category_t;

/**
 * @brief Diagnostic message structure
 */
typedef struct {
  coil_diagnostic_severity_t severity; /**< Severity level */
  coil_diagnostic_category_t category; /**< Diagnostic category */
  uint32_t code;           /**< Diagnostic code */
  const char *message;     /**< Diagnostic message */
  const char *file;        /**< Source file (or NULL) */
  uint32_t line;           /**< Line number (or 0) */
  uint32_t column;         /**< Column number (or 0) */
  void *context;           /**< Context pointer */
} coil_diagnostic_t;

/**
 * @brief Diagnostic handler function type
 * @param diagnostic Diagnostic message
 * @param user_data User data pointer
 */
typedef void (*coil_diagnostics_handler_t)(const coil_diagnostic_t *diagnostic, 
                                          void *user_data);

/**
 * @brief Forward declaration of diagnostics context
 */
typedef struct coil_diagnostics_context_s coil_diagnostics_context_t;

/**
 * @brief Create a new diagnostics context
 * @return New diagnostics context or NULL on failure
 */
coil_diagnostics_context_t* coil_diagnostics_create(void);

/**
 * @brief Destroy a diagnostics context
 * @param context Diagnostics context to destroy
 */
void coil_diagnostics_destroy(coil_diagnostics_context_t *context);

/**
 * @brief Set a diagnostics handler
 * @param context Diagnostics context
 * @param handler Handler function
 * @param user_data User data to pass to the handler
 * @return 0 on success, non-zero on failure
 */
int coil_diagnostics_set_handler(coil_diagnostics_context_t *context,
                                coil_diagnostics_handler_t handler,
                                void *user_data);

/**
 * @brief Get the diagnostics handler
 * @param context Diagnostics context
 * @param handler Pointer to store the handler function
 * @param user_data Pointer to store the user data
 * @return 0 on success, non-zero on failure
 */
int coil_diagnostics_get_handler(const coil_diagnostics_context_t *context,
                                coil_diagnostics_handler_t *handler,
                                void **user_data);

/**
 * @brief Report a diagnostic message
 * @param context Diagnostics context
 * @param severity Severity level
 * @param category Diagnostic category
 * @param code Diagnostic code
 * @param message Diagnostic message
 * @return 0 on success, non-zero on failure
 */
int coil_diagnostics_report(coil_diagnostics_context_t *context,
                           coil_diagnostic_severity_t severity,
                           coil_diagnostic_category_t category,
                           uint32_t code,
                           const char *message);

/**
 * @brief Report a diagnostic message with source location
 * @param context Diagnostics context
 * @param severity Severity level
 * @param category Diagnostic category
 * @param code Diagnostic code
 * @param message Diagnostic message
 * @param file Source file
 * @param line Line number
 * @param column Column number
 * @return 0 on success, non-zero on failure
 */
int coil_diagnostics_report_at(coil_diagnostics_context_t *context,
                              coil_diagnostic_severity_t severity,
                              coil_diagnostic_category_t category,
                              uint32_t code,
                              const char *message,
                              const char *file,
                              uint32_t line,
                              uint32_t column);

/**
 * @brief Get the number of diagnostic messages
 * @param context Diagnostics context
 * @param severity Severity level (or -1 for all)
 * @return Number of messages
 */
uint32_t coil_diagnostics_get_count(const coil_diagnostics_context_t *context,
                                   int severity);

/**
 * @brief Get a diagnostic message by index
 * @param context Diagnostics context
 * @param index Message index
 * @return Diagnostic message or NULL if out of bounds
 */
const coil_diagnostic_t* coil_diagnostics_get_message(const coil_diagnostics_context_t *context,
                                                    uint32_t index);

/**
 * @brief Clear all diagnostic messages
 * @param context Diagnostics context
 */
void coil_diagnostics_clear(coil_diagnostics_context_t *context);

/**
 * @brief Default diagnostics handler that prints to stderr
 * @param diagnostic Diagnostic message
 * @param user_data User data pointer (unused)
 */
void coil_diagnostics_default_handler(const coil_diagnostic_t *diagnostic, void *user_data);

/**
 * @brief Create a formatted diagnostic message
 * @param context Diagnostics context
 * @param severity Severity level
 * @param category Diagnostic category
 * @param code Diagnostic code
 * @param format Format string
 * @param ... Format arguments
 * @return 0 on success, non-zero on failure
 */
int coil_diagnostics_reportf(coil_diagnostics_context_t *context,
                            coil_diagnostic_severity_t severity,
                            coil_diagnostic_category_t category,
                            uint32_t code,
                            const char *format, ...);

#endif /* COIL_DIAGNOSTICS_H */