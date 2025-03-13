/**
 * @file coil_assembler.h
 * @brief Public API for the COIL assembler
 * 
 * This header defines the public interface for the COIL assembler,
 * allowing clients to translate COIL binary code to native code for
 * various target architectures, with a primary focus on x86.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef COIL_ASSEMBLER_H
#define COIL_ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status codes returned by the assembler functions
 */
typedef enum {
  COIL_SUCCESS = 0,            /**< Operation completed successfully */
  COIL_ERROR_INVALID_ARGS,     /**< Invalid arguments provided */
  COIL_ERROR_FILE_IO,          /**< File input/output error */
  COIL_ERROR_MEMORY,           /**< Memory allocation or management error */
  COIL_ERROR_INVALID_BINARY,   /**< Invalid COIL binary format */
  COIL_ERROR_UNSUPPORTED_INSTR, /**< Unsupported instruction encountered */
  COIL_ERROR_CONFIG,           /**< Configuration error */
  COIL_ERROR_TRANSLATION,      /**< Error during translation */
  COIL_ERROR_OPTIMIZATION,     /**< Error during optimization */
  COIL_ERROR_GENERATION,       /**< Error during native code generation */
  COIL_ERROR_UNKNOWN           /**< Unknown error */
} coil_status_t;

/**
 * @brief Opaque handle to a COIL assembler instance
 */
typedef struct coil_assembler_t* coil_assembler_handle_t;

/**
 * @brief Optimization levels for the assembler
 */
typedef enum {
  COIL_OPT_NONE = 0,    /**< No optimizations */
  COIL_OPT_BASIC = 1,    /**< Basic optimizations */
  COIL_OPT_MODERATE = 2, /**< Moderate optimizations */
  COIL_OPT_AGGRESSIVE = 3 /**< Aggressive optimizations */
} coil_opt_level_t;

/**
 * @brief Configuration options for the assembler
 */
typedef struct {
  const char* target_name;     /**< Target architecture name (e.g., "x86_64") */
  const char* config_file;     /**< Path to target configuration file */
  coil_opt_level_t opt_level;  /**< Optimization level */
  uint8_t verbose;             /**< Verbosity level (0-3) */
  uint8_t debug_info;          /**< Whether to include debug information */
} coil_assembler_config_t;

/**
 * @brief Creates a new COIL assembler instance
 *
 * @param[in] config Configuration options for the assembler
 * @param[out] handle Pointer to receive the assembler handle
 * @return Status code indicating success or failure
 */
coil_status_t coil_assembler_create(
  const coil_assembler_config_t* config,
  coil_assembler_handle_t* handle
);

/**
 * @brief Destroys a COIL assembler instance and frees all associated resources
 *
 * @param[in] handle Assembler handle to destroy
 * @return Status code indicating success or failure
 */
coil_status_t coil_assembler_destroy(coil_assembler_handle_t handle);

/**
 * @brief Assembles COIL binary code to native code
 *
 * @param[in] handle Assembler handle
 * @param[in] coil_binary Pointer to COIL binary data
 * @param[in] coil_binary_size Size of the COIL binary data in bytes
 * @param[out] native_binary Pointer to receive the generated native binary
 * @param[out] native_binary_size Pointer to receive the size of the native binary
 * @return Status code indicating success or failure
 */
coil_status_t coil_assembler_assemble(
  coil_assembler_handle_t handle,
  const uint8_t* coil_binary,
  size_t coil_binary_size,
  uint8_t** native_binary,
  size_t* native_binary_size
);

/**
 * @brief Assembles COIL binary code from a file to native code in a file
 *
 * @param[in] handle Assembler handle
 * @param[in] coil_filename Path to COIL binary file
 * @param[in] native_filename Path to output native binary file
 * @return Status code indicating success or failure
 */
coil_status_t coil_assembler_assemble_file(
  coil_assembler_handle_t handle,
  const char* coil_filename,
  const char* native_filename
);

/**
 * @brief Gets a string description for a status code
 *
 * @param[in] status Status code
 * @return Null-terminated string describing the status code
 */
const char* coil_status_string(coil_status_t status);

/**
 * @brief Gets the version of the COIL assembler
 *
 * @return Null-terminated string containing the version (major.minor.patch)
 */
const char* coil_assembler_version(void);

#ifdef __cplusplus
}
#endif

#endif /* COIL_ASSEMBLER_H */