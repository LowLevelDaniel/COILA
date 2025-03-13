/**
 * @file instruction_decoder.c
 * @brief Implementation of COIL instruction decoder
 * 
 * This module implements decoding of COIL instructions from binary format 
 * into an internal representation that can be processed by the translator 
 * and optimizer.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "instruction_decoder.h"
#include "../utils/memory_management.h"
#include "../utils/logging.h"

/**
 * @brief Instruction format information
 */
typedef struct {
  uint8_t category;           /**< Instruction category */
  uint8_t opcode;             /**< Instruction opcode */
  uint8_t operand_count;      /**< Expected number of operands */
  uint8_t flags;              /**< Format-specific flags */
} instruction_format_t;

/**
 * @brief Maps an instruction category and opcode to its format
 *
 * @param[in] category Instruction category
 * @param[in] opcode Instruction opcode
 * @param[out] format Pointer to receive the format information
 * @return Error code indicating success or failure
 */
static error_t get_instruction_format(
  uint8_t category,
  uint8_t opcode,
  instruction_format_t* format
) {
  if (format == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  format->category = category;
  format->opcode = opcode;
  format->flags = 0;
  
  /* Set default operand count based on category and opcode */
  switch (category) {
    case COIL_CAT_ARITHMETIC:
      /* Most arithmetic ops have 2 operands except for unary ones */
      if (opcode == COIL_OP_NEG || opcode == COIL_OP_ABS) {
        format->operand_count = 1;
      } else if (opcode == COIL_OP_FMA) {
        format->operand_count = 3;  /* Three operands for Fused Multiply-Add */
      } else {
        format->operand_count = 2;
      }
      break;
    
    case COIL_CAT_LOGICAL:
      /* Most logical ops have 2 operands except for NOT */
      format->operand_count = 2;
      if (opcode == 0x01) {  /* Assuming 0x01 is NOT */
        format->operand_count = 1;
      }
      break;
    
    case COIL_CAT_COMPARISON:
      /* All comparison ops have 2 operands */
      format->operand_count = 2;
      break;
    
    case COIL_CAT_CONTROL:
      /* Control flow ops have variable operand counts */
      switch (opcode) {
        case 0x01:  /* Assuming 0x01 is JUMP */
          format->operand_count = 1;
          break;
        case 0x02:  /* Assuming 0x02 is BRANCH */
          format->operand_count = 2;
          break;
        case 0x03:  /* Assuming 0x03 is CALL */
          format->operand_count = 1;
          format->flags |= 0x01;  /* Flag indicating variable argument count */
          break;
        case 0x04:  /* Assuming 0x04 is RETURN */
          format->operand_count = 0;
          break;
        default:
          format->operand_count = 0;
          break;
      }
      break;
    
    case COIL_CAT_MEMORY:
      /* Memory ops typically have 2 operands (source/dest) */
      format->operand_count = 2;
      break;
    
    case COIL_CAT_CONVERSION:
      /* Conversion ops typically have 1 operand */
      format->operand_count = 1;
      break;
    
    case COIL_CAT_VECTOR:
      /* Vector ops typically have 2-3 operands */
      format->operand_count = 2;
      break;
    
    case COIL_CAT_ATOMIC:
      /* Atomic ops typically have 2-3 operands */
      format->operand_count = 2;
      break;
    
    case COIL_CAT_SPECIAL:
      /* Special ops have variable operand counts */
      format->operand_count = 0;
      break;
    
    default:
      log_error("Unknown instruction category: 0x%02X", category);
      return ERROR_INVALID_FORMAT;
  }
  
  return ERROR_NONE;
}

/**
 * @brief Decodes an operand from binary data
 *
 * @param[in] binary Pointer to binary data
 * @param[in] size Size of binary data in bytes
 * @param[in,out] offset Pointer to current offset in binary (updated)
 * @param[out] operand Pointer to receive the decoded operand
 * @return Error code indicating success or failure
 */
static error_t decode_operand(
  const uint8_t* binary,
  size_t size,
  size_t* offset,
  coil_operand_t* operand
) {
  if (binary == NULL || offset == NULL || operand == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  if (*offset + 1 > size) {
    return ERROR_INVALID_FORMAT;
  }
  
  /* Read operand type */
  uint8_t type_byte = binary[(*offset)++];
  operand->type = type_byte & 0x03;  /* Lower 2 bits for type */
  
  /* Read data type from the next 4 bits */
  operand->data_type = (type_byte >> 2) & 0x0F;
  
  /* Read operand value based on type */
  switch (operand->type) {
    case COIL_OPERAND_REGISTER: {
      if (*offset + sizeof(uint32_t) > size) {
        return ERROR_INVALID_FORMAT;
      }
      
      /* Read register ID (32-bit) */
      operand->value.reg_id = binary[*offset] |
                             (binary[*offset + 1] << 8) |
                             (binary[*offset + 2] << 16) |
                             (binary[*offset + 3] << 24);
      *offset += sizeof(uint32_t);
      break;
    }
    
    case COIL_OPERAND_IMMEDIATE: {
      if (*offset + sizeof(int64_t) > size) {
        return ERROR_INVALID_FORMAT;
      }
      
      /* Read immediate value (64-bit) */
      operand->value.imm_value = 
        ((int64_t)binary[*offset]) |
        ((int64_t)binary[*offset + 1] << 8) |
        ((int64_t)binary[*offset + 2] << 16) |
        ((int64_t)binary[*offset + 3] << 24) |
        ((int64_t)binary[*offset + 4] << 32) |
        ((int64_t)binary[*offset + 5] << 40) |
        ((int64_t)binary[*offset + 6] << 48) |
        ((int64_t)binary[*offset + 7] << 56);
      *offset += sizeof(int64_t);
      break;
    }
    
    case COIL_OPERAND_MEMORY: {
      if (*offset + 3 * sizeof(uint32_t) + sizeof(uint8_t) > size) {
        return ERROR_INVALID_FORMAT;
      }
      
      /* Read memory address components */
      operand->value.mem.base_reg = binary[*offset] |
                                   (binary[*offset + 1] << 8) |
                                   (binary[*offset + 2] << 16) |
                                   (binary[*offset + 3] << 24);
      *offset += sizeof(uint32_t);
      
      operand->value.mem.displacement = binary[*offset] |
                                       (binary[*offset + 1] << 8) |
                                       (binary[*offset + 2] << 16) |
                                       (binary[*offset + 3] << 24);
      *offset += sizeof(uint32_t);
      
      operand->value.mem.index_reg = binary[*offset] |
                                    (binary[*offset + 1] << 8) |
                                    (binary[*offset + 2] << 16) |
                                    (binary[*offset + 3] << 24);
      *offset += sizeof(uint32_t);
      
      operand->value.mem.scale = binary[(*offset)++];
      break;
    }
    
    case COIL_OPERAND_LABEL: {
      if (*offset + sizeof(uint32_t) > size) {
        return ERROR_INVALID_FORMAT;
      }
      
      /* Read label ID (32-bit) */
      operand->value.label_id = binary[*offset] |
                               (binary[*offset + 1] << 8) |
                               (binary[*offset + 2] << 16) |
                               (binary[*offset + 3] << 24);
      *offset += sizeof(uint32_t);
      break;
    }
    
    default:
      log_error("Unknown operand type: %u", operand->type);
      return ERROR_INVALID_FORMAT;
  }
  
  return ERROR_NONE;
}

error_t instruction_decoder_decode_instruction(
  const uint8_t* binary,
  size_t size,
  coil_instruction_t* instruction,
  size_t* bytes_read
) {
  if (binary == NULL || instruction == NULL || bytes_read == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  if (size < 3) {  /* Minimum size for category, opcode, and flags */
    return ERROR_INVALID_FORMAT;
  }
  
  size_t offset = 0;
  
  /* Read instruction header */
  uint8_t category = binary[offset++];
  uint8_t opcode = binary[offset++];
  uint8_t flags = binary[offset++];
  
  /* Get instruction format */
  instruction_format_t format;
  error_t format_result = get_instruction_format(category, opcode, &format);
  if (format_result != ERROR_NONE) {
    return format_result;
  }
  
  /* Initialize instruction fields */
  instruction->opcode = opcode;
  instruction->flags = flags;
  
  /* Read destination register */
  if (offset + sizeof(uint8_t) > size) {
    return ERROR_INVALID_FORMAT;
  }
  
  instruction->destination = binary[offset++];
  
  /* Read data type */
  if (offset + sizeof(uint32_t) > size) {
    return ERROR_INVALID_FORMAT;
  }
  
  instruction->data_type = binary[offset] |
                         (binary[offset + 1] << 8) |
                         (binary[offset + 2] << 16) |
                         (binary[offset + 3] << 24);
  offset += sizeof(uint32_t);
  
  /* Read operand count */
  if (offset + sizeof(uint8_t) > size) {
    return ERROR_INVALID_FORMAT;
  }
  
  instruction->operand_count = binary[offset++];
  
  /* Validate operand count */
  if (instruction->operand_count > MAX_OPERANDS) {
    log_error("Too many operands: %u (maximum: %u)",
             instruction->operand_count, MAX_OPERANDS);
    return ERROR_INVALID_FORMAT;
  }
  
  /* Read operands */
  for (uint8_t i = 0; i < instruction->operand_count; i++) {
    error_t operand_result = decode_operand(binary, size, &offset, &instruction->operands[i]);
    if (operand_result != ERROR_NONE) {
      return operand_result;
    }
  }
  
  *bytes_read = offset;
  return ERROR_NONE;
}

/**
 * @brief Creates a basic block structure and initializes it
 *
 * @param[in] id Block identifier
 * @param[out] block Pointer to receive the created block
 * @return Error code indicating success or failure
 */
static error_t create_basic_block(uint32_t id, coil_basic_block_t** block) {
  if (block == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  coil_basic_block_t* new_block = memory_calloc(1, sizeof(coil_basic_block_t));
  if (new_block == NULL) {
    return ERROR_MEMORY;
  }
  
  new_block->id = id;
  new_block->label = NULL;
  new_block->instructions = NULL;
  new_block->instruction_count = 0;
  new_block->successors = NULL;
  new_block->successor_count = 0;
  new_block->predecessors = NULL;
  new_block->predecessor_count = 0;
  
  *block = new_block;
  return ERROR_NONE;
}

/**
 * @brief Frees a basic block structure and its resources
 *
 * @param[in] block Block to free
 * @return Error code indicating success or failure
 */
static error_t free_basic_block(coil_basic_block_t* block) {
  if (block == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  memory_free(block->label);
  memory_free(block->instructions);
  memory_free(block->successors);
  memory_free(block->predecessors);
  memory_free(block);
  
  return ERROR_NONE;
}

/**
 * @brief Creates a function structure and initializes it
 *
 * @param[in] id Function identifier
 * @param[out] function Pointer to receive the created function
 * @return Error code indicating success or failure
 */
static error_t create_function(uint32_t id, coil_function_t** function) {
  if (function == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  coil_function_t* new_function = memory_calloc(1, sizeof(coil_function_t));
  if (new_function == NULL) {
    return ERROR_MEMORY;
  }
  
  new_function->id = id;
  new_function->name = NULL;
  new_function->param_types = NULL;
  new_function->param_count = 0;
  new_function->return_type = 0;
  new_function->entry_block = NULL;
  new_function->blocks = NULL;
  new_function->block_count = 0;
  
  *function = new_function;
  return ERROR_NONE;
}

/**
 * @brief Frees a function structure and its resources
 *
 * @param[in] function Function to free
 * @return Error code indicating success or failure
 */
static error_t free_function(coil_function_t* function) {
  if (function == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  memory_free(function->name);
  memory_free(function->param_types);
  
  /* Free all basic blocks */
  if (function->blocks != NULL) {
    for (uint32_t i = 0; i < function->block_count; i++) {
      if (function->blocks[i] != NULL) {
        free_basic_block(function->blocks[i]);
      }
    }
    memory_free(function->blocks);
  }
  
  memory_free(function);
  
  return ERROR_NONE;
}

error_t instruction_decoder_decode(
  const coil_module_t* module,
  coil_decoded_module_t** decoded_module
) {
  if (module == NULL || decoded_module == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *decoded_module = NULL;
  
  /* Allocate decoded module structure */
  coil_decoded_module_t* new_module = memory_calloc(1, sizeof(coil_decoded_module_t));
  if (new_module == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Get function declarations section */
  const uint8_t* function_section = NULL;
  size_t function_section_size = 0;
  error_t function_section_result = binary_parser_get_section(
    module,
    COIL_SECTION_FUNCTION,
    &function_section,
    &function_section_size
  );
  
  if (function_section_result == ERROR_NONE) {
    /* Process function declarations */
    if (function_section_size < sizeof(uint32_t)) {
      log_error("Function section too small to contain function count");
      memory_free(new_module);
      return ERROR_INVALID_FORMAT;
    }
    
    /* Read function count */
    uint32_t function_count = function_section[0] |
                            (function_section[1] << 8) |
                            (function_section[2] << 16) |
                            (function_section[3] << 24);
    
    /* Allocate function array */
    new_module->functions = memory_calloc(function_count, sizeof(coil_function_t*));
    if (new_module->functions == NULL) {
      memory_free(new_module);
      return ERROR_MEMORY;
    }
    new_module->function_count = function_count;
    
    size_t offset = sizeof(uint32_t);
    
    /* Process each function declaration */
    for (uint32_t i = 0; i < function_count; i++) {
      if (offset + 3 * sizeof(uint32_t) > function_section_size) {
        log_error("Function section too small to contain function declaration %u", i);
        instruction_decoder_free_module(new_module);
        return ERROR_INVALID_FORMAT;
      }
      
      /* Read function ID */
      uint32_t function_id = function_section[offset] |
                           (function_section[offset + 1] << 8) |
                           (function_section[offset + 2] << 16) |
                           (function_section[offset + 3] << 24);
      offset += sizeof(uint32_t);
      
      /* Create function structure */
      coil_function_t* function = NULL;
      error_t function_result = create_function(function_id, &function);
      if (function_result != ERROR_NONE) {
        instruction_decoder_free_module(new_module);
        return function_result;
      }
      
      /* Read return type */
      function->return_type = function_section[offset] |
                            (function_section[offset + 1] << 8) |
                            (function_section[offset + 2] << 16) |
                            (function_section[offset + 3] << 24);
      offset += sizeof(uint32_t);
      
      /* Read parameter count */
      uint32_t param_count = function_section[offset] |
                           (function_section[offset + 1] << 8) |
                           (function_section[offset + 2] << 16) |
                           (function_section[offset + 3] << 24);
      offset += sizeof(uint32_t);
      
      function->param_count = param_count;
      
      /* Read name length */
      if (offset + sizeof(uint32_t) > function_section_size) {
        log_error("Function section too small to contain name length for function %u", i);
        free_function(function);
        instruction_decoder_free_module(new_module);
        return ERROR_INVALID_FORMAT;
      }
      
      uint32_t name_length = function_section[offset] |
                           (function_section[offset + 1] << 8) |
                           (function_section[offset + 2] << 16) |
                           (function_section[offset + 3] << 24);
      offset += sizeof(uint32_t);
      
      /* Read name */
      if (offset + name_length > function_section_size) {
        log_error("Function section too small to contain name for function %u", i);
        free_function(function);
        instruction_decoder_free_module(new_module);
        return ERROR_INVALID_FORMAT;
      }
      
      function->name = memory_calloc(name_length + 1, sizeof(char));
      if (function->name == NULL) {
        free_function(function);
        instruction_decoder_free_module(new_module);
        return ERROR_MEMORY;
      }
      
      memcpy(function->name, function_section + offset, name_length);
      offset += name_length;
      
      /* Read parameter types */
      if (offset + param_count * sizeof(uint32_t) > function_section_size) {
        log_error("Function section too small to contain parameter types for function %u", i);
        free_function(function);
        instruction_decoder_free_module(new_module);
        return ERROR_INVALID_FORMAT;
      }
      
      if (param_count > 0) {
        function->param_types = memory_calloc(param_count, sizeof(uint32_t));
        if (function->param_types == NULL) {
          free_function(function);
          instruction_decoder_free_module(new_module);
          return ERROR_MEMORY;
        }
        
        for (uint32_t j = 0; j < param_count; j++) {
          function->param_types[j] = function_section[offset] |
                                   (function_section[offset + 1] << 8) |
                                   (function_section[offset + 2] << 16) |
                                   (function_section[offset + 3] << 24);
          offset += sizeof(uint32_t);
        }
      }
      
      /* Store function in module */
      new_module->functions[i] = function;
    }
  } else if (function_section_result != ERROR_NOT_FOUND) {
    /* Return error only if it's not just missing section */
    log_error("Failed to get function section: %s", error_message(function_section_result));
    memory_free(new_module);
    return function_section_result;
  }
  
  /* Get code section */
  const uint8_t* code_section = NULL;
  size_t code_section_size = 0;
  error_t code_section_result = binary_parser_get_section(
    module,
    COIL_SECTION_CODE,
    &code_section,
    &code_section_size
  );
  
  if (code_section_result == ERROR_NONE) {
    /* Process code section */
    size_t offset = 0;
    
    /* Code section should at least contain a function count */
    if (offset + sizeof(uint32_t) > code_section_size) {
      log_error("Code section too small to contain function count");
      instruction_decoder_free_module(new_module);
      return ERROR_INVALID_FORMAT;
    }
    
    /* Read function count */
    uint32_t function_count = code_section[offset] |
                           (code_section[offset + 1] << 8) |
                           (code_section[offset + 2] << 16) |
                           (code_section[offset + 3] << 24);
    offset += sizeof(uint32_t);
    
    /* Process each function code */
    for (uint32_t i = 0; i < function_count; i++) {
      /* Read function ID */
      if (offset + sizeof(uint32_t) > code_section_size) {
        log_error("Code section too small to contain function ID for function %u", i);
        instruction_decoder_free_module(new_module);
        return ERROR_INVALID_FORMAT;
      }
      
      uint32_t function_id = code_section[offset] |
                           (code_section[offset + 1] << 8) |
                           (code_section[offset + 2] << 16) |
                           (code_section[offset + 3] << 24);
      offset += sizeof(uint32_t);
      
      /* Find function in decoded module */
      coil_function_t* function = instruction_decoder_get_function(new_module, function_id);
      if (function == NULL) {
        log_error("Function with ID %u not found in function declarations", function_id);
        instruction_decoder_free_module(new_module);
        return ERROR_INVALID_FORMAT;
      }
      
      /* Read block count */
      if (offset + sizeof(uint32_t) > code_section_size) {
        log_error("Code section too small to contain block count for function %u", i);
        instruction_decoder_free_module(new_module);
        return ERROR_INVALID_FORMAT;
      }
      
      uint32_t block_count = code_section[offset] |
                           (code_section[offset + 1] << 8) |
                           (code_section[offset + 2] << 16) |
                           (code_section[offset + 3] << 24);
      offset += sizeof(uint32_t);
      
      /* Allocate block array */
      function->blocks = memory_calloc(block_count, sizeof(coil_basic_block_t*));
      if (function->blocks == NULL) {
        instruction_decoder_free_module(new_module);
        return ERROR_MEMORY;
      }
      function->block_count = block_count;
      
      /* Process each basic block */
      for (uint32_t j = 0; j < block_count; j++) {
        /* Read block ID */
        if (offset + sizeof(uint32_t) > code_section_size) {
          log_error("Code section too small to contain block ID for block %u of function %u", j, i);
          instruction_decoder_free_module(new_module);
          return ERROR_INVALID_FORMAT;
        }
        
        uint32_t block_id = code_section[offset] |
                          (code_section[offset + 1] << 8) |
                          (code_section[offset + 2] << 16) |
                          (code_section[offset + 3] << 24);
        offset += sizeof(uint32_t);
        
        /* Create basic block */
        coil_basic_block_t* block = NULL;
        error_t block_result = create_basic_block(block_id, &block);
        if (block_result != ERROR_NONE) {
          instruction_decoder_free_module(new_module);
          return block_result;
        }
        
        /* Read label length */
        if (offset + sizeof(uint32_t) > code_section_size) {
          log_error("Code section too small to contain label length for block %u of function %u", j, i);
          free_basic_block(block);
          instruction_decoder_free_module(new_module);
          return ERROR_INVALID_FORMAT;
        }
        
        uint32_t label_length = code_section[offset] |
                              (code_section[offset + 1] << 8) |
                              (code_section[offset + 2] << 16) |
                              (code_section[offset + 3] << 24);
        offset += sizeof(uint32_t);
        
        /* Read label if present */
        if (label_length > 0) {
          if (offset + label_length > code_section_size) {
            log_error("Code section too small to contain label for block %u of function %u", j, i);
            free_basic_block(block);
            instruction_decoder_free_module(new_module);
            return ERROR_INVALID_FORMAT;
          }
          
          block->label = memory_calloc(label_length + 1, sizeof(char));
          if (block->label == NULL) {
            free_basic_block(block);
            instruction_decoder_free_module(new_module);
            return ERROR_MEMORY;
          }
          
          memcpy(block->label, code_section + offset, label_length);
          offset += label_length;
        }
        
        /* Read instruction count */
        if (offset + sizeof(uint32_t) > code_section_size) {
          log_error("Code section too small to contain instruction count for block %u of function %u", j, i);
          free_basic_block(block);
          instruction_decoder_free_module(new_module);
          return ERROR_INVALID_FORMAT;
        }
        
        uint32_t instruction_count = code_section[offset] |
                                  (code_section[offset + 1] << 8) |
                                  (code_section[offset + 2] << 16) |
                                  (code_section[offset + 3] << 24);
        offset += sizeof(uint32_t);
        
        /* Allocate instruction array */
        block->instructions = memory_calloc(instruction_count, sizeof(coil_instruction_t));
        if (block->instructions == NULL) {
          free_basic_block(block);
          instruction_decoder_free_module(new_module);
          return ERROR_MEMORY;
        }
        block->instruction_count = instruction_count;
        
        /* Decode each instruction */
        for (uint32_t k = 0; k < instruction_count; k++) {
          size_t bytes_read = 0;
          error_t instr_result = instruction_decoder_decode_instruction(
            code_section + offset,
            code_section_size - offset,
            &block->instructions[k],
            &bytes_read
          );
          
          if (instr_result != ERROR_NONE) {
            log_error("Failed to decode instruction %u in block %u of function %u: %s",
                    k, j, i, error_message(instr_result));
            free_basic_block(block);
            instruction_decoder_free_module(new_module);
            return instr_result;
          }
          
          offset += bytes_read;
        }
        
        /* Store block in function */
        function->blocks[j] = block;
        
        /* Set entry block if this is the first block */
        if (j == 0) {
          function->entry_block = block;
        }
      }
      
      /* Read successor/predecessor relationships */
      for (uint32_t j = 0; j < block_count; j++) {
        coil_basic_block_t* block = function->blocks[j];
        
        /* Read successor count */
        if (offset + sizeof(uint32_t) > code_section_size) {
          log_error("Code section too small to contain successor count for block %u of function %u", j, i);
          instruction_decoder_free_module(new_module);
          return ERROR_INVALID_FORMAT;
        }
        
        uint32_t successor_count = code_section[offset] |
                                 (code_section[offset + 1] << 8) |
                                 (code_section[offset + 2] << 16) |
                                 (code_section[offset + 3] << 24);
        offset += sizeof(uint32_t);
        
        /* Allocate successor array */
        if (successor_count > 0) {
          block->successors = memory_calloc(successor_count, sizeof(coil_basic_block_t*));
          if (block->successors == NULL) {
            instruction_decoder_free_module(new_module);
            return ERROR_MEMORY;
          }
          block->successor_count = successor_count;
          
          /* Read each successor */
          for (uint32_t k = 0; k < successor_count; k++) {
            /* Read successor block ID */
            if (offset + sizeof(uint32_t) > code_section_size) {
              log_error("Code section too small to contain successor ID for block %u of function %u", j, i);
              instruction_decoder_free_module(new_module);
              return ERROR_INVALID_FORMAT;
            }
            
            uint32_t successor_id = code_section[offset] |
                                  (code_section[offset + 1] << 8) |
                                  (code_section[offset + 2] << 16) |
                                  (code_section[offset + 3] << 24);
            offset += sizeof(uint32_t);
            
            /* Find successor block */
            coil_basic_block_t* successor = NULL;
            for (uint32_t l = 0; l < block_count; l++) {
              if (function->blocks[l]->id == successor_id) {
                successor = function->blocks[l];
                break;
              }
            }
            
            if (successor == NULL) {
              log_error("Successor block with ID %u not found for block %u of function %u",
                      successor_id, j, i);
              instruction_decoder_free_module(new_module);
              return ERROR_INVALID_FORMAT;
            }
            
            /* Store successor */
            block->successors[k] = successor;
            
            /* Add this block as a predecessor to the successor */
            uint32_t pred_count = successor->predecessor_count;
            coil_basic_block_t** new_preds = memory_realloc(
              successor->predecessors,
              (pred_count + 1) * sizeof(coil_basic_block_t*)
            );
            
            if (new_preds == NULL) {
              instruction_decoder_free_module(new_module);
              return ERROR_MEMORY;
            }
            
            successor->predecessors = new_preds;
            successor->predecessors[pred_count] = block;
            successor->predecessor_count++;
          }
        }
      }
    }
  } else if (code_section_result != ERROR_NOT_FOUND) {
    /* Return error only if it's not just missing section */
    log_error("Failed to get code section: %s", error_message(code_section_result));
    instruction_decoder_free_module(new_module);
    return code_section_result;
  }
  
  /* Get type section */
  const uint8_t* type_section = NULL;
  size_t type_section_size = 0;
  error_t type_section_result = binary_parser_get_section(
    module,
    COIL_SECTION_TYPE,
    &type_section,
    &type_section_size
  );
  
  if (type_section_result == ERROR_NONE) {
    /* Store type information for now, detailed parsing would be added here */
    new_module->type_info = (void*)type_section;
  } else if (type_section_result != ERROR_NOT_FOUND) {
    /* Return error only if it's not just missing section */
    log_error("Failed to get type section: %s", error_message(type_section_result));
    instruction_decoder_free_module(new_module);
    return type_section_result;
  }
  
  /* Get global section */
  const uint8_t* global_section = NULL;
  size_t global_section_size = 0;
  error_t global_section_result = binary_parser_get_section(
    module,
    COIL_SECTION_GLOBAL,
    &global_section,
    &global_section_size
  );
  
  if (global_section_result == ERROR_NONE) {
    /* Store globals information for now, detailed parsing would be added here */
    new_module->globals = (void*)global_section;
  } else if (global_section_result != ERROR_NOT_FOUND) {
    /* Return error only if it's not just missing section */
    log_error("Failed to get global section: %s", error_message(global_section_result));
    instruction_decoder_free_module(new_module);
    return global_section_result;
  }
  
  /* Get constant section */
  const uint8_t* constant_section = NULL;
  size_t constant_section_size = 0;
  error_t constant_section_result = binary_parser_get_section(
    module,
    COIL_SECTION_CONSTANT,
    &constant_section,
    &constant_section_size
  );
  
  if (constant_section_result == ERROR_NONE) {
    /* Store constants information for now, detailed parsing would be added here */
    new_module->constants = (void*)constant_section;
  } else if (constant_section_result != ERROR_NOT_FOUND) {
    /* Return error only if it's not just missing section */
    log_error("Failed to get constant section: %s", error_message(constant_section_result));
    instruction_decoder_free_module(new_module);
    return constant_section_result;
  }
  
  log_info("Successfully decoded COIL module with %u functions",
         new_module->function_count);
  
  *decoded_module = new_module;
  return ERROR_NONE;
}

error_t instruction_decoder_free_module(coil_decoded_module_t* decoded_module) {
  if (decoded_module == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Free functions */
  if (decoded_module->functions != NULL) {
    for (uint32_t i = 0; i < decoded_module->function_count; i++) {
      if (decoded_module->functions[i] != NULL) {
        free_function(decoded_module->functions[i]);
      }
    }
    memory_free(decoded_module->functions);
  }
  
  /* Free module */
  memory_free(decoded_module);
  
  return ERROR_NONE;
}

coil_function_t* instruction_decoder_get_function(
  const coil_decoded_module_t* decoded_module,
  uint32_t function_id
) {
  if (decoded_module == NULL) {
    return NULL;
  }
  
  /* Search for function by ID */
  for (uint32_t i = 0; i < decoded_module->function_count; i++) {
    if (decoded_module->functions[i]->id == function_id) {
      return decoded_module->functions[i];
    }
  }
  
  return NULL;
}

coil_function_t* instruction_decoder_get_function_by_name(
  const coil_decoded_module_t* decoded_module,
  const char* function_name
) {
  if (decoded_module == NULL || function_name == NULL) {
    return NULL;
  }
  
  /* Search for function by name */
  for (uint32_t i = 0; i < decoded_module->function_count; i++) {
    if (decoded_module->functions[i]->name != NULL &&
        strcmp(decoded_module->functions[i]->name, function_name) == 0) {
      return decoded_module->functions[i];
    }
  }
  
  return NULL;
}