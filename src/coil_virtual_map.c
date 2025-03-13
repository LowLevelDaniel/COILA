/**
 * @file coil_virtual_map.c
 * @brief Implementation of the virtual instruction mapping system.
 */

#include "coil_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a virtual instruction map for a target architecture
 */
virtual_map_t* create_virtual_map(const char* arch_name, coil_device_class_t device_class) {
  if (!arch_name) {
    return NULL;
  }
  
  virtual_map_t* map = (virtual_map_t*)malloc(sizeof(virtual_map_t));
  if (!map) {
    return NULL;
  }
  
  memset(map, 0, sizeof(virtual_map_t));
  
  map->arch_name = strdup(arch_name);
  map->device_class = device_class;
  
  // Set default output format based on platform
  if (strstr(arch_name, "x86") || strstr(arch_name, "amd64")) {
    // Use platform-specific formats
    #if defined(_WIN32)
      map->default_format = OUTPUT_FORMAT_COFF;
    #elif defined(__APPLE__)
      map->default_format = OUTPUT_FORMAT_MACHO;
    #else
      map->default_format = OUTPUT_FORMAT_ELF;
    #endif
  } else {
    // Default to ELF for most targets
    map->default_format = OUTPUT_FORMAT_ELF;
  }
  
  return map;
}

/**
 * @brief Add an instruction mapping to a virtual map
 */
coil_error_t add_instruction_mapping(virtual_map_t* map, uint8_t coil_opcode, 
                                   const char* native_mnemonic, 
                                   translate_func_t translate) {
  if (!map || !native_mnemonic || !translate) {
    return COIL_ERROR_UNKNOWN;
  }
  
  // Check if we already have a mapping for this opcode
  for (size_t i = 0; i < map->mapping_count; i++) {
    if (map->mappings[i].coil_opcode == coil_opcode) {
      // Update existing mapping
      free(map->mappings[i].native_mnemonic);
      map->mappings[i].native_mnemonic = strdup(native_mnemonic);
      map->mappings[i].translate = translate;
      return COIL_SUCCESS;
    }
  }
  
  // Allocate more space for mappings
  instr_mapping_t* new_mappings = (instr_mapping_t*)realloc(
    map->mappings, (map->mapping_count + 1) * sizeof(instr_mapping_t));
  
  if (!new_mappings) {
    return COIL_ERROR_OUT_OF_MEMORY;
  }
  
  map->mappings = new_mappings;
  
  // Add new mapping
  map->mappings[map->mapping_count].coil_opcode = coil_opcode;
  map->mappings[map->mapping_count].native_mnemonic = strdup(native_mnemonic);
  map->mappings[map->mapping_count].translate = translate;
  map->mapping_count++;
  
  return COIL_SUCCESS;
}

/**
 * @brief Free a virtual instruction map
 */
void free_virtual_map(virtual_map_t* map) {
  if (!map) {
    return;
  }
  
  free(map->arch_name);
  
  for (size_t i = 0; i < map->mapping_count; i++) {
    free(map->mappings[i].native_mnemonic);
  }
  
  free(map->mappings);
  free(map);
}

/**
 * @brief Find a mapping for a COIL opcode
 */
static instr_mapping_t* find_mapping(virtual_map_t* map, uint8_t coil_opcode) {
  if (!map) {
    return NULL;
  }
  
  for (size_t i = 0; i < map->mapping_count; i++) {
    if (map->mappings[i].coil_opcode == coil_opcode) {
      return &map->mappings[i];
    }
  }
  
  return NULL;
}

/**
 * @brief Translate a COIL instruction to native code
 */
coil_error_t translate_instruction(translation_context_t* ctx, coil_instruction_t* instr) {
  if (!ctx || !instr) {
    return COIL_ERROR_UNKNOWN;
  }
  
  instr_mapping_t* mapping = find_mapping(ctx->vmap, instr->opcode);
  if (!mapping) {
    return COIL_ERROR_UNSUPPORTED_FEATURE;
  }
  
  // Call the architecture-specific translation function
  mapping->translate(instr, ctx);
  
  return COIL_SUCCESS;
}

/**
 * @brief Create a translation context
 */
translation_context_t* create_translation_context(coil_module_t* module, 
                                              target_config_t* target,
                                              virtual_map_t* vmap) {
  if (!module || !target || !vmap) {
    return NULL;
  }
  
  translation_context_t* ctx = (translation_context_t*)malloc(sizeof(translation_context_t));
  if (!ctx) {
    return NULL;
  }
  
  memset(ctx, 0, sizeof(translation_context_t));
  
  ctx->module = module;
  ctx->target = target;
  ctx->vmap = vmap;
  ctx->output_format = vmap->default_format;
  
  // Initialize register allocator
  if (initialize_register_allocator(&ctx->reg_alloc, target->resources.registers) != COIL_SUCCESS) {
    free(ctx);
    return NULL;
  }
  
  // Allocate initial code buffer (1MB)
  const size_t initial_code_capacity = 1024 * 1024;
  ctx->native_code = malloc(initial_code_capacity);
  if (!ctx->native_code) {
    free_register_allocator(&ctx->reg_alloc);
    free(ctx);
    return NULL;
  }
  
  ctx->code_capacity = initial_code_capacity;
  ctx->code_size = 0;
  
  return ctx;
}

/**
 * @brief Translate a module to native code
 */
coil_error_t translate_module(translation_context_t* ctx) {
  if (!ctx) {
    return COIL_ERROR_UNKNOWN;
  }
  
  coil_module_t* module = ctx->module;
  
  // Translate each function
  for (size_t i = 0; i < module->function_count; i++) {
    coil_function_t* func = &module->functions[i];
    
    // Process each basic block
    coil_basic_block_t* block = func->blocks;
    while (block) {
      // Process each instruction in the block
      for (size_t j = 0; j < block->instr_count; j++) {
        coil_error_t error = translate_instruction(ctx, &block->instructions[j]);
        if (error != COIL_SUCCESS) {
          return error;
        }
      }
      
      block = block->next;
    }
  }
  
  return COIL_SUCCESS;
}

/**
 * @brief Generate native code in the appropriate output format
 */
coil_error_t generate_native_code(translation_context_t* ctx, const char* output_filename) {
  if (!ctx || !output_filename) {
    return COIL_ERROR_UNKNOWN;
  }
  
  // This function would handle the specific output format details
  // For simplicity, we'll just dump the raw binary in this implementation
  
  FILE* file = fopen(output_filename, "wb");
  if (!file) {
    return COIL_ERROR_FILE_NOT_FOUND;
  }
  
  size_t bytes_written = fwrite(ctx->native_code, 1, ctx->code_size, file);
  fclose(file);
  
  if (bytes_written != ctx->code_size) {
    return COIL_ERROR_UNKNOWN;
  }
  
  return COIL_SUCCESS;
}

/**
 * @brief Free translation context resources
 */
void free_translation_context(translation_context_t* ctx) {
  if (!ctx) {
    return;
  }
  
  free(ctx->native_code);
  free_register_allocator(&ctx->reg_alloc);
  free(ctx);
}

/**
 * @brief Initialize register allocator
 */
coil_error_t initialize_register_allocator(register_allocator_t* allocator, uint32_t reg_count) {
  if (!allocator || reg_count == 0) {
    return COIL_ERROR_UNKNOWN;
  }
  
  allocator->entries = (reg_alloc_entry_t*)malloc(reg_count * sizeof(reg_alloc_entry_t));
  if (!allocator->entries) {
    return COIL_ERROR_OUT_OF_MEMORY;
  }
  
  memset(allocator->entries, 0, reg_count * sizeof(reg_alloc_entry_t));
  allocator->entry_count = reg_count;
  
  // All registers start as available
  allocator->available_regs = (1ULL << reg_count) - 1;
  
  return COIL_SUCCESS;
}

/**
 * @brief Allocate a physical register for a virtual register
 */
uint32_t allocate_register(register_allocator_t* allocator, uint32_t virtual_reg) {
  if (!allocator) {
    return UINT32_MAX;
  }
  
  // Check if already allocated
  for (size_t i = 0; i < allocator->entry_count; i++) {
    if (allocator->entries[i].is_allocated && 
        allocator->entries[i].virtual_reg == virtual_reg) {
      return allocator->entries[i].physical_reg;
    }
  }
  
  // Find first available register
  uint32_t available = allocator->available_regs;
  if (available == 0) {
    // No registers available, would need to spill
    // For simplicity, just return invalid register in this implementation
    return UINT32_MAX;
  }
  
  // Find first set bit (first available register)
  uint32_t physical_reg = 0;
  while ((available & 1) == 0) {
    available >>= 1;
    physical_reg++;
  }
  
  // Mark as used
  allocator->available_regs &= ~(1UL << physical_reg);
  
  // Find an empty entry
  for (size_t i = 0; i < allocator->entry_count; i++) {
    if (!allocator->entries[i].is_allocated) {
      allocator->entries[i].is_allocated = true;
      allocator->entries[i].virtual_reg = virtual_reg;
      allocator->entries[i].physical_reg = physical_reg;
      allocator->entries[i].is_dirty = false;
      return physical_reg;
    }
  }
  
  // Shouldn't reach here if initialized properly
  return UINT32_MAX;
}

/**
 * @brief Free a register allocation
 */
void free_register(register_allocator_t* allocator, uint32_t virtual_reg) {
  if (!allocator) {
    return;
  }
  
  for (size_t i = 0; i < allocator->entry_count; i++) {
    if (allocator->entries[i].is_allocated && 
        allocator->entries[i].virtual_reg == virtual_reg) {
      // Mark as available
      allocator->available_regs |= (1UL << allocator->entries[i].physical_reg);
      allocator->entries[i].is_allocated = false;
      return;
    }
  }
}

/**
 * @brief Free register allocator resources
 */
void free_register_allocator(register_allocator_t* allocator) {
  if (!allocator) {
    return;
  }
  
  free(allocator->entries);
  allocator->entries = NULL;
  allocator->entry_count = 0;
}