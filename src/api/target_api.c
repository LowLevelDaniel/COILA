/**
 * @file target_api.c
 * @brief Implementation of the target API
 * @details Provides functions for managing target architectures.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include "coil-assembler/target.h"
#include "../utils/memory.c"
#include "../utils/logging.c"

/**
 * @brief Target context structure
 */
struct coil_target_context_s {
  const coil_target_descriptor_t* descriptor; /**< Target descriptor */
  coil_target_resources_t resources;          /**< Target resources */
  void* target_data;                          /**< Target-specific data */
  void* assembler_data;                       /**< Assembler-specific data */
  int features_enabled[32];                   /**< Array of enabled features (32 max) */
  uint32_t feature_count;                     /**< Number of features supported */
};

/**
 * @brief Create a target context
 * @param descriptor Target descriptor
 * @return New target context or NULL on failure
 */
coil_target_context_t* coil_target_context_create(const coil_target_descriptor_t* descriptor) {
  if (!descriptor) {
    coil_log_error("Failed to create target context: NULL descriptor");
    return NULL;
  }
  
  coil_target_context_t* context = (coil_target_context_t*)coil_malloc(
      sizeof(coil_target_context_t));
  
  if (!context) {
    coil_log_error("Failed to allocate target context");
    return NULL;
  }
  
  /* Initialize context */
  context->descriptor = descriptor;
  context->target_data = NULL;
  context->assembler_data = NULL;
  context->feature_count = 0;
  
  /* Initialize resources to default values */
  memset(&context->resources, 0, sizeof(coil_target_resources_t));
  
  /* Initialize features */
  memset(context->features_enabled, 0, sizeof(context->features_enabled));
  
  /* Enable all features by default */
  if (descriptor->feature_count > 0 && descriptor->features) {
    /* Count valid features (limit to 32) */
    uint32_t valid_features = 0;
    for (uint32_t i = 0; i < descriptor->feature_count && valid_features < 32; i++) {
      if (descriptor->features[i]) {
        context->features_enabled[valid_features++] = 1;
      }
    }
    context->feature_count = valid_features;
  }
  
  coil_log_debug("Created target context for %s", descriptor->name);
  
  return context;
}

/**
 * @brief Destroy a target context
 * @param context Target context to destroy
 */
void coil_target_context_destroy(coil_target_context_t* context) {
  if (!context) {
    return;
  }
  
  /* Finalize the target if initialized */
  if (context->descriptor && context->descriptor->finalize) {
    context->descriptor->finalize(context);
  }
  
  /* Free context */
  coil_free(context, sizeof(coil_target_context_t));
  
  coil_log_debug("Destroyed target context");
}

/**
 * @brief Set target resources
 * @param context Target context
 * @param resources Resource properties
 * @return 0 on success, non-zero on failure
 */
int coil_target_set_resources(coil_target_context_t* context,
                            const coil_target_resources_t* resources) {
  if (!context || !resources) {
    coil_log_error("Failed to set target resources: invalid parameters");
    return -1;
  }
  
  /* Copy resources */
  memcpy(&context->resources, resources, sizeof(coil_target_resources_t));
  
  coil_log_debug("Set target resources (general_registers=%u, float_registers=%u, vector_registers=%u)",
               resources->general_registers, resources->float_registers, resources->vector_registers);
  
  return 0;
}

/**
 * @brief Get target resources
 * @param context Target context
 * @return Resource properties or NULL on failure
 */
const coil_target_resources_t* coil_target_get_resources(const coil_target_context_t* context) {
  if (!context) {
    coil_log_error("Failed to get target resources: NULL context");
    return NULL;
  }
  
  return &context->resources;
}

/**
 * @brief Get the target descriptor from a context
 * @param context Target context
 * @return Target descriptor or NULL on failure
 */
const coil_target_descriptor_t* coil_target_get_descriptor(const coil_target_context_t* context) {
  if (!context) {
    coil_log_error("Failed to get target descriptor: NULL context");
    return NULL;
  }
  
  return context->descriptor;
}

/**
 * @brief Set target-specific data
 * @param context Target context
 * @param data Target-specific data
 * @return 0 on success, non-zero on failure
 */
int coil_target_set_data(coil_target_context_t* context, void* data) {
  if (!context) {
    coil_log_error("Failed to set target data: NULL context");
    return -1;
  }
  
  context->target_data = data;
  
  return 0;
}

/**
 * @brief Get target-specific data
 * @param context Target context
 * @return Target-specific data or NULL on failure
 */
void* coil_target_get_data(const coil_target_context_t* context) {
  if (!context) {
    coil_log_error("Failed to get target data: NULL context");
    return NULL;
  }
  
  return context->target_data;
}

/**
 * @brief Set assembler-specific data
 * @param context Target context
 * @param data Assembler-specific data
 * @return 0 on success, non-zero on failure
 */
int coil_target_set_assembler_data(coil_target_context_t* context, void* data) {
  if (!context) {
    coil_log_error("Failed to set assembler data: NULL context");
    return -1;
  }
  
  context->assembler_data = data;
  
  return 0;
}

/**
 * @brief Get assembler-specific data
 * @param context Target context
 * @return Assembler-specific data or NULL on failure
 */
void* coil_target_get_assembler_data(const coil_target_context_t* context) {
  if (!context) {
    coil_log_error("Failed to get assembler data: NULL context");
    return NULL;
  }
  
  return context->assembler_data;
}

/**
 * @brief Check if a target supports a specific feature
 * @param descriptor Target descriptor
 * @param feature Feature name
 * @return true if supported, false otherwise
 */
bool coil_target_has_feature(const coil_target_descriptor_t* descriptor,
                           const char* feature) {
  if (!descriptor || !feature) {
    return false;
  }
  
  /* Check if feature is in the list */
  for (uint32_t i = 0; i < descriptor->feature_count; i++) {
    if (descriptor->features[i] && 
        strcmp(descriptor->features[i], feature) == 0) {
      return true;
    }
  }
  
  return false;
}

/**
 * @brief Enable or disable a specific feature
 * @param context Target context
 * @param feature Feature name
 * @param enabled Non-zero to enable, zero to disable
 * @return 0 on success, non-zero on failure
 */
int coil_target_set_feature_enabled(coil_target_context_t* context,
                                  const char* feature,
                                  int enabled) {
  if (!context || !feature) {
    coil_log_error("Failed to set feature state: invalid parameters");
    return -1;
  }
  
  /* Find feature index */
  int feature_index = -1;
  for (uint32_t i = 0; i < context->descriptor->feature_count; i++) {
    if (context->descriptor->features[i] && 
        strcmp(context->descriptor->features[i], feature) == 0) {
      feature_index = i;
      break;
    }
  }
  
  if (feature_index < 0 || feature_index >= 32) {
    coil_log_warning("Feature not found or out of range: %s", feature);
    return -1;
  }
  
  /* Set feature state */
  context->features_enabled[feature_index] = enabled ? 1 : 0;
  
  coil_log_debug("%s feature: %s", enabled ? "Enabled" : "Disabled", feature);
  
  return 0;
}

/**
 * @brief Check if a feature is enabled
 * @param context Target context
 * @param feature Feature name
 * @return true if enabled, false otherwise
 */
bool coil_target_is_feature_enabled(const coil_target_context_t* context,
                                  const char* feature) {
  if (!context || !feature) {
    return false;
  }
  
  /* Find feature index */
  int feature_index = -1;
  for (uint32_t i = 0; i < context->descriptor->feature_count; i++) {
    if (context->descriptor->features[i] && 
        strcmp(context->descriptor->features[i], feature) == 0) {
      feature_index = i;
      break;
    }
  }
  
  if (feature_index < 0 || feature_index >= 32) {
    return false;
  }
  
  /* Get feature state */
  return context->features_enabled[feature_index] != 0;
}

/**
 * @brief Detect the current platform's target
 * @return Target name for the current platform or NULL on failure
 */
const char* coil_target_detect_current_platform(void) {
  /* Simple platform detection based on preprocessor macros */
  #if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
  #elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
  #elif defined(__riscv)
    return "riscv";
  #else
    /* Unknown platform */
    coil_log_warning("Could not detect current platform");
    return NULL;
  #endif
}

/**
 * @brief Get architecture word size in bits
 * @param context Target context
 * @return Word size in bits or 0 on failure
 */
uint32_t coil_target_get_word_size(const coil_target_context_t* context) {
  if (!context || !context->descriptor) {
    coil_log_error("Failed to get word size: invalid context");
    return 0;
  }
  
  return context->descriptor->word_size;
}

/**
 * @brief Get architecture endianness
 * @param context Target context
 * @return Endianness or COIL_ENDIAN_LITTLE on failure
 */
coil_endianness_t coil_target_get_endianness(const coil_target_context_t* context) {
  if (!context || !context->descriptor) {
    coil_log_error("Failed to get endianness: invalid context");
    return COIL_ENDIAN_LITTLE;
  }
  
  return context->descriptor->endianness;
}

/**
 * @brief Get architecture device class
 * @param context Target context
 * @return Device class or COIL_DEVICE_CPU on failure
 */
coil_device_class_t coil_target_get_device_class(const coil_target_context_t* context) {
  if (!context || !context->descriptor) {
    coil_log_error("Failed to get device class: invalid context");
    return COIL_DEVICE_CPU;
  }
  
  return context->descriptor->device_class;
}

/**
 * @brief Get a list of supported features
 * @param context Target context
 * @param count Pointer to store the number of features
 * @return Array of feature names or NULL on failure
 */
const char** coil_target_get_features(const coil_target_context_t* context,
                                    uint32_t* count) {
  if (!context || !context->descriptor || !count) {
    coil_log_error("Failed to get features: invalid parameters");
    return NULL;
  }
  
  *count = context->descriptor->feature_count;
  return context->descriptor->features;
}

/**
 * @brief Map COIL instruction to target instructions
 * @param context Target context
 * @param instruction COIL instruction to map
 * @return 0 on success, non-zero on failure
 */
int coil_target_map_instruction(coil_target_context_t* context,
                              coil_instruction_t* instruction) {
  if (!context || !context->descriptor || !instruction) {
    coil_log_error("Failed to map instruction: invalid parameters");
    return -1;
  }
  
  /* Check if target has mapping function */
  if (!context->descriptor->map_instruction) {
    coil_log_error("Target does not support instruction mapping");
    return -1;
  }
  
  /* Call target-specific mapping function */
  return context->descriptor->map_instruction(context, instruction);
}

/**
 * @brief Generate native code for a function
 * @param context Target context
 * @param function COIL function to generate
 * @return 0 on success, non-zero on failure
 */
int coil_target_generate_function(coil_target_context_t* context,
                                coil_function_t* function) {
  if (!context || !context->descriptor || !function) {
    coil_log_error("Failed to generate function: invalid parameters");
    return -1;
  }
  
  /* Check if target has function generation function */
  if (!context->descriptor->generate_function) {
    coil_log_error("Target does not support function generation");
    return -1;
  }
  
  /* Call target-specific function generation function */
  coil_log_debug("Generating code for function '%s'", function->name);
  return context->descriptor->generate_function(context, function);
}

/**
 * @brief Get the default alignment for a target
 * @param context Target context
 * @return Default alignment in bytes or 0 on failure
 */
uint32_t coil_target_get_default_alignment(const coil_target_context_t* context) {
  if (!context) {
    coil_log_error("Failed to get default alignment: NULL context");
    return 0;
  }
  
  /* Get from resources */
  return context->resources.min_alignment;
}

/**
 * @brief Get the cache line size for a target
 * @param context Target context
 * @return Cache line size in bytes or 0 on failure
 */
uint32_t coil_target_get_cache_line_size(const coil_target_context_t* context) {
  if (!context) {
    coil_log_error("Failed to get cache line size: NULL context");
    return 0;
  }
  
  /* Get from resources */
  return context->resources.cache_line_size;
}

/**
 * @brief Get the natural alignment for a type on a target
 * @param context Target context
 * @param type Type to get alignment for
 * @return Natural alignment in bytes or 0 on failure
 */
uint32_t coil_target_get_type_alignment(const coil_target_context_t* context,
                                      coil_type_t type) {
  if (!context) {
    coil_log_error("Failed to get type alignment: NULL context");
    return 0;
  }
  
  /* Compute alignment based on type width and target properties */
  uint8_t width = coil_type_get_width(type);
  
  /* If width is 0, return the minimum alignment */
  if (width == 0) {
    return context->resources.min_alignment;
  }
  
  /* Convert width in bits to bytes */
  uint8_t size = width / 8;
  
  /* Round up to power of 2 */
  uint32_t alignment = 1;
  while (alignment < size) {
    alignment *= 2;
  }
  
  /* Ensure minimum alignment */
  if (alignment < context->resources.min_alignment) {
    alignment = context->resources.min_alignment;
  }
  
  return alignment;
}

/**
 * @brief Initialize the target
 * @param context Target context
 * @return 0 on success, non-zero on failure
 */
int coil_target_initialize(coil_target_context_t* context) {
  if (!context || !context->descriptor) {
    coil_log_error("Failed to initialize target: invalid context");
    return -1;
  }
  
  /* Check if target has initialization function */
  if (!context->descriptor->initialize) {
    coil_log_warning("Target does not have initialization function");
    return 0;
  }
  
  /* Call target-specific initialization function */
  coil_log_info("Initializing target: %s", context->descriptor->name);
  return context->descriptor->initialize(context);
}

/**
 * @brief Finalize the target
 * @param context Target context
 * @return 0 on success, non-zero on failure
 */
int coil_target_finalize(coil_target_context_t* context) {
  if (!context || !context->descriptor) {
    coil_log_error("Failed to finalize target: invalid context");
    return -1;
  }
  
  /* Check if target has finalization function */
  if (!context->descriptor->finalize) {
    coil_log_warning("Target does not have finalization function");
    return 0;
  }
  
  /* Call target-specific finalization function */
  coil_log_info("Finalizing target: %s", context->descriptor->name);
  return context->descriptor->finalize(context);
}