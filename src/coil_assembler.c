/**
 * @file coil_assembler.c
 * @brief Implementation of COIL Assembler public API.
 */

#include "coil_assembler.h"
#include "coil_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/** 
 * @brief Global error message buffer
 */
static char g_error_message[256] = "";

/**
 * @brief Set error message with formatting
 */
void set_error(coil_assembler_t* assembler, coil_error_t error, const char* format, ...) {
  va_list args;
  va_start(args, format);
  
  if (assembler) {
    assembler->last_error = error;
    vsnprintf(assembler->error_message, sizeof(assembler->error_message), format, args);
    strncpy(g_error_message, assembler->error_message, sizeof(g_error_message) - 1);
  } else {
    vsnprintf(g_error_message, sizeof(g_error_message), format, args);
  }
  
  va_end(args);
}

/**
 * @brief Get the virtual map for a target architecture
 */
static virtual_map_t* get_virtual_map(const char* target_name, coil_device_class_t device_class) {
  // Check for x86 family
  if (strstr(target_name, "x86") || strstr(target_name, "i386") || 
      strstr(target_name, "i686") || strstr(target_name, "amd64")) {
    return initialize_x86_map(device_class);
  }
  // Check for ARM family
  else if (strstr(target_name, "arm") || strstr(target_name, "aarch")) {
    // Will implement in future
    return NULL; 
  }
  // Check for RISC-V family
  else if (strstr(target_name, "riscv")) {
    // Will implement in future
    return NULL;
  }
  // Check for PowerPC family
  else if (strstr(target_name, "ppc") || strstr(target_name, "powerpc")) {
    // Will implement in future
    return NULL;
  }
  
  return NULL;
}

/**
 * @brief Create a default target configuration
 */
static target_config_t* create_default_target_config(const char* target_name, coil_device_class_t device_class) {
  target_config_t* config = (target_config_t*)malloc(sizeof(target_config_t));
  if (!config) {
    return NULL;
  }
  
  memset(config, 0, sizeof(target_config_t));
  
  config->name = strdup(target_name);
  
  // Set defaults based on target name
  if (strstr(target_name, "x86_64")) {
    config->architecture = strdup("x86_64");
    
    // Default x86_64 resources
    config->resources.registers = 16;
    config->resources.vector_width = 128;  // SSE2 default
    config->resources.min_alignment = 16;
    
    // Default optimization parameters
    config->optimization.vector_threshold = 4;
    config->optimization.unroll_factor = 4;
    config->optimization.use_fma = false;
    config->optimization.level = 2;
    
    // Add basic x86_64 features
    const char* default_features[] = {"sse", "sse2", "mmx"};
    config->feature_count = 3;
    config->features = (char**)malloc(config->feature_count * sizeof(char*));
    for (size_t i = 0; i < config->feature_count; i++) {
      config->features[i] = strdup(default_features[i]);
    }
    
    // Add memory models
    const char* memory_models[] = {"strong", "acquire_release", "relaxed"};
    config->resources.memory_model_count = 3;
    config->resources.memory_models = (char**)malloc(config->resources.memory_model_count * sizeof(char*));
    for (size_t i = 0; i < config->resources.memory_model_count; i++) {
      config->resources.memory_models[i] = strdup(memory_models[i]);
    }
  } else if (strstr(target_name, "x86") || strstr(target_name, "i386") || strstr(target_name, "i686")) {
    config->architecture = strdup("x86");
    
    // Default x86 resources
    config->resources.registers = 8;  // 32-bit x86 has 8 general purpose registers
    config->resources.vector_width = 128;  // SSE2
    config->resources.min_alignment = 16;
    
    // Default optimization parameters
    config->optimization.vector_threshold = 4;
    config->optimization.unroll_factor = 4;
    config->optimization.use_fma = false;
    config->optimization.level = 2;
    
    // Add basic x86 features
    const char* default_features[] = {"mmx", "sse", "sse2"};
    config->feature_count = 3;
    config->features = (char**)malloc(config->feature_count * sizeof(char*));
    for (size_t i = 0; i < config->feature_count; i++) {
      config->features[i] = strdup(default_features[i]);
    }
    
    // Add memory models
    const char* memory_models[] = {"strong", "acquire_release", "relaxed"};
    config->resources.memory_model_count = 3;
    config->resources.memory_models = (char**)malloc(config->resources.memory_model_count * sizeof(char*));
    for (size_t i = 0; i < config->resources.memory_model_count; i++) {
      config->resources.memory_models[i] = strdup(memory_models[i]);
    }
  } else {
    // Generic defaults
    config->architecture = strdup("generic");
    config->resources.registers = 8;
    config->resources.vector_width = 0;
    config->resources.min_alignment = 8;
    config->optimization.level = 1;
  }
  
  config->vendor = strdup("generic");
  config->device_class = device_class;
  
  return config;
}

/**
 * @brief Free a target configuration
 */
static void free_target_config(target_config_t* config) {
  if (!config) {
    return;
  }
  
  free(config->name);
  free(config->architecture);
  free(config->vendor);
  
  for (size_t i = 0; i < config->feature_count; i++) {
    free(config->features[i]);
  }
  free(config->features);
  
  for (size_t i = 0; i < config->resources.memory_model_count; i++) {
    free(config->resources.memory_models[i]);
  }
  free(config->resources.memory_models);
  
  free(config);
}

/**
 * @brief Initialize a COIL assembler for the specified target
 */
coil_assembler_handle_t coil_assembler_init(const char* target_name, coil_device_class_t device_class) {
  if (!target_name) {
    set_error(NULL, COIL_ERROR_UNSUPPORTED_TARGET, "No target specified");
    return NULL;
  }
  
  coil_assembler_t* assembler = (coil_assembler_t*)malloc(sizeof(coil_assembler_t));
  if (!assembler) {
    set_error(NULL, COIL_ERROR_OUT_OF_MEMORY, "Failed to allocate assembler");
    return NULL;
  }
  
  memset(assembler, 0, sizeof(coil_assembler_t));
  
  // Create target configuration
  assembler->target = create_default_target_config(target_name, device_class);
  if (!assembler->target) {
    set_error(assembler, COIL_ERROR_OUT_OF_MEMORY, "Failed to create target configuration");
    free(assembler);
    return NULL;
  }
  
  // Get virtual map for target
  assembler->vmap = get_virtual_map(target_name, device_class);
  if (!assembler->vmap) {
    set_error(assembler, COIL_ERROR_UNSUPPORTED_TARGET, 
              "Unsupported target architecture: %s", target_name);
    free_target_config(assembler->target);
    free(assembler);
    return NULL;
  }
  
  // Set default optimization level
  assembler->optimization_level = 2;
  
  return assembler;
}

/**
 * @brief Clean up resources used by a COIL assembler
 */
void coil_assembler_cleanup(coil_assembler_handle_t assembler) {
  if (!assembler) {
    return;
  }
  
  free_target_config(assembler->target);
  free_virtual_map(assembler->vmap);
  free(assembler);
}

/**
 * @brief Load a COIL binary module from a file
 */
coil_module_handle_t coil_module_load(const char* filename) {
  if (!filename) {
    set_error(NULL, COIL_ERROR_FILE_NOT_FOUND, "No filename specified");
    return NULL;
  }
  
  FILE* file = fopen(filename, "rb");
  if (!file) {
    set_error(NULL, COIL_ERROR_FILE_NOT_FOUND, "Failed to open file: %s", filename);
    return NULL;
  }
  
  // Get file size
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  // Read entire file
  uint8_t* file_data = (uint8_t*)malloc(file_size);
  if (!file_data) {
    set_error(NULL, COIL_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for file data");
    fclose(file);
    return NULL;
  }
  
  size_t bytes_read = fread(file_data, 1, file_size, file);
  fclose(file);
  
  if (bytes_read != file_size) {
    set_error(NULL, COIL_ERROR_INVALID_FORMAT, "Failed to read entire file");
    free(file_data);
    return NULL;
  }
  
  // Check file header
  if (file_size < sizeof(coil_header_t)) {
    set_error(NULL, COIL_ERROR_INVALID_FORMAT, "File too small to be a valid COIL binary");
    free(file_data);
    return NULL;
  }
  
  coil_header_t* header = (coil_header_t*)file_data;
  if (header->magic != 0x434F494C) {  // "COIL" in ASCII
    set_error(NULL, COIL_ERROR_INVALID_FORMAT, "Invalid COIL magic number");
    free(file_data);
    return NULL;
  }
  
  // Allocate module
  coil_module_t* module = (coil_module_t*)malloc(sizeof(coil_module_t));
  if (!module) {
    set_error(NULL, COIL_ERROR_OUT_OF_MEMORY, "Failed to allocate module");
    free(file_data);
    return NULL;
  }
  
  memset(module, 0, sizeof(coil_module_t));
  
  // Extract module name from filename
  const char* module_name = strrchr(filename, '/');
  if (!module_name) {
    module_name = strrchr(filename, '\\');
  }
  
  if (module_name) {
    module_name++;  // Skip the slash
  } else {
    module_name = filename;
  }
  
  module->name = strdup(module_name);
  
  // Process sections
  size_t section_table_offset = sizeof(coil_header_t);
  section_entry_t* section_table = (section_entry_t*)(file_data + section_table_offset);
  
  for (uint32_t i = 0; i < header->section_count; i++) {
    if (section_table_offset + (i + 1) * sizeof(section_entry_t) > file_size) {
      set_error(NULL, COIL_ERROR_INVALID_FORMAT, "Section table extends beyond file");
      coil_module_free(module);
      free(file_data);
      return NULL;
    }
    
    section_entry_t* entry = &section_table[i];
    
    if (entry->offset + entry->size > file_size) {
      set_error(NULL, COIL_ERROR_INVALID_FORMAT, "Section data extends beyond file");
      coil_module_free(module);
      free(file_data);
      return NULL;
    }
    
    uint8_t* section_data = file_data + entry->offset;
    coil_error_t error;
    
    switch (entry->section_type) {
      case SECTION_TYPE_TYPE:
        error = process_type_section(module, section_data, entry->size);
        break;
      case SECTION_TYPE_FUNCTION:
        error = process_function_section(module, section_data, entry->size);
        break;
      case SECTION_TYPE_GLOBAL:
        error = process_global_section(module, section_data, entry->size);
        break;
      case SECTION_TYPE_CONSTANT:
        error = process_constant_section(module, section_data, entry->size);
        break;
      case SECTION_TYPE_CODE:
        // Store code section for later processing
        module->code_section = (uint8_t*)malloc(entry->size);
        if (!module->code_section) {
          set_error(NULL, COIL_ERROR_OUT_OF_MEMORY, "Failed to allocate code section");
          coil_module_free(module);
          free(file_data);
          return NULL;
        }
        memcpy(module->code_section, section_data, entry->size);
        module->code_size = entry->size;
        error = COIL_SUCCESS;
        break;
      case SECTION_TYPE_RELOCATION:
        error = process_relocation_section(module, section_data, entry->size);
        break;
      case SECTION_TYPE_METADATA:
        error = process_metadata_section(module, section_data, entry->size);
        break;
      default:
        // Skip unknown sections
        error = COIL_SUCCESS;
        break;
    }
    
    if (error != COIL_SUCCESS) {
      coil_module_free(module);
      free(file_data);
      return NULL;
    }
  }
  
  // Process code section now that all types and functions are known
  if (module->code_section) {
    coil_error_t error = process_code_section(module, module->code_section, module->code_size);
    if (error != COIL_SUCCESS) {
      coil_module_free(module);
      free(file_data);
      return NULL;
    }
  }
  
  free(file_data);
  return module;
}

/**
 * @brief Free resources associated with a COIL module
 */
void coil_module_free(coil_module_handle_t module) {
  if (!module) {
    return;
  }
  
  free(module->name);
  
  // Free functions
  for (size_t i = 0; i < module->function_count; i++) {
    coil_function_t* func = &module->functions[i];
    free(func->name);
    free(func->param_types);
    
    // Free basic blocks
    coil_basic_block_t* block = func->blocks;
    while (block) {
      coil_basic_block_t* next = block->next;
      
      free(block->label);
      
      // Free instructions
      for (size_t j = 0; j < block->instr_count; j++) {
        free(block->instructions[j].operands);
      }
      free(block->instructions);
      
      free(block);
      block = next;
    }
  }
  free(module->functions);
  
  // Free globals
  for (size_t i = 0; i < module->global_count; i++) {
    free(module->globals[i].name);
    free(module->globals[i].initializer);
  }
  free(module->globals);
  
  // Free types
  for (size_t i = 0; i < module->type_count; i++) {
    free(module->types[i].name);
  }
  free(module->types);
  
  free(module->code_section);
  free(module);
}

/**
 * @brief Assemble a COIL module to native code
 */
coil_error_t coil_assemble(coil_assembler_handle_t assembler, 
                          coil_module_handle_t module,
                          const char* output_filename) {
  if (!assembler) {
    return COIL_ERROR_UNKNOWN;
  }
  
  if (!module) {
    set_error(assembler, COIL_ERROR_INVALID_FORMAT, "No module provided");
    return COIL_ERROR_INVALID_FORMAT;
  }
  
  if (!output_filename) {
    set_error(assembler, COIL_ERROR_FILE_NOT_FOUND, "No output filename provided");
    return COIL_ERROR_FILE_NOT_FOUND;
  }
  
  // Create translation context
  translation_context_t* ctx = create_translation_context(module, assembler->target, assembler->vmap);
  if (!ctx) {
    set_error(assembler, COIL_ERROR_OUT_OF_MEMORY, "Failed to create translation context");
    return COIL_ERROR_OUT_OF_MEMORY;
  }
  
  // Translate module to native code
  coil_error_t error = translate_module(ctx);
  if (error != COIL_SUCCESS) {
    free_translation_context(ctx);
    return error;
  }
  
  // Generate output file
  error = generate_native_code(ctx, output_filename);
  
  // Free translation context
  free_translation_context(ctx);
  
  return error;
}

/**
 * @brief Get the last error message
 */
const char* coil_get_error_message(void) {
  return g_error_message;
}

/**
 * @brief Create a custom target configuration
 */
target_config_handle_t coil_target_config_create(const char* architecture,
                                              const char* vendor,
                                              coil_device_class_t device_class) {
  if (!architecture) {
    set_error(NULL, COIL_ERROR_UNSUPPORTED_TARGET, "No architecture specified");
    return NULL;
  }
  
  target_config_t* config = (target_config_t*)malloc(sizeof(target_config_t));
  if (!config) {
    set_error(NULL, COIL_ERROR_OUT_OF_MEMORY, "Failed to allocate target configuration");
    return NULL;
  }
  
  memset(config, 0, sizeof(target_config_t));
  
  config->name = strdup(architecture);
  config->architecture = strdup(architecture);
  config->vendor = vendor ? strdup(vendor) : strdup("generic");
  config->device_class = device_class;
  
  // Default resources
  if (strcmp(architecture, "x86_64") == 0) {
    config->resources.registers = 16;
  } else if (strcmp(architecture, "x86") == 0) {
    config->resources.registers = 8;
  } else if (strstr(architecture, "arm") || strstr(architecture, "aarch")) {
    config->resources.registers = 16;
  } else if (strstr(architecture, "riscv")) {
    config->resources.registers = 32;
  } else {
    config->resources.registers = 8;  // Conservative default
  }
  
  // Default optimization
  config->optimization.level = 2;
  config->optimization.unroll_factor = 4;
  
  return config;
}

/**
 * @brief Add a feature to a target configuration
 */
coil_error_t coil_target_config_add_feature(target_config_handle_t config,
                                         const char* feature_name) {
  if (!config) {
    set_error(NULL, COIL_ERROR_UNKNOWN, "Invalid target configuration");
    return COIL_ERROR_UNKNOWN;
  }
  
  if (!feature_name) {
    set_error(NULL, COIL_ERROR_UNKNOWN, "No feature name specified");
    return COIL_ERROR_UNKNOWN;
  }
  
  // Allocate more space for features
  char** new_features = (char**)realloc(config->features, 
                                        (config->feature_count + 1) * sizeof(char*));
  if (!new_features) {
    set_error(NULL, COIL_ERROR_OUT_OF_MEMORY, "Failed to allocate feature array");
    return COIL_ERROR_OUT_OF_MEMORY;
  }
  
  config->features = new_features;
  config->features[config->feature_count] = strdup(feature_name);
  if (!config->features[config->feature_count]) {
    set_error(NULL, COIL_ERROR_OUT_OF_MEMORY, "Failed to allocate feature name");
    return COIL_ERROR_OUT_OF_MEMORY;
  }
  
  config->feature_count++;
  
  // Update configuration based on feature
  if (strcmp(feature_name, "avx") == 0) {
    config->resources.vector_width = 256;
    config->resources.min_alignment = 32;
  } else if (strcmp(feature_name, "avx2") == 0) {
    config->resources.vector_width = 256;
    config->resources.min_alignment = 32;
  } else if (strcmp(feature_name, "avx512") == 0) {
    config->resources.vector_width = 512;
    config->resources.min_alignment = 64;
  } else if (strcmp(feature_name, "fma") == 0) {
    config->optimization.use_fma = true;
  }
  
  return COIL_SUCCESS;
}

/**
 * @brief Free resources associated with a target configuration
 */
void coil_target_config_free(target_config_handle_t config) {
  free_target_config(config);
}

/**
 * @brief Set optimization level for code generation
 */
coil_error_t coil_set_optimization_level(coil_assembler_handle_t assembler,
                                      int level) {
  if (!assembler) {
    set_error(NULL, COIL_ERROR_UNKNOWN, "Invalid assembler handle");
    return COIL_ERROR_UNKNOWN;
  }
  
  if (level < 0 || level > 3) {
    set_error(assembler, COIL_ERROR_UNKNOWN, "Invalid optimization level: %d", level);
    return COIL_ERROR_UNKNOWN;
  }
  
  assembler->optimization_level = level;
  assembler->target->optimization.level = level;
  
  return COIL_SUCCESS;
}