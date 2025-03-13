/**
 * @file coil_assembler.c
 * @brief Implementation of the COIL assembler API
 * 
 * This file implements the public API for the COIL assembler defined in
 * coil_assembler.h, providing functions to translate COIL binary code
 * to native code for various target architectures.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "include/coil_assembler.h"
#include "utils/logging.h"
#include "utils/memory_management.h"
#include "utils/error_handling.h"
#include "core/binary_parser.h"
#include "core/instruction_decoder.h"
#include "core/target_config.h"
#include "core/translator.h"
#include "core/optimization.h"
#include "core/native_generator.h"

/**
 * @brief Version string for the COIL assembler
 */
#define COIL_VERSION "1.0.0"

/**
 * @brief COIL assembler internal structure
 */
typedef struct coil_assembler {
  coil_assembler_config_t config;    /**< Configuration options */
  target_config_t* target;           /**< Target architecture configuration */
  optimization_context_t* optimizer; /**< Optimization context */
  native_format_t output_format;     /**< Output binary format */
} coil_assembler_t;

/**
 * @brief Maps an internal error code to a COIL status code
 *
 * @param[in] error Internal error code
 * @return Corresponding COIL status code
 */
static coil_status_t map_error_to_status(error_t error) {
  switch (error) {
    case ERROR_NONE:
      return COIL_SUCCESS;
    
    case ERROR_INVALID_ARGUMENT:
      return COIL_ERROR_INVALID_ARGS;
    
    case ERROR_FILE_IO:
      return COIL_ERROR_FILE_IO;
    
    case ERROR_MEMORY:
      return COIL_ERROR_MEMORY;
    
    case ERROR_INVALID_FORMAT:
      return COIL_ERROR_INVALID_BINARY;
    
    case ERROR_NOT_FOUND:
      return COIL_ERROR_INVALID_BINARY;
    
    case ERROR_UNSUPPORTED:
      return COIL_ERROR_UNSUPPORTED_INSTR;
    
    case ERROR_CONFIG:
      return COIL_ERROR_CONFIG;
    
    case ERROR_TRANSLATION:
      return COIL_ERROR_TRANSLATION;
    
    case ERROR_OPTIMIZATION:
      return COIL_ERROR_OPTIMIZATION;
    
    case ERROR_GENERATION:
      return COIL_ERROR_GENERATION;
    
    case ERROR_INTERNAL:
    default:
      return COIL_ERROR_UNKNOWN;
  }
}

/**
 * @brief Sets up logging based on verbosity level
 *
 * @param[in] verbose Verbosity level (0-3)
 */
static void setup_logging(uint8_t verbose) {
  /* Map verbosity to log level */
  log_level_t level;
  
  switch (verbose) {
    case 0:
      level = LOG_LEVEL_ERROR;
      break;
    
    case 1:
      level = LOG_LEVEL_WARNING;
      break;
    
    case 2:
      level = LOG_LEVEL_INFO;
      break;
    
    case 3:
    default:
      level = LOG_LEVEL_DEBUG;
      break;
  }
  
  log_init(verbose);
  log_set_level(level);
}

/**
 * @brief Maps optimization level to internal levels
 *
 * @param[in] level COIL optimization level
 * @return Internal optimization level
 */
static uint32_t map_optimization_level(coil_opt_level_t level) {
  switch (level) {
    case COIL_OPT_NONE:
      return 0;
    
    case COIL_OPT_BASIC:
      return 1;
    
    case COIL_OPT_MODERATE:
      return 2;
    
    case COIL_OPT_AGGRESSIVE:
      return 3;
    
    default:
      return 1;  /* Default to basic optimization */
  }
}

/**
 * @brief Determines the output binary format based on the target
 *
 * @param[in] target Target configuration
 * @return Native output format
 */
static native_format_t determine_output_format(const target_config_t* target) {
  /* Choose output format based on architecture and platform */
  /* For now, always use ELF for simplicity */
  return NATIVE_FORMAT_ELF;
}

coil_status_t coil_assembler_create(
  const coil_assembler_config_t* config,
  coil_assembler_handle_t* handle
) {
  if (config == NULL || handle == NULL) {
    return COIL_ERROR_INVALID_ARGS;
  }
  
  *handle = NULL;
  
  /* Initialize subsystems */
  memory_init(true);  /* Enable memory tracking */
  setup_logging(config->verbose);
  
  /* Allocate assembler structure */
  coil_assembler_t* assembler = memory_calloc(1, sizeof(coil_assembler_t));
  if (assembler == NULL) {
    log_error("Failed to allocate memory for assembler");
    return COIL_ERROR_MEMORY;
  }
  
  /* Copy configuration */
  memcpy(&assembler->config, config, sizeof(coil_assembler_config_t));
  
  /* Load target configuration */
  if (config->config_file != NULL && config->config_file[0] != '\0') {
    /* Load from configuration file */
    error_t target_result = target_config_load(config->config_file, &assembler->target);
    if (target_result != ERROR_NONE) {
      log_error("Failed to load target configuration from %s: %s",
               config->config_file, error_message(target_result));
      memory_free(assembler);
      return map_error_to_status(target_result);
    }
  } else if (config->target_name != NULL && config->target_name[0] != '\0') {
    /* Create configuration for specified target */
    /* In a real implementation, we would use the target name to load a built-in config */
    log_error("Built-in target configurations not implemented yet");
    memory_free(assembler);
    return COIL_ERROR_CONFIG;
  } else {
    /* Auto-detect target */
    error_t target_result = target_config_detect_current(&assembler->target);
    if (target_result != ERROR_NONE) {
      log_error("Failed to auto-detect target configuration: %s",
               error_message(target_result));
      memory_free(assembler);
      return map_error_to_status(target_result);
    }
  }
  
  /* Create optimization context */
  error_t opt_result = optimization_create_context(
    assembler->target,
    map_optimization_level(config->opt_level),
    &assembler->optimizer
  );
  
  if (opt_result != ERROR_NONE) {
    log_error("Failed to create optimization context: %s",
             error_message(opt_result));
    target_config_free(assembler->target);
    memory_free(assembler);
    return map_error_to_status(opt_result);
  }
  
  /* Determine output format */
  assembler->output_format = determine_output_format(assembler->target);
  
  log_info("COIL assembler created for target: %s", assembler->target->name);
  
  *handle = assembler;
  return COIL_SUCCESS;
}

coil_status_t coil_assembler_destroy(coil_assembler_handle_t handle) {
  if (handle == NULL) {
    return COIL_ERROR_INVALID_ARGS;
  }
  
  /* Free resources */
  if (handle->optimizer != NULL) {
    optimization_destroy_context(handle->optimizer);
  }
  
  if (handle->target != NULL) {
    target_config_free(handle->target);
  }
  
  memory_free(handle);
  
  return COIL_SUCCESS;
}

coil_status_t coil_assembler_assemble(
  coil_assembler_handle_t handle,
  const uint8_t* coil_binary,
  size_t coil_binary_size,
  uint8_t** native_binary,
  size_t* native_binary_size
) {
  if (handle == NULL || coil_binary == NULL || coil_binary_size == 0) {
    return COIL_ERROR_INVALID_ARGS;
  }
  
  /* Initialize output parameters if provided */
  if (native_binary != NULL) {
    *native_binary = NULL;
  }
  
  if (native_binary_size != NULL) {
    *native_binary_size = 0;
  }
  
  /* Parse COIL binary */
  coil_module_t* module = NULL;
  error_t parse_result = binary_parser_parse(coil_binary, coil_binary_size, &module);
  
  if (parse_result != ERROR_NONE) {
    log_error("Failed to parse COIL binary: %s", error_message(parse_result));
    return map_error_to_status(parse_result);
  }
  
  /* Decode COIL module */
  coil_decoded_module_t* decoded_module = NULL;
  error_t decode_result = instruction_decoder_decode(module, &decoded_module);
  
  if (decode_result != ERROR_NONE) {
    log_error("Failed to decode COIL module: %s", error_message(decode_result));
    binary_parser_free_module(module);
    return map_error_to_status(decode_result);
  }
  
  /* Create translation context */
  translator_context_t* translator_ctx = NULL;
  error_t translator_result = translator_create_context(handle->target, &translator_ctx);
  
  if (translator_result != ERROR_NONE) {
    log_error("Failed to create translator context: %s", error_message(translator_result));
    instruction_decoder_free_module(decoded_module);
    binary_parser_free_module(module);
    return map_error_to_status(translator_result);
  }
  
  /* Create native generator */
  native_generator_t* generator = NULL;
  error_t generator_result = native_generator_create(
    handle->target,
    handle->output_format,
    &generator
  );
  
  if (generator_result != ERROR_NONE) {
    log_error("Failed to create native generator: %s", error_message(generator_result));
    translator_destroy_context(translator_ctx);
    instruction_decoder_free_module(decoded_module);
    binary_parser_free_module(module);
    return map_error_to_status(generator_result);
  }
  
  /* Process each function */
  for (uint32_t i = 0; i < decoded_module->function_count; i++) {
    coil_function_t* function = decoded_module->functions[i];
    
    /* Translate function */
    error_t translate_result = translator_translate_function(function, translator_ctx);
    if (translate_result != ERROR_NONE) {
      log_error("Failed to translate function %s: %s",
               function->name, error_message(translate_result));
      native_generator_destroy(generator);
      translator_destroy_context(translator_ctx);
      instruction_decoder_free_module(decoded_module);
      binary_parser_free_module(module);
      return map_error_to_status(translate_result);
    }
    
    /* Optimize translated instructions */
    error_t optimize_result = optimization_apply_all_passes(
      handle->optimizer,
      translator_ctx->output
    );
    
    if (optimize_result != ERROR_NONE) {
      log_error("Failed to optimize function %s: %s",
               function->name, error_message(optimize_result));
      native_generator_destroy(generator);
      translator_destroy_context(translator_ctx);
      instruction_decoder_free_module(decoded_module);
      binary_parser_free_module(module);
      return map_error_to_status(optimize_result);
    }
    
    /* Add function to native binary */
    uint32_t symbol_index;
    error_t add_result = native_generator_add_function(
      generator,
      function->name,
      translator_ctx->output,
      &symbol_index
    );
    
    if (add_result != ERROR_NONE) {
      log_error("Failed to add function %s to native binary: %s",
               function->name, error_message(add_result));
      native_generator_destroy(generator);
      translator_destroy_context(translator_ctx);
      instruction_decoder_free_module(decoded_module);
      binary_parser_free_module(module);
      return map_error_to_status(add_result);
    }
    
    /* Set entry point if this is the main function */
    if (strcmp(function->name, "main") == 0) {
      native_generator_set_entry_point(generator, symbol_index);
    }
  }
  
  /* Generate native binary if requested */
  if (native_binary != NULL && native_binary_size != NULL) {
    error_t generate_result = native_generator_generate(
      generator,
      native_binary,
      native_binary_size
    );
    
    if (generate_result != ERROR_NONE) {
      log_error("Failed to generate native binary: %s", error_message(generate_result));
      native_generator_destroy(generator);
      translator_destroy_context(translator_ctx);
      instruction_decoder_free_module(decoded_module);
      binary_parser_free_module(module);
      return map_error_to_status(generate_result);
    }
  }
  
  /* Cleanup */
  native_generator_destroy(generator);
  translator_destroy_context(translator_ctx);
  instruction_decoder_free_module(decoded_module);
  binary_parser_free_module(module);
  
  return COIL_SUCCESS;
}

coil_status_t coil_assembler_assemble_file(
  coil_assembler_handle_t handle,
  const char* coil_filename,
  const char* native_filename
) {
  if (handle == NULL || coil_filename == NULL || native_filename == NULL) {
    return COIL_ERROR_INVALID_ARGS;
  }
  
  /* Read input file */
  FILE* input_file = fopen(coil_filename, "rb");
  if (input_file == NULL) {
    log_error("Failed to open input file: %s", coil_filename);
    return COIL_ERROR_FILE_IO;
  }
  
  /* Get file size */
  fseek(input_file, 0, SEEK_END);
  long file_size = ftell(input_file);
  fseek(input_file, 0, SEEK_SET);
  
  if (file_size < 0) {
    log_error("Failed to determine size of input file: %s", coil_filename);
    fclose(input_file);
    return COIL_ERROR_FILE_IO;
  }
  
  /* Allocate buffer */
  uint8_t* buffer = memory_alloc(file_size);
  if (buffer == NULL) {
    log_error("Failed to allocate memory for input file: %s", coil_filename);
    fclose(input_file);
    return COIL_ERROR_MEMORY;
  }
  
  /* Read file data */
  size_t read_size = fread(buffer, 1, file_size, coil_filename);
  fclose(input_file);
  
  if (read_size != (size_t)file_size) {
    log_error("Failed to read input file: %s", coil_filename);
    memory_free(buffer);
    return COIL_ERROR_FILE_IO;
  }
  
  /* Parse COIL binary */
  coil_module_t* module = NULL;
  error_t parse_result = binary_parser_parse(buffer, read_size, &module);
  
  if (parse_result != ERROR_NONE) {
    log_error("Failed to parse COIL binary: %s", error_message(parse_result));
    memory_free(buffer);
    return map_error_to_status(parse_result);
  }
  
  /* Decode COIL module */
  coil_decoded_module_t* decoded_module = NULL;
  error_t decode_result = instruction_decoder_decode(module, &decoded_module);
  
  if (decode_result != ERROR_NONE) {
    log_error("Failed to decode COIL module: %s", error_message(decode_result));
    binary_parser_free_module(module);
    memory_free(buffer);
    return map_error_to_status(decode_result);
  }
  
  /* Create translation context */
  translator_context_t* translator_ctx = NULL;
  error_t translator_result = translator_create_context(handle->target, &translator_ctx);
  
  if (translator_result != ERROR_NONE) {
    log_error("Failed to create translator context: %s", error_message(translator_result));
    instruction_decoder_free_module(decoded_module);
    binary_parser_free_module(module);
    memory_free(buffer);
    return map_error_to_status(translator_result);
  }
  
  /* Create native generator */
  native_generator_t* generator = NULL;
  error_t generator_result = native_generator_create(
    handle->target,
    handle->output_format,
    &generator
  );
  
  if (generator_result != ERROR_NONE) {
    log_error("Failed to create native generator: %s", error_message(generator_result));
    translator_destroy_context(translator_ctx);
    instruction_decoder_free_module(decoded_module);
    binary_parser_free_module(module);
    memory_free(buffer);
    return map_error_to_status(generator_result);
  }
  
  /* Process each function */
  for (uint32_t i = 0; i < decoded_module->function_count; i++) {
    coil_function_t* function = decoded_module->functions[i];
    
    /* Translate function */
    error_t translate_result = translator_translate_function(function, translator_ctx);
    if (translate_result != ERROR_NONE) {
      log_error("Failed to translate function %s: %s",
               function->name, error_message(translate_result));
      native_generator_destroy(generator);
      translator_destroy_context(translator_ctx);
      instruction_decoder_free_module(decoded_module);
      binary_parser_free_module(module);
      memory_free(buffer);
      return map_error_to_status(translate_result);
    }
    
    /* Optimize translated instructions */
    error_t optimize_result = optimization_apply_all_passes(
      handle->optimizer,
      translator_ctx->output
    );
    
    if (optimize_result != ERROR_NONE) {
      log_error("Failed to optimize function %s: %s",
               function->name, error_message(optimize_result));
      native_generator_destroy(generator);
      translator_destroy_context(translator_ctx);
      instruction_decoder_free_module(decoded_module);
      binary_parser_free_module(module);
      memory_free(buffer);
      return map_error_to_status(optimize_result);
    }
    
    /* Add function to native binary */
    uint32_t symbol_index;
    error_t add_result = native_generator_add_function(
      generator,
      function->name,
      translator_ctx->output,
      &symbol_index
    );
    
    if (add_result != ERROR_NONE) {
      log_error("Failed to add function %s to native binary: %s",
               function->name, error_message(add_result));
      native_generator_destroy(generator);
      translator_destroy_context(translator_ctx);
      instruction_decoder_free_module(decoded_module);
      binary_parser_free_module(module);
      memory_free(buffer);
      return map_error_to_status(add_result);
    }
    
    /* Set entry point if this is the main function */
    if (strcmp(function->name, "main") == 0) {
      native_generator_set_entry_point(generator, symbol_index);
    }
  }
  
  /* Write output file */
  error_t write_result = native_generator_write_file(generator, native_filename);
  if (write_result != ERROR_NONE) {
    log_error("Failed to write output file: %s", error_message(write_result));
    native_generator_destroy(generator);
    translator_destroy_context(translator_ctx);
    instruction_decoder_free_module(decoded_module);
    binary_parser_free_module(module);
    memory_free(buffer);
    return map_error_to_status(write_result);
  }
  
  /* Cleanup */
  native_generator_destroy(generator);
  translator_destroy_context(translator_ctx);
  instruction_decoder_free_module(decoded_module);
  binary_parser_free_module(module);
  memory_free(buffer);
  
  return COIL_SUCCESS;
}

const char* coil_status_string(coil_status_t status) {
  switch (status) {
    case COIL_SUCCESS:
      return "Success";
    
    case COIL_ERROR_INVALID_ARGS:
      return "Invalid arguments";
    
    case COIL_ERROR_FILE_IO:
      return "File I/O error";
    
    case COIL_ERROR_MEMORY:
      return "Memory allocation or management error";
    
    case COIL_ERROR_INVALID_BINARY:
      return "Invalid COIL binary format";
    
    case COIL_ERROR_UNSUPPORTED_INSTR:
      return "Unsupported instruction encountered";
    
    case COIL_ERROR_CONFIG:
      return "Configuration error";
    
    case COIL_ERROR_TRANSLATION:
      return "Error during translation";
    
    case COIL_ERROR_OPTIMIZATION:
      return "Error during optimization";
    
    case COIL_ERROR_GENERATION:
      return "Error during native code generation";
    
    case COIL_ERROR_UNKNOWN:
    default:
      return "Unknown error";
  }
}

const char* coil_assembler_version(void) {
  return COIL_VERSION;
}