/**
 * @file assembler.h
 * @brief Main assembler interface
 * @details This file defines the main interface for the COIL assembler, which
 *          translates COIL binary code into native code for specific targets.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_ASSEMBLER_H
#define COIL_ASSEMBLER_H

#include "../coil/binary.h"
#include "config.h"
#include "target.h"
#include "diagnostics.h"

/**
 * @brief Optimization levels
 */
typedef enum {
  COIL_OPT_LEVEL_0 = 0, /**< No optimization */
  COIL_OPT_LEVEL_1 = 1, /**< Basic optimizations */
  COIL_OPT_LEVEL_2 = 2, /**< More aggressive optimizations */
  COIL_OPT_LEVEL_3 = 3, /**< Most aggressive optimizations */
  COIL_OPT_LEVEL_S = 4  /**< Optimize for size */
} coil_optimization_level_t;

/**
 * @brief Output format types
 */
typedef enum {
  COIL_OUTPUT_FORMAT_OBJECT = 0, /**< Native object file */
  COIL_OUTPUT_FORMAT_ASSEMBLY = 1, /**< Assembly source file */
  COIL_OUTPUT_FORMAT_EXECUTABLE = 2, /**< Executable file */
  COIL_OUTPUT_FORMAT_LIBRARY = 3 /**< Library file */
} coil_output_format_t;

/**
 * @brief Forward declaration of assembler structure
 */
typedef struct coil_assembler_s coil_assembler_t;

/**
 * @brief Creates a new assembler instance
 * @return Pointer to a new assembler instance or NULL on failure
 */
coil_assembler_t* coil_assembler_create(void);

/**
 * @brief Destroys an assembler instance and frees all associated memory
 * @param assembler Pointer to the assembler to destroy
 */
void coil_assembler_destroy(coil_assembler_t *assembler);

/**
 * @brief Sets the target architecture for the assembler
 * @param assembler Pointer to the assembler
 * @param target_name Name of the target architecture
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_target(coil_assembler_t *assembler, const char *target_name);

/**
 * @brief Sets a target configuration file for the assembler
 * @param assembler Pointer to the assembler
 * @param config_file Path to the configuration file
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_target_config(coil_assembler_t *assembler, const char *config_file);

/**
 * @brief Sets the optimization level for the assembler
 * @param assembler Pointer to the assembler
 * @param level Optimization level
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_optimization_level(coil_assembler_t *assembler, 
                                          coil_optimization_level_t level);

/**
 * @brief Sets the output format for the assembler
 * @param assembler Pointer to the assembler
 * @param format Output format
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_output_format(coil_assembler_t *assembler,
                                     coil_output_format_t format);

/**
 * @brief Sets a diagnostic handler for the assembler
 * @param assembler Pointer to the assembler
 * @param handler Diagnostic handler function
 * @param user_data User data to pass to the handler
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_diagnostics_handler(coil_assembler_t *assembler, 
                                          coil_diagnostics_handler_t handler,
                                          void *user_data);

/**
 * @brief Processes a COIL module
 * @param assembler Pointer to the assembler
 * @param module Pointer to the COIL module to process
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_process_module(coil_assembler_t *assembler, 
                                  coil_module_t *module);

/**
 * @brief Writes the assembled output to a file
 * @param assembler Pointer to the assembler
 * @param filename Path to the output file
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_write_output(coil_assembler_t *assembler, const char *filename);

/**
 * @brief Gets the last error message from the assembler
 * @param assembler Pointer to the assembler
 * @return Last error message or NULL if no error
 */
const char* coil_assembler_get_last_error(const coil_assembler_t *assembler);

/**
 * @brief Gets information about the supported targets
 * @param assembler Pointer to the assembler
 * @param target_count Pointer to store the number of targets
 * @return Array of target descriptors or NULL on failure
 */
const coil_target_descriptor_t* coil_assembler_get_targets(
    const coil_assembler_t *assembler, 
    uint32_t *target_count);

/**
 * @brief Gets information about the current target
 * @param assembler Pointer to the assembler
 * @return Current target descriptor or NULL if no target set
 */
const coil_target_descriptor_t* coil_assembler_get_current_target(
    const coil_assembler_t *assembler);

/**
 * @brief Example usage of the assembler API
 * @code
 * coil_assembler_t* assembler = coil_assembler_create();
 * coil_assembler_set_target(assembler, "x86_64");
 * coil_assembler_set_optimization_level(assembler, COIL_OPT_LEVEL_2);
 * 
 * coil_module_t* module = coil_module_load_from_file("input.coil");
 * coil_assembler_process_module(assembler, module);
 * coil_assembler_write_output(assembler, "output.o");
 * 
 * coil_module_destroy(module);
 * coil_assembler_destroy(assembler);
 * @endcode
 */

#endif /* COIL_ASSEMBLER_H */