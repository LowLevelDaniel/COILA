/**
 * @file coil_binary_parser.c
 * @brief Implementation of COIL binary format parsing.
 */

#include "coil_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Read a null-terminated string from binary data
 * 
 * @param data Pointer to binary data
 * @param offset Offset from which to read (updated to position after string)
 * @param size Total size of data buffer
 * @return Allocated string or NULL on error
 */
static char* read_string(const uint8_t* data, size_t* offset, size_t size) {
  if (*offset >= size) {
    return NULL;
  }
  
  const char* str = (const char*)(data + *offset);
  size_t str_len = 0;
  
  while (*offset + str_len < size && str[str_len] != '\0') {
    str_len++;
  }
  
  if (*offset + str_len >= size) {
    return NULL;  // String not null-terminated within buffer
  }
  
  char* result = (char*)malloc(str_len + 1);
  if (!result) {
    return NULL;
  }
  
  memcpy(result, str, str_len + 1);
  *offset += str_len + 1;
  
  return result;
}

/**
 * @brief Read a 32-bit unsigned integer from binary data
 */
static uint32_t read_uint32(const uint8_t* data, size_t* offset, size_t size) {
  if (*offset + sizeof(uint32_t) > size) {
    return 0;
  }
  
  uint32_t value = *((uint32_t*)(data + *offset));
  *offset += sizeof(uint32_t);
  
  return value;
}

/**
 * @brief Read a 64-bit unsigned integer from binary data
 */
static uint64_t read_uint64(const uint8_t* data, size_t* offset, size_t size) {
  if (*offset + sizeof(uint64_t) > size) {
    return 0;
  }
  
  uint64_t value = *((uint64_t*)(data + *offset));
  *offset += sizeof(uint64_t);
  
  return value;
}

/**
 * @brief Process type section data
 */
coil_error_t process_type_section(coil_module_t* module, const uint8_t* data, size_t size) {
  if (!module || !data) {
    return COIL_ERROR_INVALID_FORMAT;
  }
  
  size_t offset = 0;
  
  // Read type count
  uint32_t type_count = read_uint32(data, &offset, size);
  if (offset >= size) {
    return COIL_ERROR_INVALID_FORMAT;
  }
  
  // Allocate types array
  module->types = (coil_type_info_t*)malloc(type_count * sizeof(coil_type_info_t));
  if (!module->types) {
    return COIL_ERROR_OUT_OF_MEMORY;
  }
  
  memset(module->types, 0, type_count * sizeof(coil_type_info_t));
  module->type_count = type_count;
  
  // Read each type definition
  for (uint32_t i = 0; i < type_count; i++) {
    if (offset >= size) {
      return COIL_ERROR_INVALID_FORMAT;
    }
    
    // Read type encoding
    coil_type_encoding_t encoding = read_uint32(data, &offset, size);
    module->types[i].encoding = encoding;
    
    // Read type name (may be empty for built-in types)
    char* name = read_string(data, &offset, size);
    module->types[i].name = name ? name : strdup(get_type_name(encoding));
    
    // Read type size and alignment
    module->types[i].size = read_uint32(data, &offset, size);
    module->types[i].alignment = read_uint32(data, &offset, size);
    
    // Additional type-specific information could be read here
    // depending on the type category
    uint8_t category = (encoding >> 28) & 0xF;
    
    if (category == TYPE_CATEGORY_STRUCTURE) {
      // Read structure field count and field information
      uint32_t field_count = read_uint32(data, &offset, size);
      
      // Skip field information for now, just validate bounds
      for (uint32_t j = 0; j < field_count; j++) {
        // Field name
        char* field_name = read_string(data, &offset, size);
        if (!field_name) {
          return COIL_ERROR_INVALID_FORMAT;
        }
        free(field_name);
        
        // Field type encoding
        read_uint32(data, &offset, size);
        
        // Field offset
        read_uint32(data, &offset, size);
      }
    } else if (category == TYPE_CATEGORY_ARRAY) {
      // Read element type and count
      read_uint32(data, &offset, size);  // Element type encoding
      read_uint32(data, &offset, size);  // Element count
    }
  }
  
  return COIL_SUCCESS;
}

/**
 * @brief Process function section data
 */
coil_error_t process_function_section(coil_module_t* module, const uint8_t* data, size_t size) {
  if (!module || !data) {
    return COIL_ERROR_INVALID_FORMAT;
  }
  
  size_t offset = 0;
  
  // Read function count
  uint32_t function_count = read_uint32(data, &offset, size);
  if (offset >= size) {
    return COIL_ERROR_INVALID_FORMAT;
  }
  
  // Allocate functions array
  module->functions = (coil_function_t*)malloc(function_count * sizeof(coil_function_t));
  if (!module->functions) {
    return COIL_ERROR_OUT_OF_MEMORY;
  }
  
  memset(module->functions, 0, function_count * sizeof(coil_function_t));
  module->function_count = function_count;
  
  // Read each function definition
  for (uint32_t i = 0; i < function_count; i++) {
    if (offset >= size) {
      return COIL_ERROR_INVALID_FORMAT;
    }
    
    // Read function name
    char* name = read_string(data, &offset, size);
    if (!name) {
      return COIL_ERROR_INVALID_FORMAT;
    }
    module->functions[i].name = name;
    
    // Read return type
    coil_type_encoding_t return_type = read_uint32(data, &offset, size);
    module->functions[i].return_type.encoding = return_type;
    
    // Read parameter count
    uint32_t param_count = read_uint32(data, &offset, size);
    module->functions[i].param_count = param_count;
    
    // Allocate parameter types array
    if (param_count > 0) {
      module->functions[i].param_types = 
        (coil_type_info_t*)malloc(param_count * sizeof(coil_type_info_t));
      
      if (!module->functions[i].param_types) {
        return COIL_ERROR_OUT_OF_MEMORY;
      }
      
      // Read each parameter type
      for (uint32_t j = 0; j < param_count; j++) {
        coil_type_encoding_t param_type = read_uint32(data, &offset, size);
        module->functions[i].param_types[j].encoding = param_type;
      }
    }
    
    // Function flags and other attributes could be read here
  }
  
  return COIL_SUCCESS;
}

/**
 * @brief Process global section data
 */
coil_error_t process_global_section(coil_module_t* module, const uint8_t* data, size_t size) {
  if (!module || !data) {
    return COIL_ERROR_INVALID_FORMAT;
  }
  
  size_t offset = 0;
  
  // Read global count
  uint32_t global_count = read_uint32(data, &offset, size);
  if (offset >= size) {
    return COIL_ERROR_INVALID_FORMAT;
  }
  
  // Allocate globals array
  module->globals = (coil_global_t*)malloc(global_count * sizeof(coil_global_t));
  if (!module->globals) {
    return COIL_ERROR_OUT_OF_MEMORY;
  }
  
  memset(module->globals, 0, global_count * sizeof(coil_global_t));
  module->global_count = global_count;
  
  // Read each global definition
  for (uint32_t i = 0; i < global_count; i++) {
    if (offset >= size) {
      return COIL_ERROR_INVALID_FORMAT;
    }
    
    // Read global name
    char* name = read_string(data, &offset, size);
    if (!name) {
      return COIL_ERROR_INVALID_FORMAT;
    }
    module->globals[i].name = name;
    
    // Read type
    coil_type_encoding_t type = read_uint32(data, &offset, size);
    module->globals[i].type.encoding = type;
    
    // Read flags
    uint32_t flags = read_uint32(data, &offset, size);
    module->globals[i].is_constant = (flags & 0x01) != 0;
    
    // Read initializer (if present)
    uint32_t has_initializer = read_uint32(data, &offset, size);
    if (has_initializer) {
      uint32_t init_size = read_uint32(data, &offset, size);
      
      if (offset + init_size > size) {
        return COIL_ERROR_INVALID_FORMAT;
      }
      
      module->globals[i].initializer = (uint8_t*)malloc(init_size);
      if (!module->globals[i].initializer) {
        return COIL_ERROR_OUT_OF_MEMORY;
      }
      
      memcpy(module->globals[i].initializer, data + offset, init_size);
      module->globals[i].init_size = init_size;
      
      offset += init_size;
    }
  }
  
  return COIL_SUCCESS;
}

/**
 * @brief Process constant section data
 */
coil_error_t process_constant_section(coil_module_t* module, const uint8_t* data, size_t size) {
  // Constants are simply stored in the constant section without complex parsing
  // in this simplified implementation. In a complete implementation, we would
  // parse constant pools and allocate them appropriately.
  return COIL_SUCCESS;
}

/**
 * @brief Read an instruction from binary data
 */
static coil_error_t read_instruction(coil_instruction_t* instr, const uint8_t* data, 
                                   size_t* offset, size_t size) {
  if (*offset + 4 > size) {
    return COIL_ERROR_INVALID_FORMAT;
  }
  
  // Read basic instruction fields
  instr->opcode = data[*offset];
  instr->flags = data[*offset + 1];
  instr->num_operands = data[*offset + 2];
  instr->dest = data[*offset + 3];
  *offset += 4;
  
  // Read type information
  instr->type.encoding = read_uint32(data, offset, size);
  
  // Allocate operands array
  if (instr->num_operands > 0) {
    instr->operands = (coil_operand_t*)malloc(instr->num_operands * sizeof(coil_operand_t));
    if (!instr->operands) {
      return COIL_ERROR_OUT_OF_MEMORY;
    }
    
    memset(instr->operands, 0, instr->num_operands * sizeof(coil_operand_t));
    
    // Read each operand
    for (uint8_t i = 0; i < instr->num_operands; i++) {
      if (*offset + 1 > size) {
        free(instr->operands);
        instr->operands = NULL;
        return COIL_ERROR_INVALID_FORMAT;
      }
      
      uint8_t operand_type = data[(*offset)++];
      instr->operands[i].type = operand_type;
      
      // Read operand data type
      instr->operands[i].data_type.encoding = read_uint32(data, offset, size);
      
      // Read operand-specific data
      switch (operand_type) {
        case OPERAND_REGISTER:
          instr->operands[i].value.reg = read_uint32(data, offset, size);
          break;
          
        case OPERAND_IMMEDIATE:
          instr->operands[i].value.imm = read_uint64(data, offset, size);
          break;
          
        case OPERAND_MEMORY:
          instr->operands[i].value.mem.base = read_uint32(data, offset, size);
          instr->operands[i].value.mem.index = read_uint32(data, offset, size);
          instr->operands[i].value.mem.scale = read_uint32(data, offset, size);
          instr->operands[i].value.mem.disp = read_uint64(data, offset, size);
          break;
          
        case OPERAND_LABEL:
          instr->operands[i].value.label = read_string(data, offset, size);
          if (!instr->operands[i].value.label) {
            free(instr->operands);
            instr->operands = NULL;
            return COIL_ERROR_INVALID_FORMAT;
          }
          break;
          
        default:
          free(instr->operands);
          instr->operands = NULL;
          return COIL_ERROR_INVALID_FORMAT;
      }
    }
  }
  
  return COIL_SUCCESS;
}

/**
 * @brief Process code section data
 */
coil_error_t process_code_section(coil_module_t* module, const uint8_t* data, size_t size) {
  if (!module || !data) {
    return COIL_ERROR_INVALID_FORMAT;
  }
  
  size_t offset = 0;
  
  // Process each function's code
  for (size_t i = 0; i < module->function_count; i++) {
    coil_function_t* func = &module->functions[i];
    
    if (offset >= size) {
      return COIL_ERROR_INVALID_FORMAT;
    }
    
    // Read function ID to confirm match
    uint32_t func_id = read_uint32(data, &offset, size);
    if (func_id != i) {
      return COIL_ERROR_INVALID_FORMAT;
    }
    
    // Read block count
    uint32_t block_count = read_uint32(data, &offset, size);
    func->block_count = block_count;
    
    coil_basic_block_t* last_block = NULL;
    
    // Read each block
    for (uint32_t j = 0; j < block_count; j++) {
      // Allocate a new block
      coil_basic_block_t* block = (coil_basic_block_t*)malloc(sizeof(coil_basic_block_t));
      if (!block) {
        return COIL_ERROR_OUT_OF_MEMORY;
      }
      
      memset(block, 0, sizeof(coil_basic_block_t));
      
      // Link into block list
      if (last_block) {
        last_block->next = block;
      } else {
        func->blocks = block;
      }
      last_block = block;
      
      // Read block label
      block->label = read_string(data, &offset, size);
      if (!block->label) {
        return COIL_ERROR_INVALID_FORMAT;
      }
      
      // Read instruction count
      uint32_t instr_count = read_uint32(data, &offset, size);
      block->instr_count = instr_count;
      
      // Allocate instructions array
      block->instructions = (coil_instruction_t*)malloc(instr_count * sizeof(coil_instruction_t));
      if (!block->instructions) {
        return COIL_ERROR_OUT_OF_MEMORY;
      }
      
      memset(block->instructions, 0, instr_count * sizeof(coil_instruction_t));
      
      // Read each instruction
      for (uint32_t k = 0; k < instr_count; k++) {
        coil_error_t error = read_instruction(&block->instructions[k], data, &offset, size);
        if (error != COIL_SUCCESS) {
          return error;
        }
      }
    }
  }
  
  return COIL_SUCCESS;
}

/**
 * @brief Process relocation section data
 */
coil_error_t process_relocation_section(coil_module_t* module, const uint8_t* data, size_t size) {
  // Relocation section processing is simplified in this implementation
  // In a complete implementation, we would parse relocation entries and store them
  return COIL_SUCCESS;
}

/**
 * @brief Process metadata section data
 */
coil_error_t process_metadata_section(coil_module_t* module, const uint8_t* data, size_t size) {
  // Metadata section processing is simplified in this implementation
  // In a complete implementation, we would parse debug information, optimization hints, etc.
  return COIL_SUCCESS;
}

/**
 * @brief Find a type by its encoding in a module
 */
coil_type_info_t* find_type_by_encoding(coil_module_t* module, coil_type_encoding_t encoding) {
  if (!module) {
    return NULL;
  }
  
  // First check if it matches a built-in type's encoding exactly
  uint8_t category = (encoding >> 28) & 0xF;
  
  // For built-in scalar types, we can just check the encoding directly
  switch (category) {
    case TYPE_CATEGORY_VOID:
    case TYPE_CATEGORY_BOOLEAN:
    case TYPE_CATEGORY_INTEGER:
    case TYPE_CATEGORY_FLOAT:
      for (size_t i = 0; i < module->type_count; i++) {
        if (module->types[i].encoding == encoding) {
          return &module->types[i];
        }
      }
      break;
      
    // For more complex types, we need custom comparison logic
    default:
      // Just check encoding for now, but a real implementation would need more sophisticated
      // type comparison for complex types
      for (size_t i = 0; i < module->type_count; i++) {
        if (module->types[i].encoding == encoding) {
          return &module->types[i];
        }
      }
      break;
  }
  
  return NULL;
}

/**
 * @brief Find a function by name in a module
 */
coil_function_t* find_function_by_name(coil_module_t* module, const char* name) {
  if (!module || !name) {
    return NULL;
  }
  
  for (size_t i = 0; i < module->function_count; i++) {
    if (strcmp(module->functions[i].name, name) == 0) {
      return &module->functions[i];
    }
  }
  
  return NULL;
}

/**
 * @brief Get name for a built-in type encoding
 */
const char* get_type_name(coil_type_encoding_t encoding) {
  uint8_t category = (encoding >> 28) & 0xF;
  uint8_t width = (encoding >> 20) & 0xFF;
  uint8_t qualifiers = (encoding >> 12) & 0xFF;
  
  static char buffer[64];
  
  switch (category) {
    case TYPE_CATEGORY_VOID:
      return "void";
      
    case TYPE_CATEGORY_BOOLEAN:
      return "bool";
      
    case TYPE_CATEGORY_INTEGER:
      snprintf(buffer, sizeof(buffer), "%s%d", 
               (qualifiers & TYPE_QUALIFIER_UNSIGNED) ? "u" : "i", width);
      return buffer;
      
    case TYPE_CATEGORY_FLOAT:
      snprintf(buffer, sizeof(buffer), "f%d", width);
      return buffer;
      
    case TYPE_CATEGORY_POINTER:
      return "ptr";
      
    case TYPE_CATEGORY_VECTOR:
      snprintf(buffer, sizeof(buffer), "vec%d", width);
      return buffer;
      
    case TYPE_CATEGORY_ARRAY:
      return "array";
      
    case TYPE_CATEGORY_STRUCTURE:
      return "struct";
      
    case TYPE_CATEGORY_FUNCTION:
      return "function";
      
    default:
      return "unknown";
  }
}

/**
 * @brief Get size of a type
 */
uint32_t get_type_size(coil_type_encoding_t encoding, target_config_t* target) {
  uint8_t category = (encoding >> 28) & 0xF;
  uint8_t width = (encoding >> 20) & 0xFF;
  
  switch (category) {
    case TYPE_CATEGORY_VOID:
      return 0;
      
    case TYPE_CATEGORY_BOOLEAN:
      return 1;
      
    case TYPE_CATEGORY_INTEGER:
    case TYPE_CATEGORY_FLOAT:
      return width / 8;
      
    case TYPE_CATEGORY_POINTER:
      if (strcmp(target->architecture, "x86_64") == 0 || 
          strcmp(target->architecture, "arm64") == 0) {
        return 8;  // 64-bit pointers
      } else {
        return 4;  // 32-bit pointers on other architectures
      }
      
    case TYPE_CATEGORY_VECTOR:
      // Vector size depends on element type and count
      // This is a simplification
      return width / 8;
      
    default:
      // For complex types, we would need to look up in the type table
      return 8;  // Default conservative size
  }
}

/**
 * @brief Get alignment of a type
 */
uint32_t get_type_alignment(coil_type_encoding_t encoding, target_config_t* target) {
  uint8_t category = (encoding >> 28) & 0xF;
  uint8_t width = (encoding >> 20) & 0xFF;
  
  switch (category) {
    case TYPE_CATEGORY_VOID:
      return 1;
      
    case TYPE_CATEGORY_BOOLEAN:
      return 1;
      
    case TYPE_CATEGORY_INTEGER:
    case TYPE_CATEGORY_FLOAT:
      return width / 8;
      
    case TYPE_CATEGORY_POINTER:
      if (strcmp(target->architecture, "x86_64") == 0 || 
          strcmp(target->architecture, "arm64") == 0) {
        return 8;  // 64-bit pointers
      } else {
        return 4;  // 32-bit pointers on other architectures
      }
      
    case TYPE_CATEGORY_VECTOR:
      // Vector alignment depends on target architecture
      return target->resources.min_alignment;
      
    default:
      // For complex types, we would need to look up in the type table
      return 8;  // Default conservative alignment
  }
}