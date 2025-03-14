/**
 * @file target.h
 * @brief Target architecture definitions
 * @details This file defines the interfaces for target architectures in the COIL assembler.
 *          It provides structures and functions for describing target capabilities and
 *          handling target-specific code generation.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_TARGET_H
#define COIL_TARGET_H

#include <stdint.h>
#include <stdbool.h>
#include "../coil/binary.h"
#include "../coil/types.h"
#include "../coil/instructions.h"

/**
 * @brief Target endianness
 */
typedef enum {
  COIL_ENDIAN_LITTLE = 0, /**< Little-endian */
  COIL_ENDIAN_BIG = 1     /**< Big-endian */
} coil_endianness_t;

/**
 * @brief Target device class
 */
typedef enum {
  COIL_DEVICE_CPU = 0,  /**< Central Processing Unit */
  COIL_DEVICE_GPU = 1,  /**< Graphics Processing Unit */
  COIL_DEVICE_NPU = 2,  /**< Neural Processing Unit */
  COIL_DEVICE_TPU = 3,  /**< Tensor Processing Unit */
  COIL_DEVICE_DSP = 4,  /**< Digital Signal Processor */
  COIL_DEVICE_FPGA = 5, /**< Field-Programmable Gate Array */
  COIL_DEVICE_CUSTOM = 6 /**< Custom accelerator */
} coil_device_class_t;

/**
 * @brief Forward declaration of target context
 */
typedef struct coil_target_context_s coil_target_context_t;

/**
 * @brief Target initialization function type
 * @param context Target context to initialize
 * @return 0 on success, non-zero on failure
 */
typedef int (*coil_target_initialize_fn)(coil_target_context_t *context);

/**
 * @brief Target finalization function type
 * @param context Target context to finalize
 * @return 0 on success, non-zero on failure
 */
typedef int (*coil_target_finalize_fn)(coil_target_context_t *context);

/**
 * @brief Instruction mapping function type
 * @param context Target context
 * @param instruction COIL instruction to map
 * @return 0 on success, non-zero on failure
 */
typedef int (*coil_target_map_instruction_fn)(coil_target_context_t *context,
                                             coil_instruction_t *instruction);

/**
 * @brief Function generation function type
 * @param context Target context
 * @param function COIL function to generate
 * @return 0 on success, non-zero on failure
 */
typedef int (*coil_target_generate_function_fn)(coil_target_context_t *context,
                                               coil_function_t *function);

/**
 * @brief Target descriptor structure
 * @details Describes a target architecture and its capabilities
 */
typedef struct {
  /* Basic properties */
  const char *name;         /**< Target name */
  const char *description;  /**< Human-readable description */
  uint32_t    version;      /**< Target version */
  
  /* Architecture properties */
  uint32_t           word_size;   /**< Native word size in bits */
  coil_endianness_t  endianness;  /**< Byte order */
  coil_device_class_t device_class; /**< Device class */
  
  /* Feature flags */
  const char        **features;   /**< Supported features */
  uint32_t            feature_count; /**< Number of features */
  
  /* Target functions */
  coil_target_initialize_fn       initialize;      /**< Initialization function */
  coil_target_finalize_fn         finalize;        /**< Finalization function */
  coil_target_map_instruction_fn  map_instruction; /**< Instruction mapping function */
  coil_target_generate_function_fn generate_function; /**< Function generation function */
  
  /* Optional custom data */
  void *custom_data; /**< Target-specific data */
} coil_target_descriptor_t;

/**
 * @brief Resource properties structure
 * @details Describes hardware resources available on the target
 */
typedef struct {
  uint32_t general_registers;  /**< Available general-purpose registers */
  uint32_t float_registers;    /**< Available floating-point registers */
  uint32_t vector_registers;   /**< Available vector registers */
  uint32_t vector_width;       /**< Native vector width in bits */
  uint32_t min_alignment;      /**< Minimum memory alignment */
  uint32_t cache_line_size;    /**< Cache line size in bytes */
  uint32_t hardware_threads;   /**< Hardware thread count per core */
  uint32_t execution_units;    /**< Number of execution units */
  uint32_t pipeline_depth;     /**< Typical instruction pipeline depth */
  uint32_t issue_width;        /**< Maximum instructions issued per cycle */
} coil_target_resources_t;

/**
 * @brief Register a target with the assembler
 * @param descriptor Target descriptor
 * @return 0 on success, non-zero on failure
 */
int coil_register_target(const coil_target_descriptor_t *descriptor);

/**
 * @brief Get a target descriptor by name
 * @param name Target name
 * @return Target descriptor or NULL if not found
 */
const coil_target_descriptor_t* coil_get_target_by_name(const char *name);

/**
 * @brief Get all registered targets
 * @param count Pointer to store the number of targets
 * @return Array of target descriptors or NULL on failure
 */
const coil_target_descriptor_t** coil_get_all_targets(uint32_t *count);

/**
 * @brief Check if a target supports a specific feature
 * @param descriptor Target descriptor
 * @param feature Feature name
 * @return true if supported, false otherwise
 */
bool coil_target_has_feature(const coil_target_descriptor_t *descriptor,
                            const char *feature);

/**
 * @brief Create a target context
 * @param descriptor Target descriptor
 * @return New target context or NULL on failure
 */
coil_target_context_t* coil_target_context_create(const coil_target_descriptor_t *descriptor);

/**
 * @brief Destroy a target context
 * @param context Target context to destroy
 */
void coil_target_context_destroy(coil_target_context_t *context);

/**
 * @brief Set target resources
 * @param context Target context
 * @param resources Resource properties
 * @return 0 on success, non-zero on failure
 */
int coil_target_set_resources(coil_target_context_t *context,
                             const coil_target_resources_t *resources);

/**
 * @brief Get target resources
 * @param context Target context
 * @return Resource properties or NULL on failure
 */
const coil_target_resources_t* coil_target_get_resources(const coil_target_context_t *context);

/**
 * @brief Get the target descriptor from a context
 * @param context Target context
 * @return Target descriptor or NULL on failure
 */
const coil_target_descriptor_t* coil_target_get_descriptor(const coil_target_context_t *context);

/**
 * @brief Detect the current platform's target
 * @return Target name for the current platform or NULL on failure
 */
const char* coil_target_detect_current_platform(void);

#endif /* COIL_TARGET_H */