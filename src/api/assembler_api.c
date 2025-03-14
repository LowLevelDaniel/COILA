/**
 * @file assembler_api.c
 * @brief Implementation of the public assembler API
 * @details Provides the interface between applications and the COIL assembler.
 */
#include <stdlib.h>
#include <string.h>
#include "coil-assembler/assembler.h"
#include "coil-assembler/diagnostics.h"
#include "../core/assembler.h"

/**
 * @brief Creates a new assembler instance
 * @return Pointer to a new assembler instance or NULL on failure
 */
coil_assembler_t* coil_assembler_create(void) {
  coil_assembler_t* assembler = (coil_assembler_t*)malloc(sizeof(coil_assembler_t));
  
  if (!assembler) {
    return NULL;
  }
  
  /* Initialize to default values */
  memset(assembler, 0, sizeof(coil_assembler_t));
  
  /* Create diagnostics context */
  assembler->diag_context = coil_diagnostics_create();
  if (!assembler->diag_context) {
    coil_assembler_destroy(assembler);
    return NULL;
  }
  
  /* Set default optimization level */
  assembler->optimization_level = COIL_OPT_LEVEL_1;
  
  /* Set default output format */
  assembler->output_format = COIL_OUTPUT_FORMAT_OBJECT;
  
  /* Initialize target registry if needed */
  if (coil_target_registry_init() != 0) {
    coil_assembler_destroy(assembler);
    return NULL;
  }
  
  return assembler;
}

/**
 * @brief Destroys an assembler instance and frees all associated memory
 * @param assembler Pointer to the assembler to destroy
 */
void coil_assembler_destroy(coil_assembler_t* assembler) {
  if (!assembler) {
    return;
  }
  
  /* Free diagnostics context if created */
  if (assembler->diag_context) {
    coil_diagnostics_destroy(assembler->diag_context);
  }
  
  /* Free target context if created */
  if (assembler->target_context) {
    coil_target_context_destroy(assembler->target_context);
  }
  
  /* Free the assembler instance */
  free(assembler);
}

/**
 * @brief Sets the target architecture for the assembler
 * @param assembler Pointer to the assembler
 * @param target_name Name of the target architecture
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_target(coil_assembler_t* assembler, const char* target_name) {
  if (!assembler || !target_name) {
    return -1;
  }
  
  /* Get target descriptor by name */
  const coil_target_descriptor_t* descriptor = coil_get_target_by_name(target_name);
  if (!descriptor) {
    coil_diagnostics_reportf(assembler->diag_context, COIL_DIAG_ERROR, 
                           COIL_DIAG_CATEGORY_TARGET, 1,
                           "Target '%s' not found", target_name);
    return -1;
  }
  
  /* Create target context */
  coil_target_context_t* context = coil_target_context_create(descriptor);
  if (!context) {
    coil_diagnostics_reportf(assembler->diag_context, COIL_DIAG_ERROR, 
                           COIL_DIAG_CATEGORY_TARGET, 2,
                           "Failed to create target context for '%s'", target_name);
    return -1;
  }
  
  /* Free previous target context if any */
  if (assembler->target_context) {
    coil_target_context_destroy(assembler->target_context);
  }
  
  assembler->target_context = context;
  
  /* Initialize the target */
  if (descriptor->initialize && descriptor->initialize(context) != 0) {
    coil_diagnostics_reportf(assembler->diag_context, COIL_DIAG_ERROR, 
                           COIL_DIAG_CATEGORY_TARGET, 3,
                           "Failed to initialize target '%s'", target_name);
    return -1;
  }
  
  return 0;
}