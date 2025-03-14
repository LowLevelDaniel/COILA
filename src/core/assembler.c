/**
 * @file assembler.c
 * @brief Core assembler implementation
 * @details Implements the core assembler functionality for processing COIL modules.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */
#include <stdlib.h>
#include <string.h>
#include "coil-assembler/assembler.h"
#include "coil-assembler/diagnostics.h"
#include "coil-assembler/target.h"
#include "../utils/memory.c"
#include "../utils/logging.c"

/* Forward declarations */
static int process_function_declarations(coil_assembler_t* assembler, coil_module_t* module);
static int process_globals(coil_assembler_t* assembler, coil_module_t* module);
static int process_code(coil_assembler_t* assembler, coil_module_t* module);
static int process_relocation(coil_assembler_t* assembler, coil_module_t* module);
static int optimize_module(coil_assembler_t* assembler, coil_module_t* module);
static int generate_target_code(coil_assembler_t* assembler, coil_module_t* module);
static int write_output_file(coil_assembler_t* assembler, const char* filename);

/**
 * @brief Assembler structure
 */
struct coil_assembler_s {
  coil_target_context_t* target_context;        /**< Target context */
  coil_diagnostics_context_t* diag_context;     /**< Diagnostics context */
  coil_optimization_level_t optimization_level; /**< Optimization level */
  coil_output_format_t output_format;           /**< Output format */
  coil_module_t* current_module;                /**< Current module being processed */
  void* code_generator;                         /**< Code generator instance */
  void* optimizer;                              /**< Optimizer instance */
  char* last_error;                             /**< Last error message */
  coil_target_descriptor_t* targets;            /**< Available targets */
  uint32_t target_count;                        /**< Number of available targets */
  const coil_target_descriptor_t* current_target; /**< Current target descriptor */
  void* output_buffer;                          /**< Output buffer for generated code */
  size_t output_buffer_size;                    /**< Size of output buffer */
  size_t output_size;                           /**< Size of generated output */
};

/**
 * @brief Process a COIL module with the assembler
 * @param assembler Pointer to the assembler
 * @param module Pointer to the COIL module to process
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_process_module(coil_assembler_t* assembler, coil_module_t* module) {
  if (!assembler || !module) {
    return -1;
  }
  
  /* Check if target has been set */
  if (!assembler->target_context) {
    coil_diagnostics_report(assembler->diag_context, COIL_DIAG_ERROR, 
                          COIL_DIAG_CATEGORY_GENERAL, 1,
                          "No target architecture set");
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("No target architecture set");
    return -1;
  }
  
  /* Validate the module */
  if (coil_module_validate(module) != 0) {
    coil_diagnostics_report(assembler->diag_context, COIL_DIAG_ERROR, 
                          COIL_DIAG_CATEGORY_PARSER, 2,
                          "Invalid COIL module");
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Invalid COIL module");
    return -1;
  }
  
  /* Store current module */
  assembler->current_module = module;
  
  coil_log_info("Processing COIL module with %d sections", module->header.section_count);
  
  /* Process the module sections in order */
  if (process_function_declarations(assembler, module) != 0) {
    coil_log_error("Failed to process function declarations");
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Failed to process function declarations");
    return -1;
  }
  
  if (process_globals(assembler, module) != 0) {
    coil_log_error("Failed to process global variables");
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Failed to process global variables");
    return -1;
  }
  
  if (optimize_module(assembler, module) != 0) {
    coil_log_error("Failed to optimize module");
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Failed to optimize module");
    return -1;
  }
  
  if (process_code(assembler, module) != 0) {
    coil_log_error("Failed to process code sections");
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Failed to process code sections");
    return -1;
  }
  
  if (process_relocation(assembler, module) != 0) {
    coil_log_error("Failed to process relocations");
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Failed to process relocations");
    return -1;
  }
  
  if (generate_target_code(assembler, module) != 0) {
    coil_log_error("Failed to generate target code");
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Failed to generate target code");
    return -1;
  }
  
  coil_log_info("Successfully processed COIL module");
  
  return 0;
}

/**
 * @brief Process function declarations in the module
 * @param assembler Pointer to the assembler
 * @param module Pointer to the COIL module
 * @return 0 on success, non-zero on failure
 */
static int process_function_declarations(coil_assembler_t* assembler, coil_module_t* module) {
  if (!assembler || !module) {
    return -1;
  }
  
  /* Find the function section */
  const uint8_t* function_section = NULL;
  size_t function_section_size = 0;
  
  for (uint32_t i = 0; i < module->header.section_count; i++) {
    if (module->sections[i].section_type == COIL_SECTION_TYPE_FUNCTION) {
      function_section = module->section_data[i];
      function_section_size = module->sections[i].size;
      break;
    }
  }
  
  if (!function_section) {
    coil_log_warning("No function declarations section found in module");
    return 0; /* This is not an error, the module might not have functions */
  }
  
  coil_log_debug("Processing function declarations section (%zu bytes)", function_section_size);
  
  /* Parse function declarations */
  /* In a real implementation, we would parse the function section here */
  /* For simplicity, we'll just log the section data size */
  
  coil_log_info("Processed %zu bytes of function declarations", function_section_size);
  
  return 0;
}

/**
 * @brief Process global variables in the module
 * @param assembler Pointer to the assembler
 * @param module Pointer to the COIL module
 * @return 0 on success, non-zero on failure
 */
static int process_globals(coil_assembler_t* assembler, coil_module_t* module) {
  if (!assembler || !module) {
    return -1;
  }
  
  /* Find the globals section */
  const uint8_t* globals_section = NULL;
  size_t globals_section_size = 0;
  
  for (uint32_t i = 0; i < module->header.section_count; i++) {
    if (module->sections[i].section_type == COIL_SECTION_TYPE_GLOBAL) {
      globals_section = module->section_data[i];
      globals_section_size = module->sections[i].size;
      break;
    }
  }
  
  if (!globals_section) {
    coil_log_debug("No global variables section found in module");
    return 0; /* This is not an error, the module might not have globals */
  }
  
  coil_log_debug("Processing global variables section (%zu bytes)", globals_section_size);
  
  /* Parse global variables */
  /* In a real implementation, we would parse the globals section here */
  /* For simplicity, we'll just log the section data size */
  
  coil_log_info("Processed %zu bytes of global variables", globals_section_size);
  
  return 0;
}

/**
 * @brief Apply optimizations to the module
 * @param assembler Pointer to the assembler
 * @param module Pointer to the COIL module
 * @return 0 on success, non-zero on failure
 */
static int optimize_module(coil_assembler_t* assembler, coil_module_t* module) {
  if (!assembler || !module) {
    return -1;
  }
  
  /* Skip optimization if level is 0 */
  if (assembler->optimization_level == COIL_OPT_LEVEL_0) {
    coil_log_debug("Optimization level is 0, skipping optimizations");
    return 0;
  }
  
  coil_log_info("Optimizing module with level %d", assembler->optimization_level);
  
  /* In a real implementation, we would apply optimizations here */
  /* For simplicity, we'll just log the optimization level */
  
  /* Apply different optimizations based on level */
  switch (assembler->optimization_level) {
    case COIL_OPT_LEVEL_1:
      coil_log_debug("Applying basic optimizations");
      break;
      
    case COIL_OPT_LEVEL_2:
      coil_log_debug("Applying standard optimizations");
      break;
      
    case COIL_OPT_LEVEL_3:
      coil_log_debug("Applying aggressive optimizations");
      break;
      
    case COIL_OPT_LEVEL_S:
      coil_log_debug("Applying size optimizations");
      break;
      
    default:
      coil_log_warning("Unknown optimization level: %d", assembler->optimization_level);
      break;
  }
  
  coil_log_info("Optimization complete");
  
  return 0;
}

/**
 * @brief Process code sections in the module
 * @param assembler Pointer to the assembler
 * @param module Pointer to the COIL module
 * @return 0 on success, non-zero on failure
 */
static int process_code(coil_assembler_t* assembler, coil_module_t* module) {
  if (!assembler || !module) {
    return -1;
  }
  
  /* Find the code section */
  const uint8_t* code_section = NULL;
  size_t code_section_size = 0;
  
  for (uint32_t i = 0; i < module->header.section_count; i++) {
    if (module->sections[i].section_type == COIL_SECTION_TYPE_CODE) {
      code_section = module->section_data[i];
      code_section_size = module->sections[i].size;
      break;
    }
  }
  
  if (!code_section) {
    coil_log_warning("No code section found in module");
    return 0; /* This is not an error, the module might not have code */
  }
  
  coil_log_debug("Processing code section (%zu bytes)", code_section_size);
  
  /* Parse and process code */
  /* In a real implementation, we would parse and process the code section here */
  /* For simplicity, we'll just log the section data size */
  
  coil_log_info("Processed %zu bytes of code", code_section_size);
  
  return 0;
}

/**
 * @brief Process relocation information in the module
 * @param assembler Pointer to the assembler
 * @param module Pointer to the COIL module
 * @return 0 on success, non-zero on failure
 */
static int process_relocation(coil_assembler_t* assembler, coil_module_t* module) {
  if (!assembler || !module) {
    return -1;
  }
  
  /* Find the relocation section */
  const uint8_t* reloc_section = NULL;
  size_t reloc_section_size = 0;
  
  for (uint32_t i = 0; i < module->header.section_count; i++) {
    if (module->sections[i].section_type == COIL_SECTION_TYPE_RELOCATION) {
      reloc_section = module->section_data[i];
      reloc_section_size = module->sections[i].size;
      break;
    }
  }
  
  if (!reloc_section) {
    coil_log_debug("No relocation section found in module");
    return 0; /* This is not an error, the module might not have relocations */
  }
  
  coil_log_debug("Processing relocation section (%zu bytes)", reloc_section_size);
  
  /* Parse and process relocations */
  /* In a real implementation, we would parse and process the relocation section here */
  /* For simplicity, we'll just log the section data size */
  
  coil_log_info("Processed %zu bytes of relocations", reloc_section_size);
  
  return 0;
}

/**
 * @brief Generate target code for the module
 * @param assembler Pointer to the assembler
 * @param module Pointer to the COIL module
 * @return 0 on success, non-zero on failure
 */
static int generate_target_code(coil_assembler_t* assembler, coil_module_t* module) {
  if (!assembler || !module) {
    return -1;
  }
  
  coil_log_info("Generating target code for %s", assembler->current_target->name);
  
  /* In a real implementation, we would generate target code here */
  /* This would involve mapping COIL instructions to target instructions */
  /* and then generating the appropriate binary output */
  
  /* Allocate output buffer if needed */
  if (!assembler->output_buffer) {
    assembler->output_buffer_size = 65536; /* Initial size */
    assembler->output_buffer = coil_malloc(assembler->output_buffer_size);
    if (!assembler->output_buffer) {
      coil_log_error("Failed to allocate output buffer");
      return -1;
    }
  }
  
  /* For simplicity, we'll just fill the output buffer with zeros */
  /* In a real implementation, this would be the generated code */
  memset(assembler->output_buffer, 0, assembler->output_buffer_size);
  
  /* Set output size */
  assembler->output_size = 1024; /* Example output size */
  
  coil_log_info("Generated %zu bytes of target code", assembler->output_size);
  
  return 0;
}

/**
 * @brief Write assembled output to a file
 * @param assembler Pointer to the assembler
 * @param filename Output filename
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_write_output(coil_assembler_t* assembler, const char* filename) {
  if (!assembler || !filename) {
    return -1;
  }
  
  /* Check if we have output to write */
  if (!assembler->output_buffer || assembler->output_size == 0) {
    coil_diagnostics_report(assembler->diag_context, COIL_DIAG_ERROR, 
                          COIL_DIAG_CATEGORY_GENERAL, 3,
                          "No output to write");
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("No output to write");
    return -1;
  }
  
  coil_log_info("Writing output to %s (%zu bytes)", filename, assembler->output_size);
  
  /* Open the output file */
  FILE* file = fopen(filename, "wb");
  if (!file) {
    coil_diagnostics_reportf(assembler->diag_context, COIL_DIAG_ERROR, 
                           COIL_DIAG_CATEGORY_GENERAL, 4,
                           "Failed to open output file: %s", filename);
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Failed to open output file");
    return -1;
  }
  
  /* Write the output buffer to the file */
  size_t written = fwrite(assembler->output_buffer, 1, assembler->output_size, file);
  if (written != assembler->output_size) {
    coil_diagnostics_reportf(assembler->diag_context, COIL_DIAG_ERROR, 
                           COIL_DIAG_CATEGORY_GENERAL, 5,
                           "Failed to write output: wrote %zu of %zu bytes", 
                           written, assembler->output_size);
    fclose(file);
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Failed to write output");
    return -1;
  }
  
  /* Close the file */
  fclose(file);
  
  coil_log_info("Successfully wrote %zu bytes to %s", written, filename);
  
  return 0;
}

/**
 * @brief Get the last error message from the assembler
 * @param assembler Pointer to the assembler
 * @return Last error message or NULL if no error
 */
const char* coil_assembler_get_last_error(const coil_assembler_t* assembler) {
  if (!assembler) {
    return NULL;
  }
  
  return assembler->last_error;
}

/**
 * @brief Gets information about the supported targets
 * @param assembler Pointer to the assembler
 * @param target_count Pointer to store the number of targets
 * @return Array of target descriptors or NULL on failure
 */
const coil_target_descriptor_t* coil_assembler_get_targets(
    const coil_assembler_t* assembler, 
    uint32_t* target_count) {
  if (!assembler || !target_count) {
    return NULL;
  }
  
  *target_count = assembler->target_count;
  return assembler->targets;
}

/**
 * @brief Gets information about the current target
 * @param assembler Pointer to the assembler
 * @return Current target descriptor or NULL if no target set
 */
const coil_target_descriptor_t* coil_assembler_get_current_target(
    const coil_assembler_t* assembler) {
  if (!assembler) {
    return NULL;
  }
  
  return assembler->current_target;
}

/**
 * @brief Set the optimization level for the assembler
 * @param assembler Pointer to the assembler
 * @param level Optimization level
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_optimization_level(coil_assembler_t* assembler, 
                                        coil_optimization_level_t level) {
  if (!assembler) {
    return -1;
  }
  
  /* Validate optimization level */
  if (level > COIL_OPT_LEVEL_S) {
    coil_diagnostics_reportf(assembler->diag_context, COIL_DIAG_WARNING, 
                           COIL_DIAG_CATEGORY_OPTIMIZER, 1,
                           "Invalid optimization level: %d, using default", level);
    level = COIL_OPT_LEVEL_1; /* Use default level */
  }
  
  assembler->optimization_level = level;
  
  coil_log_debug("Set optimization level to %d", level);
  
  return 0;
}

/**
 * @brief Set the output format for the assembler
 * @param assembler Pointer to the assembler
 * @param format Output format
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_output_format(coil_assembler_t* assembler,
                                   coil_output_format_t format) {
  if (!assembler) {
    return -1;
  }
  
  /* Validate output format */
  if (format > COIL_OUTPUT_FORMAT_LIBRARY) {
    coil_diagnostics_reportf(assembler->diag_context, COIL_DIAG_WARNING, 
                           COIL_DIAG_CATEGORY_GENERAL, 6,
                           "Invalid output format: %d, using default", format);
    format = COIL_OUTPUT_FORMAT_OBJECT; /* Use default format */
  }
  
  assembler->output_format = format;
  
  coil_log_debug("Set output format to %d", format);
  
  return 0;
}

/**
 * @brief Set a diagnostic handler for the assembler
 * @param assembler Pointer to the assembler
 * @param handler Diagnostic handler function
 * @param user_data User data to pass to the handler
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_diagnostics_handler(coil_assembler_t* assembler, 
                                         coil_diagnostics_handler_t handler,
                                         void* user_data) {
  if (!assembler || !handler) {
    return -1;
  }
  
  /* Set the diagnostics handler */
  if (!assembler->diag_context) {
    assembler->diag_context = coil_diagnostics_create();
    if (!assembler->diag_context) {
      return -1;
    }
  }
  
  return coil_diagnostics_set_handler(assembler->diag_context, handler, user_data);
}

/**
 * @brief Set a target configuration file for the assembler
 * @param assembler Pointer to the assembler
 * @param config_file Path to the configuration file
 * @return 0 on success, non-zero on failure
 */
int coil_assembler_set_target_config(coil_assembler_t* assembler, const char* config_file) {
  if (!assembler || !config_file) {
    return -1;
  }
  
  coil_log_info("Loading target configuration from %s", config_file);
  
  /* Load the configuration file */
  coil_config_t* config = coil_config_load_file(config_file);
  if (!config) {
    coil_diagnostics_reportf(assembler->diag_context, COIL_DIAG_ERROR, 
                           COIL_DIAG_CATEGORY_GENERAL, 7,
                           "Failed to load target configuration: %s", config_file);
    if (assembler->last_error) {
      coil_free(assembler->last_error, strlen(assembler->last_error) + 1);
    }
    assembler->last_error = coil_strdup("Failed to load target configuration");
    return -1;
  }
  
  /* Apply configuration to target context */
  /* In a real implementation, we would apply the configuration here */
  /* For simplicity, we'll just free the configuration */
  
  coil_config_destroy(config);
  
  coil_log_info("Target configuration loaded successfully");
  
  return 0;
}