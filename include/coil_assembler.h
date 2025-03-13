/**
 * @file coil_assembler.h
 * @brief Main header file for COIL Assembler.
 * 
 * This file defines the primary interfaces for the COIL Assembler,
 * which translates COIL binary code to native code for various target architectures.
 */

#ifndef COIL_ASSEMBLER_H
#define COIL_ASSEMBLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** 
 * @brief COIL assembler version information.
 */
#define COIL_ASSEMBLER_VERSION_MAJOR 0
#define COIL_ASSEMBLER_VERSION_MINOR 1
#define COIL_ASSEMBLER_VERSION_PATCH 0

/**
 * @brief Error codes for COIL assembler operations.
 */
typedef enum {
  COIL_SUCCESS = 0,                 /**< Operation completed successfully */
  COIL_ERROR_FILE_NOT_FOUND,        /**< Input file not found */
  COIL_ERROR_INVALID_FORMAT,        /**< Invalid COIL binary format */
  COIL_ERROR_UNSUPPORTED_TARGET,    /**< Target architecture not supported */
  COIL_ERROR_OUT_OF_MEMORY,         /**< Memory allocation failed */
  COIL_ERROR_INVALID_INSTRUCTION,   /**< Invalid COIL instruction */
  COIL_ERROR_UNSUPPORTED_FEATURE,   /**< Unsupported COIL feature */
  COIL_ERROR_UNKNOWN                /**< Unknown error */
} coil_error_t;

/**
 * @brief Device class enumeration.
 */
typedef enum {
  COIL_DEVICE_CPU,                  /**< Central Processing Unit */
  COIL_DEVICE_GPU,                  /**< Graphics Processing Unit */
  COIL_DEVICE_NPU,                  /**< Neural Processing Unit */
  COIL_DEVICE_TPU,                  /**< Tensor Processing Unit */
  COIL_DEVICE_DSP,                  /**< Digital Signal Processor */
  COIL_DEVICE_FPGA                  /**< Field-Programmable Gate Array */
} coil_device_class_t;

/**
 * @brief Opaque handle to a COIL module.
 */
typedef struct coil_module* coil_module_handle_t;

/**
 * @brief Opaque handle to a COIL assembler.
 */
typedef struct coil_assembler* coil_assembler_handle_t;

/**
 * @brief Opaque handle to a target configuration.
 */
typedef struct target_config* target_config_handle_t;

/**
 * @brief Initialize a COIL assembler for the specified target.
 * 
 * @param target_name Name of the target architecture.
 * @param device_class Class of processing device (default is CPU if NULL).
 * @return Assembler handle or NULL on error.
 */
coil_assembler_handle_t coil_assembler_init(const char* target_name, 
                                            coil_device_class_t device_class);

/**
 * @brief Clean up resources used by a COIL assembler.
 * 
 * @param assembler Assembler handle.
 */
void coil_assembler_cleanup(coil_assembler_handle_t assembler);

/**
 * @brief Load a COIL binary module from a file.
 * 
 * @param filename Path to the COIL binary file.
 * @return Module handle or NULL on error.
 */
coil_module_handle_t coil_module_load(const char* filename);

/**
 * @brief Free resources associated with a COIL module.
 * 
 * @param module Module handle.
 */
void coil_module_free(coil_module_handle_t module);

/**
 * @brief Assemble a COIL module to native code.
 * 
 * @param assembler Assembler handle.
 * @param module Module handle.
 * @param output_filename Path to the output native code file.
 * @return COIL_SUCCESS on success, error code otherwise.
 */
coil_error_t coil_assemble(coil_assembler_handle_t assembler, 
                            coil_module_handle_t module,
                            const char* output_filename);

/**
 * @brief Get the last error message.
 * 
 * @return Error message string.
 */
const char* coil_get_error_message(void);

/**
 * @brief Create a custom target configuration.
 * 
 * @param architecture Base architecture name (e.g., "x86_64", "arm64").
 * @param vendor Optional vendor name or NULL for generic.
 * @param device_class Processing device class.
 * @return Target configuration handle or NULL on error.
 */
target_config_handle_t coil_target_config_create(const char* architecture,
                                                const char* vendor,
                                                coil_device_class_t device_class);

/**
 * @brief Add a feature to a target configuration.
 * 
 * @param config Target configuration handle.
 * @param feature_name Feature name (e.g., "avx2", "neon").
 * @return COIL_SUCCESS on success, error code otherwise.
 */
coil_error_t coil_target_config_add_feature(target_config_handle_t config,
                                           const char* feature_name);

/**
 * @brief Free resources associated with a target configuration.
 * 
 * @param config Target configuration handle.
 */
void coil_target_config_free(target_config_handle_t config);

/**
 * @brief Set optimization level for code generation.
 * 
 * @param assembler Assembler handle.
 * @param level Optimization level (0-3).
 * @return COIL_SUCCESS on success, error code otherwise.
 */
coil_error_t coil_set_optimization_level(coil_assembler_handle_t assembler,
                                        int level);

#endif /* COIL_ASSEMBLER_H */