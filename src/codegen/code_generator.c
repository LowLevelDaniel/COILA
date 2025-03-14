/**
 * @file code_generator.c
 * @brief Core code generator implementation
 * @details Implementation of the code generator component for the COIL assembler.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include "coil-assembler/target.h"
#include "coil-assembler/diagnostics.h"
#include "../utils/memory.c"

/**
 * @brief Code generator context
 */
typedef struct {
  coil_target_context_t *target_context;  /**< Target context */
  void *output_buffer;                    /**< Output buffer */
  size_t output_buffer_size;              /**< Size of output buffer */
  size_t output_offset;                   /**< Current offset in output buffer */
  uint32_t error_count;                   /**< Number of errors encountered */
  coil_diagnostics_context_t *diag_context; /**< Diagnostics context */
  
  /* Code generation options */
  coil_output_format_t output_format;     /**< Output format */
  uint32_t optimization_level;            /**< Optimization level */
  bool generate_debug_info;               /**< Whether to generate debug info */
  
  /* Symbol information */
  char **symbol_names;                    /**< Symbol names */
  uint32_t *symbol_offsets;               /**< Symbol offsets */
  uint32_t symbol_count;                  /**< Number of symbols */
  
  /* Relocation information */
  uint32_t *reloc_offsets;                /**< Relocation offsets */
  uint32_t *reloc_targets;                /**< Relocation targets */
  uint32_t *reloc_types;                  /**< Relocation types */
  uint32_t reloc_count;                   /**< Number of relocations */
} code_generator_context_t;

/**
 * @brief Relocation types
 */
typedef enum {
  COIL_RELOC_ABSOLUTE = 0,   /**< Absolute address */
  COIL_RELOC_RELATIVE = 1,   /**< PC-relative address */
  COIL_RELOC_GOT = 2,        /**< Global offset table */
  COIL_RELOC_PLT = 3         /**< Procedure linkage table */
} coil_relocation_type_t;

/**
 * @brief Initialize the code generator
 * @param target_context Target context
 * @param diag_context Diagnostics context (can be NULL)
 * @return New code generator context or NULL on failure
 */
code_generator_context_t* code_generator_create(
    coil_target_context_t *target_context,
    coil_diagnostics_context_t *diag_context) {
  
  if (!target_context) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            1, "NULL target context");
    }
    return NULL;
  }
  
  code_generator_context_t *ctx = (code_generator_context_t*)coil_calloc(
      1, sizeof(code_generator_context_t));
  
  if (!ctx) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            2, "Failed to allocate code generator context");
    }
    return NULL;
  }
  
  // Initialize context
  ctx->target_context = target_context;
  ctx->diag_context = diag_context;
  ctx->output_format = COIL_OUTPUT_FORMAT_OBJECT;
  ctx->optimization_level = 2; // Default optimization level
  
  // Allocate initial output buffer (can be expanded later)
  const size_t initial_buffer_size = 65536; // 64KB initial size
  ctx->output_buffer = coil_malloc(initial_buffer_size);
  
  if (!ctx->output_buffer) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            3, "Failed to allocate output buffer");
    }
    coil_free(ctx, sizeof(code_generator_context_t));
    return NULL;
  }
  
  ctx->output_buffer_size = initial_buffer_size;
  ctx->output_offset = 0;
  
  // Allocate initial symbol and relocation tables
  const uint32_t initial_table_size = 256;
  
  ctx->symbol_names = (char**)coil_calloc(initial_table_size, sizeof(char*));
  ctx->symbol_offsets = (uint32_t*)coil_calloc(initial_table_size, sizeof(uint32_t));
  
  ctx->reloc_offsets = (uint32_t*)coil_calloc(initial_table_size, sizeof(uint32_t));
  ctx->reloc_targets = (uint32_t*)coil_calloc(initial_table_size, sizeof(uint32_t));
  ctx->reloc_types = (uint32_t*)coil_calloc(initial_table_size, sizeof(uint32_t));
  
  if (!ctx->symbol_names || !ctx->symbol_offsets || 
      !ctx->reloc_offsets || !ctx->reloc_targets || !ctx->reloc_types) {
    // Clean up all allocations
    if (ctx->symbol_names) coil_free(ctx->symbol_names, initial_table_size * sizeof(char*));
    if (ctx->symbol_offsets) coil_free(ctx->symbol_offsets, initial_table_size * sizeof(uint32_t));
    if (ctx->reloc_offsets) coil_free(ctx->reloc_offsets, initial_table_size * sizeof(uint32_t));
    if (ctx->reloc_targets) coil_free(ctx->reloc_targets, initial_table_size * sizeof(uint32_t));
    if (ctx->reloc_types) coil_free(ctx->reloc_types, initial_table_size * sizeof(uint32_t));
    coil_free(ctx->output_buffer, initial_buffer_size);
    coil_free(ctx, sizeof(code_generator_context_t));
    
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            4, "Failed to allocate symbol or relocation tables");
    }
    return NULL;
  }
  
  return ctx;
}

/**
 * @brief Destroy the code generator and free resources
 * @param ctx Code generator context
 */
void code_generator_destroy(code_generator_context_t *ctx) {
  if (!ctx) {
    return;
  }
  
  // Free symbol names
  for (uint32_t i = 0; i < ctx->symbol_count; i++) {
    if (ctx->symbol_names[i]) {
      coil_free(ctx->symbol_names[i], strlen(ctx->symbol_names[i]) + 1);
    }
  }
  
  // Free all allocated resources
  if (ctx->symbol_names) coil_free(ctx->symbol_names, 256 * sizeof(char*));
  if (ctx->symbol_offsets) coil_free(ctx->symbol_offsets, 256 * sizeof(uint32_t));
  if (ctx->reloc_offsets) coil_free(ctx->reloc_offsets, 256 * sizeof(uint32_t));
  if (ctx->reloc_targets) coil_free(ctx->reloc_targets, 256 * sizeof(uint32_t));
  if (ctx->reloc_types) coil_free(ctx->reloc_types, 256 * sizeof(uint32_t));
  if (ctx->output_buffer) coil_free(ctx->output_buffer, ctx->output_buffer_size);
  
  coil_free(ctx, sizeof(code_generator_context_t));
}

/**
 * @brief Ensure the output buffer has enough space
 * @param ctx Code generator context
 * @param size Required additional space
 * @return 0 on success, non-zero on failure
 */
static int ensure_output_buffer_space(code_generator_context_t *ctx, size_t size) {
  if (!ctx) {
    return -1;
  }
  
  // Check if we need to expand
  if (ctx->output_offset + size > ctx->output_buffer_size) {
    // Calculate new size (double current size or add what we need, whichever is larger)
    size_t new_size = ctx->output_buffer_size * 2;
    if (new_size < ctx->output_offset + size) {
      new_size = ctx->output_offset + size;
    }
    
    // Reallocate buffer
    void *new_buffer = coil_realloc(ctx->output_buffer, ctx->output_buffer_size, new_size);
    if (!new_buffer) {
      if (ctx->diag_context) {
        coil_diagnostics_report(ctx->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              5, "Failed to resize output buffer");
      }
      return -1;
    }
    
    ctx->output_buffer = new_buffer;
    ctx->output_buffer_size = new_size;
  }
  
  return 0;
}

/**
 * @brief Write data to the output buffer
 * @param ctx Code generator context
 * @param data Data to write
 * @param size Size of data in bytes
 * @return 0 on success, non-zero on failure
 */
int code_generator_write(code_generator_context_t *ctx, const void *data, size_t size) {
  if (!ctx || !data || size == 0) {
    return -1;
  }
  
  // Ensure buffer has enough space
  if (ensure_output_buffer_space(ctx, size) != 0) {
    return -1;
  }
  
  // Write data to buffer
  memcpy((uint8_t*)ctx->output_buffer + ctx->output_offset, data, size);
  ctx->output_offset += size;
  
  return 0;
}

/**
 * @brief Add a symbol to the symbol table
 * @param ctx Code generator context
 * @param name Symbol name
 * @param offset Offset in the output buffer
 * @return Symbol index or -1 on failure
 */
int code_generator_add_symbol(code_generator_context_t *ctx, const char *name, uint32_t offset) {
  if (!ctx || !name) {
    return -1;
  }
  
  // Check if we have space for another symbol
  if (ctx->symbol_count >= 256) {
    if (ctx->diag_context) {
      coil_diagnostics_report(ctx->diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            6, "Symbol table is full");
    }
    return -1;
  }
  
  // Duplicate the symbol name
  char *name_copy = coil_strdup(name);
  if (!name_copy) {
    if (ctx->diag_context) {
      coil_diagnostics_report(ctx->diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            7, "Failed to allocate memory for symbol name");
    }
    return -1;
  }
  
  // Add symbol to table
  ctx->symbol_names[ctx->symbol_count] = name_copy;
  ctx->symbol_offsets[ctx->symbol_count] = offset;
  
  return ctx->symbol_count++;
}

/**
 * @brief Add a relocation to the relocation table
 * @param ctx Code generator context
 * @param offset Offset in the output buffer where relocation should be applied
 * @param target Target symbol index
 * @param type Relocation type
 * @return Relocation index or -1 on failure
 */
int code_generator_add_relocation(code_generator_context_t *ctx, 
                                uint32_t offset, 
                                uint32_t target, 
                                coil_relocation_type_t type) {
  if (!ctx) {
    return -1;
  }
  
  // Check if we have space for another relocation
  if (ctx->reloc_count >= 256) {
    if (ctx->diag_context) {
      coil_diagnostics_report(ctx->diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            8, "Relocation table is full");
    }
    return -1;
  }
  
  // Add relocation to table
  ctx->reloc_offsets[ctx->reloc_count] = offset;
  ctx->reloc_targets[ctx->reloc_count] = target;
  ctx->reloc_types[ctx->reloc_count] = type;
  
  return ctx->reloc_count++;
}

/**
 * @brief Set code generator options
 * @param ctx Code generator context
 * @param output_format Output format
 * @param optimization_level Optimization level
 * @param generate_debug_info Whether to generate debug info
 * @return 0 on success, non-zero on failure
 */
int code_generator_set_options(code_generator_context_t *ctx,
                             coil_output_format_t output_format,
                             uint32_t optimization_level,
                             bool generate_debug_info) {
  if (!ctx) {
    return -1;
  }
  
  ctx->output_format = output_format;
  ctx->optimization_level = optimization_level;
  ctx->generate_debug_info = generate_debug_info;
  
  return 0;
}

/**
 * @brief Generate code for a function
 * @param ctx Code generator context
 * @param function COIL function to generate code for
 * @return 0 on success, non-zero on failure
 */
int code_generator_generate_function(code_generator_context_t *ctx, 
                                    coil_function_t *function) {
  if (!ctx || !function) {
    return -1;
  }
  
  // Add function symbol
  int symbol_index = code_generator_add_symbol(ctx, function->name, ctx->output_offset);
  if (symbol_index < 0) {
    if (ctx->diag_context) {
      coil_diagnostics_reportf(ctx->diag_context, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_GENERATOR,
                             9, "Failed to add symbol for function '%s'", 
                             function->name);
    }
    return -1;
  }
  
  // Call the target-specific function generator
  const coil_target_descriptor_t *target = 
      coil_target_get_descriptor(ctx->target_context);
  
  if (!target || !target->generate_function) {
    if (ctx->diag_context) {
      coil_diagnostics_report(ctx->diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            10, "Target does not support function generation");
    }
    return -1;
  }
  
  return target->generate_function(ctx->target_context, function);
}

/**
 * @brief Get the generated code buffer
 * @param ctx Code generator context
 * @param size Pointer to store the size of the generated code
 * @return Pointer to the generated code buffer or NULL on failure
 */
const void* code_generator_get_buffer(code_generator_context_t *ctx, size_t *size) {
  if (!ctx || !size) {
    return NULL;
  }
  
  *size = ctx->output_offset;
  return ctx->output_buffer;
}

/**
 * @brief Generate object file header
 * @param ctx Code generator context
 * @return 0 on success, non-zero on failure
 */
int code_generator_generate_object_header(code_generator_context_t *ctx) {
  if (!ctx) {
    return -1;
  }
  
  // Object file header generation would depend on the specific object format
  // (ELF, COFF, Mach-O, etc.). This is just a simple placeholder that writes
  // a basic header with a magic number.
  
  // Example: Write a simple header with a magic number "COIL"
  const uint32_t magic = 0x434F494C; // "COIL" in ASCII
  const uint32_t version = 0x00000100; // Version 1.0
  const uint32_t timestamp = 0x12345678; // Placeholder timestamp
  
  // Ensure buffer has enough space
  if (ensure_output_buffer_space(ctx, 12) != 0) {
    return -1;
  }
  
  // Write header to buffer
  uint32_t *header = (uint32_t*)((uint8_t*)ctx->output_buffer + ctx->output_offset);
  header[0] = magic;
  header[1] = version;
  header[2] = timestamp;
  
  ctx->output_offset += 12;
  
  return 0;
}

/**
 * @brief Generate symbol table
 * @param ctx Code generator context
 * @return 0 on success, non-zero on failure
 */
int code_generator_generate_symbol_table(code_generator_context_t *ctx) {
  if (!ctx) {
    return -1;
  }
  
  // Symbol table generation would depend on the specific object format.
  // This is just a simple placeholder that writes symbol names and offsets.
  
  // Calculate required space
  size_t required_size = 4; // Symbol count (uint32_t)
  for (uint32_t i = 0; i < ctx->symbol_count; i++) {
    required_size += 4; // Symbol name length (uint32_t)
    required_size += strlen(ctx->symbol_names[i]) + 1; // Symbol name (null-terminated)
    required_size += 4; // Symbol offset (uint32_t)
  }
  
  // Ensure buffer has enough space
  if (ensure_output_buffer_space(ctx, required_size) != 0) {
    return -1;
  }
  
  // Get current offset
  uint8_t *buffer = (uint8_t*)ctx->output_buffer + ctx->output_offset;
  size_t offset = 0;
  
  // Write symbol count
  *(uint32_t*)(buffer + offset) = ctx->symbol_count;
  offset += 4;
  
  // Write symbols
  for (uint32_t i = 0; i < ctx->symbol_count; i++) {
    // Write symbol name length
    *(uint32_t*)(buffer + offset) = (uint32_t)strlen(ctx->symbol_names[i]);
    offset += 4;
    
    // Write symbol name
    strcpy((char*)(buffer + offset), ctx->symbol_names[i]);
    offset += strlen(ctx->symbol_names[i]) + 1;
    
    // Write symbol offset
    *(uint32_t*)(buffer + offset) = ctx->symbol_offsets[i];
    offset += 4;
  }
  
  ctx->output_offset += offset;
  
  return 0;
}

/**
 * @brief Generate relocation table
 * @param ctx Code generator context
 * @return 0 on success, non-zero on failure
 */
int code_generator_generate_relocation_table(code_generator_context_t *ctx) {
  if (!ctx) {
    return -1;
  }
  
  // Relocation table generation would depend on the specific object format.
  // This is just a simple placeholder that writes relocation entries.
  
  // Calculate required space
  size_t required_size = 4; // Relocation count (uint32_t)
  required_size += ctx->reloc_count * 12; // 3 uint32_t per relocation
  
  // Ensure buffer has enough space
  if (ensure_output_buffer_space(ctx, required_size) != 0) {
    return -1;
  }
  
  // Get current offset
  uint8_t *buffer = (uint8_t*)ctx->output_buffer + ctx->output_offset;
  size_t offset = 0;
  
  // Write relocation count
  *(uint32_t*)(buffer + offset) = ctx->reloc_count;
  offset += 4;
  
  // Write relocations
  for (uint32_t i = 0; i < ctx->reloc_count; i++) {
    // Write relocation offset
    *(uint32_t*)(buffer + offset) = ctx->reloc_offsets[i];
    offset += 4;
    
    // Write relocation target
    *(uint32_t*)(buffer + offset) = ctx->reloc_targets[i];
    offset += 4;
    
    // Write relocation type
    *(uint32_t*)(buffer + offset) = ctx->reloc_types[i];
    offset += 4;
  }
  
  ctx->output_offset += offset;
  
  return 0;
}

/**
 * @brief Finalize code generation
 * @param ctx Code generator context
 * @return 0 on success, non-zero on failure
 */
int code_generator_finalize(code_generator_context_t *ctx) {
  if (!ctx) {
    return -1;
  }
  
  // Generate appropriate output based on the format
  switch (ctx->output_format) {
    case COIL_OUTPUT_FORMAT_OBJECT:
      // Generate object file with header, code, symbols, and relocations
      if (code_generator_generate_object_header(ctx) != 0 ||
          code_generator_generate_symbol_table(ctx) != 0 ||
          code_generator_generate_relocation_table(ctx) != 0) {
        return -1;
      }
      break;
      
    case COIL_OUTPUT_FORMAT_ASSEMBLY:
      // Assembly output would be generated during function generation
      // Nothing special to do here
      break;
      
    case COIL_OUTPUT_FORMAT_EXECUTABLE:
    case COIL_OUTPUT_FORMAT_LIBRARY:
      // These formats would require additional linking steps
      if (ctx->diag_context) {
        coil_diagnostics_report(ctx->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              11, "Executable and library output formats not yet implemented");
      }
      return -1;
  }
  
  return 0;
}

/**
 * @brief Get the number of generated symbols
 * @param ctx Code generator context
 * @return Number of symbols or 0 on failure
 */
uint32_t code_generator_get_symbol_count(code_generator_context_t *ctx) {
  if (!ctx) {
    return 0;
  }
  
  return ctx->symbol_count;
}

/**
 * @brief Get the number of generated relocations
 * @param ctx Code generator context
 * @return Number of relocations or 0 on failure
 */
uint32_t code_generator_get_relocation_count(code_generator_context_t *ctx) {
  if (!ctx) {
    return 0;
  }
  
  return ctx->reloc_count;
}

/**
 * @brief Get the error count
 * @param ctx Code generator context
 * @return Number of errors encountered or 0 on failure
 */
uint32_t code_generator_get_error_count(code_generator_context_t *ctx) {
  if (!ctx) {
    return 0;
  }
  
  return ctx->error_count;
}