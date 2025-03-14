/**
 * @file target_registry.c
 * @brief Target backend registry implementation
 * @details Implementation of the target registry for managing target architecture backends.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "coil-assembler/target.h"
#include "coil-assembler/diagnostics.h"
#include "../utils/memory.c"

/**
 * @brief Maximum number of target backends that can be registered
 */
#define MAX_TARGETS 32

/**
 * @brief Registry of target descriptors
 */
static coil_target_descriptor_t* target_registry[MAX_TARGETS];

/**
 * @brief Number of registered targets
 */
static uint32_t target_count = 0;

/**
 * @brief Thread-safe initialization flag (simple implementation)
 */
static int registry_initialized = 0;

/**
 * @brief Initialize the target registry
 * @return 0 on success, non-zero on failure
 */
static int initialize_registry(void) {
  if (registry_initialized) {
    return 0;  // Already initialized
  }
  
  // Initialize registry array to NULL
  for (uint32_t i = 0; i < MAX_TARGETS; i++) {
    target_registry[i] = NULL;
  }
  
  target_count = 0;
  registry_initialized = 1;
  return 0;
}

/**
 * @brief Register a target with the assembler
 * @param descriptor Target descriptor
 * @return 0 on success, non-zero on failure
 */
int coil_register_target(const coil_target_descriptor_t *descriptor) {
  // Validate input
  if (!descriptor) {
    coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                          1, "NULL target descriptor");
    return -1;
  }
  
  // Initialize registry if not already done
  if (!registry_initialized) {
    if (initialize_registry() != 0) {
      coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                            2, "Failed to initialize target registry");
      return -1;
    }
  }
  
  // Check if registry is full
  if (target_count >= MAX_TARGETS) {
    coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                          3, "Target registry is full");
    return -1;
  }
  
  // Check if target with the same name already exists
  for (uint32_t i = 0; i < target_count; i++) {
    if (target_registry[i] && 
        strcmp(target_registry[i]->name, descriptor->name) == 0) {
      coil_diagnostics_reportf(NULL, COIL_DIAG_WARNING, COIL_DIAG_CATEGORY_TARGET,
                             4, "Target '%s' already registered, will be replaced", 
                             descriptor->name);
      
      // Replace existing target
      coil_free(target_registry[i], sizeof(coil_target_descriptor_t));
      target_registry[i] = (coil_target_descriptor_t*)coil_memdup(descriptor, 
                                                      sizeof(coil_target_descriptor_t));
      return 0;
    }
  }
  
  // Create a copy of the descriptor to store in the registry
  coil_target_descriptor_t* copy = (coil_target_descriptor_t*)coil_memdup(
      descriptor, sizeof(coil_target_descriptor_t));
  
  if (!copy) {
    coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                          5, "Failed to allocate memory for target descriptor");
    return -1;
  }
  
  // If descriptor has features, copy them too
  if (descriptor->feature_count > 0 && descriptor->features) {
    const char** features_copy = (const char**)coil_calloc(
        descriptor->feature_count, sizeof(const char*));
    
    if (!features_copy) {
      coil_free(copy, sizeof(coil_target_descriptor_t));
      coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                            6, "Failed to allocate memory for target features");
      return -1;
    }
    
    // Copy each feature string
    for (uint32_t i = 0; i < descriptor->feature_count; i++) {
      if (descriptor->features[i]) {
        features_copy[i] = coil_strdup(descriptor->features[i]);
        if (!features_copy[i]) {
          // Clean up previously allocated features
          for (uint32_t j = 0; j < i; j++) {
            coil_free((void*)features_copy[j], strlen(features_copy[j]) + 1);
          }
          coil_free(features_copy, descriptor->feature_count * sizeof(const char*));
          coil_free(copy, sizeof(coil_target_descriptor_t));
          
          coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                                7, "Failed to allocate memory for target feature string");
          return -1;
        }
      }
    }
    
    // Update the copy to point to the new features array
    copy->features = features_copy;
  }
  
  // Add to registry
  target_registry[target_count++] = copy;
  
  coil_diagnostics_reportf(NULL, COIL_DIAG_INFO, COIL_DIAG_CATEGORY_TARGET,
                         0, "Registered target '%s': %s", 
                         copy->name, copy->description);
  
  return 0;
}

/**
 * @brief Get a target descriptor by name
 * @param name Target name
 * @return Target descriptor or NULL if not found
 */
const coil_target_descriptor_t* coil_get_target_by_name(const char *name) {
  // Validate input
  if (!name) {
    coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                          8, "NULL target name");
    return NULL;
  }
  
  // Ensure registry is initialized
  if (!registry_initialized) {
    if (initialize_registry() != 0) {
      coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                            9, "Failed to initialize target registry");
      return NULL;
    }
  }
  
  // Look for target with matching name
  for (uint32_t i = 0; i < target_count; i++) {
    if (target_registry[i] && strcmp(target_registry[i]->name, name) == 0) {
      return target_registry[i];
    }
  }
  
  // Not found
  coil_diagnostics_reportf(NULL, COIL_DIAG_WARNING, COIL_DIAG_CATEGORY_TARGET,
                         10, "Target '%s' not found", name);
  return NULL;
}

/**
 * @brief Get all registered targets
 * @param count Pointer to store the number of targets
 * @return Array of target descriptors or NULL on failure
 */
const coil_target_descriptor_t** coil_get_all_targets(uint32_t *count) {
  // Validate input
  if (!count) {
    coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                          11, "NULL count pointer");
    return NULL;
  }
  
  // Ensure registry is initialized
  if (!registry_initialized) {
    if (initialize_registry() != 0) {
      coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                            12, "Failed to initialize target registry");
      *count = 0;
      return NULL;
    }
  }
  
  // If no targets registered, return NULL
  if (target_count == 0) {
    *count = 0;
    return NULL;
  }
  
  // Allocate array for target pointers
  const coil_target_descriptor_t** targets = 
      (const coil_target_descriptor_t**)coil_calloc(
          target_count, sizeof(coil_target_descriptor_t*));
  
  if (!targets) {
    coil_diagnostics_report(NULL, COIL_DIAG_ERROR, COIL_DIAG_CATEGORY_TARGET,
                          13, "Failed to allocate memory for target list");
    *count = 0;
    return NULL;
  }
  
  // Copy pointers to the array
  for (uint32_t i = 0; i < target_count; i++) {
    targets[i] = target_registry[i];
  }
  
  *count = target_count;
  return targets;
}

/**
 * @brief Check if a target supports a specific feature
 * @param descriptor Target descriptor
 * @param feature Feature name
 * @return true if supported, false otherwise
 */
bool coil_target_has_feature(const coil_target_descriptor_t *descriptor,
                           const char *feature) {
  // Validate input
  if (!descriptor || !feature) {
    return false;
  }
  
  // Check if feature is in the list
  for (uint32_t i = 0; i < descriptor->feature_count; i++) {
    if (descriptor->features[i] && 
        strcmp(descriptor->features[i], feature) == 0) {
      return true;
    }
  }
  
  return false;
}

/**
 * @brief Free target registry resources
 * @return 0 on success, non-zero on failure
 */
int coil_target_registry_shutdown(void) {
  if (!registry_initialized) {
    return 0;  // Nothing to do
  }
  
  // Free all registered targets
  for (uint32_t i = 0; i < target_count; i++) {
    if (target_registry[i]) {
      // Free feature strings if any
      if (target_registry[i]->feature_count > 0 && 
          target_registry[i]->features) {
        for (uint32_t j = 0; j < target_registry[i]->feature_count; j++) {
          if (target_registry[i]->features[j]) {
            coil_free((void*)target_registry[i]->features[j], 
                    strlen(target_registry[i]->features[j]) + 1);
          }
        }
        coil_free((void*)target_registry[i]->features, 
                target_registry[i]->feature_count * sizeof(const char*));
      }
      
      // Free descriptor
      coil_free(target_registry[i], sizeof(coil_target_descriptor_t));
      target_registry[i] = NULL;
    }
  }
  
  target_count = 0;
  registry_initialized = 0;
  
  return 0;
}

/**
 * @brief Detect the current platform's target
 * @return Target name for the current platform or NULL on failure
 */
const char* coil_target_detect_current_platform(void) {
  // Simple platform detection based on preprocessor macros
  #if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
  #elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
  #elif defined(__riscv)
    return "riscv";
  #else
    // Unknown platform
    coil_diagnostics_report(NULL, COIL_DIAG_WARNING, COIL_DIAG_CATEGORY_TARGET,
                         14, "Could not detect current platform");
    return NULL;
  #endif
}

/**
 * @brief Initialize the target registry system
 * @return 0 on success, non-zero on failure
 */
int coil_target_registry_init(void) {
  return initialize_registry();
}