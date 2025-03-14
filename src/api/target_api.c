/**
 * @file target_api.c
 * @brief Implementation of the target API
 * @details Provides functions for managing target architectures.
 */
#include <stdlib.h>
#include <string.h>
#include "coil-assembler/target.h"
#include "../utils/memory.c"

/**
 * @brief Target context structure
 */
struct coil_target_context_s {
  const coil_target_descriptor_t* descriptor;
  coil_target_resources_t resources;
  void* target_data;
  void* assembler_data;
};

/**
 * @brief Create a target context
 * @param descriptor Target descriptor
 * @return New target context or NULL on failure
 */
coil_target_context_t* coil_target_context_create(const coil_target_descriptor_t* descriptor) {
  if (!descriptor) {
    return NULL;
  }
  
  coil_target_context_t* context = (coil_target_context_t*)coil_malloc(
      sizeof(coil_target_context_t));
  
  if (!context) {
    return NULL;
  }
  
  /* Initialize context */
  context->descriptor = descriptor;
  context->target_data = NULL;
  context->assembler_data = NULL;
  
  /* Initialize resources to default values */
  memset(&context->resources, 0, sizeof(coil_target_resources_t));
  
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
    return -1;
  }
  
  /* Copy resources */
  memcpy(&context->resources, resources, sizeof(coil_target_resources_t));
  
  return 0;
}

/**
 * @brief Get target resources
 * @param context Target context
 * @return Resource properties or NULL on failure
 */
const coil_target_resources_t* coil_target_get_resources(const coil_target_context_t* context) {
  if (!context) {
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
    return NULL;
  }
  
  return context->descriptor;
}