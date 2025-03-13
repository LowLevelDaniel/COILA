/**
 * @file target_config.h
 * @brief Target architecture configuration handler
 * 
 * This module loads, parses, and manages target architecture
 * configurations, providing access to architecture capabilities
 * and constraints.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef TARGET_CONFIG_H
#define TARGET_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "../utils/error_handling.h"

/**
 * @brief Maximum length of feature name
 */
#define MAX_FEATURE_NAME_LENGTH 32

/**
 * @brief Maximum number of features per target
 */
#define MAX_FEATURES 64

/**
 * @brief Memory model types
 */
typedef enum {
  MEMORY_MODEL_STRONG,         /**< Strong memory model */
  MEMORY_MODEL_ACQUIRE_RELEASE, /**< Acquire-release memory model */
  MEMORY_MODEL_RELAXED         /**< Relaxed memory model */
} memory_model_t;

/**
 * @brief Target resource properties
 */
typedef struct {
  uint32_t general_registers;  /**< Available general-purpose registers */
  uint32_t float_registers;    /**< Available floating-point registers */
  uint32_t vector_registers;   /**< Available vector registers */
  uint32_t vector_width;       /**< Native vector width in bits */
  uint32_t min_alignment;      /**< Minimum memory alignment */
  memory_model_t* memory_models; /**< Supported memory ordering models */
  uint32_t memory_model_count; /**< Number of supported memory models */
} target_resources_t;

/**
 * @brief Target memory properties
 */
typedef struct {
  uint32_t alignment;          /**< Alignment requirements */
  uint32_t page_size;          /**< Virtual memory page size */
  uint32_t cacheline_size;     /**< Cache line size */
} target_memory_t;

/**
 * @brief Target optimization properties
 */
typedef struct {
  uint32_t vector_threshold;   /**< Threshold for vectorization */
  uint32_t unroll_factor;      /**< Default loop unrolling factor */
  bool use_fma;                /**< Whether to use fused multiply-add */
  char** specialized_strategies; /**< Names of specialized optimization strategies */
  uint32_t strategy_count;     /**< Number of specialized strategies */
} target_optimization_t;

/**
 * @brief Target architecture configuration
 */
typedef struct {
  char* name;                  /**< Target name (e.g., "x86_64_avx2") */
  char* architecture;          /**< Base architecture name (e.g., "x86_64") */
  char* vendor;                /**< Hardware vendor or "generic" */
  char* description;           /**< Human-readable description */
  char** features;             /**< Available hardware features */
  uint32_t feature_count;      /**< Number of features */
  target_resources_t resources; /**< Resource properties */
  target_memory_t memory;      /**< Memory properties */
  target_optimization_t optimization; /**< Optimization properties */
} target_config_t;

/**
 * @brief Loads a target configuration from a file
 *
 * @param[in] path Path to the configuration file
 * @param[out] config Pointer to receive the loaded configuration
 * @return Error code indicating success or failure
 */
error_t target_config_load(const char* path, target_config_t** config);

/**
 * @brief Frees resources associated with a target configuration
 *
 * @param[in] config Pointer to the configuration to free
 * @return Error code indicating success or failure
 */
error_t target_config_free(target_config_t* config);

/**
 * @brief Checks if the target has a specific feature
 *
 * @param[in] config Pointer to the target configuration
 * @param[in] feature_name Name of the feature to check
 * @return true if the feature is available, false otherwise
 */
bool target_config_has_feature(const target_config_t* config, const char* feature_name);

/**
 * @brief Validates a target configuration for consistency
 *
 * @param[in] config Pointer to the configuration to validate
 * @return Error code indicating success or failure
 */
error_t target_config_validate(const target_config_t* config);

/**
 * @brief Automatically detects the current platform's target configuration
 *
 * @param[out] config Pointer to receive the detected configuration
 * @return Error code indicating success or failure
 */
error_t target_config_detect_current(target_config_t** config);

/**
 * @brief Creates an empty target configuration
 *
 * @param[out] config Pointer to receive the empty configuration
 * @return Error code indicating success or failure
 */
error_t target_config_create_empty(target_config_t** config);

/**
 * @brief Merges two target configurations, with the second taking precedence
 *
 * @param[in] base Base configuration
 * @param[in] override Configuration that overrides the base
 * @param[out] result Pointer to receive the merged configuration
 * @return Error code indicating success or failure
 */
error_t target_config_merge(
  const target_config_t* base,
  const target_config_t* override,
  target_config_t** result
);

#endif /* TARGET_CONFIG_H */