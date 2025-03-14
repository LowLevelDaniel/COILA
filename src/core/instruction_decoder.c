/**
 * @file instruction_decoder.c
 * @brief COIL instruction decoder implementation
 * @details Decodes binary COIL instructions into the internal representation.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */
#include <stdlib.h>
#include <string.h>
#include "coil/instructions.h"
#include "coil-assembler/diagnostics.h"
#include "../utils/memory.c"
#include "../utils/logging.c"

/* Instruction format constants */
#define OPCODE_MASK     0xFF
#define FLAG_MASK       0xFF00
#define FLAG_SHIFT      8
#define OPERAND_MASK    0xFF0000
#define OPERAND_SHIFT   16
#define RESULT_MASK     0xFF000000
#define RESULT_SHIFT    24

/* Forward declarations */
static int decode_operand(const uint8_t* data, size_t size, size_t* offset, 
                       coil_operand_t* operand, coil_diagnostics_context_t* diag_context);
static int decode_result_operand(const uint8_t* data, size_t size, size_t* offset, 
                              coil_operand_t* result, coil_diagnostics_context_t* diag_context);

/**
 * @brief Decode a binary instruction into its internal representation
 * @param data Pointer to binary instruction data
 * @param size Size of data in bytes
 * @param offset Current offset in data (updated on return)
 * @param diag_context Diagnostics context (can be NULL)
 * @return Decoded instruction or NULL on failure
 */
coil_instruction_t* coil_decode_instruction(const uint8_t* data, size_t size,
                                          size_t* offset,
                                          coil_diagnostics_context_t* diag_context) {
  if (!data || !offset || *offset >= size) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 1,
                            "Invalid parameters for instruction decoding");
    }
    return NULL;
  }
  
  /* Ensure we have enough data for the instruction header */
  if (*offset + 4 > size) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 2,
                            "Incomplete instruction header");
    }
    return NULL;
  }
  
  /* Read instruction header */
  uint32_t header = *((uint32_t*)(data + *offset));
  *offset += 4;
  
  /* Extract fields */
  uint8_t opcode = header & OPCODE_MASK;
  uint8_t flags = (header & FLAG_MASK) >> FLAG_SHIFT;
  uint8_t operand_count = (header & OPERAND_MASK) >> OPERAND_SHIFT;
  uint8_t has_result = (header & RESULT_MASK) >> RESULT_SHIFT;
  
  coil_log_debug("Decoding instruction: opcode=%u, flags=%u, operand_count=%u, has_result=%u",
               opcode, flags, operand_count, has_result);
  
  /* Read result type if present */
  coil_type_t result_type = COIL_TYPE_VOID;
  if (has_result) {
    if (*offset + 4 > size) {
      if (diag_context) {
        coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                              COIL_DIAG_CATEGORY_INSTRUCTION, 3,
                              "Incomplete result type");
      }
      return NULL;
    }
    
    result_type = *((coil_type_t*)(data + *offset));
    *offset += 4;
    
    coil_log_debug("Result type: 0x%08x", result_type);
  }
  
  /* Allocate instruction */
  coil_instruction_t* inst = (coil_instruction_t*)coil_malloc(
      sizeof(coil_instruction_t));
  
  if (!inst) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 4,
                            "Failed to allocate instruction");
    }
    return NULL;
  }
  
  /* Initialize instruction */
  inst->opcode = opcode;
  inst->flags = flags;
  inst->operand_count = operand_count;
  inst->result_type = result_type;
  
  /* Initialize result operand to NONE */
  inst->result.type = COIL_OPERAND_NONE;
  
  /* Initialize source operands to NONE */
  for (uint8_t i = 0; i < 4; i++) {
    inst->operands[i].type = COIL_OPERAND_NONE;
  }
  
  /* Read result operand if present */
  if (has_result) {
    if (decode_result_operand(data, size, offset, &inst->result, diag_context) != 0) {
      coil_free(inst, sizeof(coil_instruction_t));
      return NULL;
    }
  }
  
  /* Read source operands */
  for (uint8_t i = 0; i < operand_count && i < 4; i++) {
    if (decode_operand(data, size, offset, &inst->operands[i], diag_context) != 0) {
      coil_free(inst, sizeof(coil_instruction_t));
      return NULL;
    }
  }
  
  coil_log_debug("Successfully decoded instruction (opcode=%u)", opcode);
  
  return inst;
}

/**
 * @brief Decode a result operand
 * @param data Pointer to binary data
 * @param size Size of data in bytes
 * @param offset Current offset in data (updated on return)
 * @param result Pointer to store the decoded result operand
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
static int decode_result_operand(const uint8_t* data, size_t size, size_t* offset, 
                              coil_operand_t* result, coil_diagnostics_context_t* diag_context) {
  if (!data || !offset || *offset >= size || !result) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 5,
                            "Invalid parameters for result operand decoding");
    }
    return -1;
  }
  
  /* Ensure we have enough data for the operand type */
  if (*offset + 1 > size) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 6,
                            "Incomplete operand type");
    }
    return -1;
  }
  
  /* Read operand type */
  uint8_t type = data[*offset];
  *offset += 1;
  
  /* Initialize the operand */
  result->type = type;
  
  /* Process based on operand type */
  switch (type) {
    case COIL_OPERAND_REGISTER: {
      /* Decode register operand */
      if (*offset + 4 > size) {
        if (diag_context) {
          coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                                COIL_DIAG_CATEGORY_INSTRUCTION, 7,
                                "Incomplete register operand");
        }
        return -1;
      }
      
      /* Read register ID */
      result->value.reg_id = *((uint32_t*)(data + *offset));
      *offset += 4;
      
      coil_log_debug("Decoded register operand: reg_id=%u", result->value.reg_id);
      break;
    }
    
    case COIL_OPERAND_IMMEDIATE: {
      /* Immediate values shouldn't be used as result operands */
      if (diag_context) {
        coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                              COIL_DIAG_CATEGORY_INSTRUCTION, 8,
                              "Immediate value used as result operand");
      }
      return -1;
    }
    
    case COIL_OPERAND_MEMORY: {
      /* Decode memory operand */
      if (*offset + 12 > size) {
        if (diag_context) {
          coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                                COIL_DIAG_CATEGORY_INSTRUCTION, 9,
                                "Incomplete memory operand");
        }
        return -1;
      }
      
      /* Read base register */
      result->value.mem.base_reg = *((uint32_t*)(data + *offset));
      *offset += 4;
      
      /* Read index register */
      result->value.mem.index_reg = *((uint32_t*)(data + *offset));
      *offset += 4;
      
      /* Read offset */
      result->value.mem.offset = *((int32_t*)(data + *offset));
      *offset += 4;
      
      /* Read scale */
      if (*offset + 1 > size) {
        if (diag_context) {
          coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                                COIL_DIAG_CATEGORY_INSTRUCTION, 10,
                                "Incomplete memory operand scale");
        }
        return -1;
      }
      
      result->value.mem.scale = data[*offset];
      *offset += 1;
      
      coil_log_debug("Decoded memory operand: base_reg=%u, index_reg=%u, offset=%d, scale=%u",
                   result->value.mem.base_reg, result->value.mem.index_reg,
                   result->value.mem.offset, result->value.mem.scale);
      break;
    }
    
    case COIL_OPERAND_BLOCK_REF:
    case COIL_OPERAND_FUNC_REF:
    case COIL_OPERAND_TYPE_REF: {
      /* These types shouldn't be used as result operands */
      if (diag_context) {
        coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                              COIL_DIAG_CATEGORY_INSTRUCTION, 11,
                              "Invalid operand type for result");
      }
      return -1;
    }
    
    case COIL_OPERAND_NONE:
      /* Nothing to decode for NONE type */
      break;
      
    default: {
      /* Unknown operand type */
      if (diag_context) {
        coil_diagnostics_reportf(diag_context, COIL_DIAG_ERROR,
                              COIL_DIAG_CATEGORY_INSTRUCTION, 12,
                              "Unknown operand type: %u", type);
      }
      return -1;
    }
  }
  
  return 0;
}

/**
 * @brief Decode a source operand
 * @param data Pointer to binary data
 * @param size Size of data in bytes
 * @param offset Current offset in data (updated on return)
 * @param operand Pointer to store the decoded operand
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
static int decode_operand(const uint8_t* data, size_t size, size_t* offset, 
                       coil_operand_t* operand, coil_diagnostics_context_t* diag_context) {
  if (!data || !offset || *offset >= size || !operand) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 13,
                            "Invalid parameters for operand decoding");
    }
    return -1;
  }
  
  /* Ensure we have enough data for the operand type */
  if (*offset + 1 > size) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 14,
                            "Incomplete operand type");
    }
    return -1;
  }
  
  /* Read operand type */
  uint8_t type = data[*offset];
  *offset += 1;
  
  /* Initialize the operand */
  operand->type = type;
  
  /* Read data type for typed operands */
  if (type == COIL_OPERAND_REGISTER || type == COIL_OPERAND_IMMEDIATE || 
      type == COIL_OPERAND_MEMORY || type == COIL_OPERAND_TYPE_REF) {
    if (*offset + 4 > size) {
      if (diag_context) {
        coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                              COIL_DIAG_CATEGORY_INSTRUCTION, 15,
                              "Incomplete operand data type");
      }
      return -1;
    }
    
    operand->data_type = *((coil_type_t*)(data + *offset));
    *offset += 4;
    
    coil_log_debug("Operand data type: 0x%08x", operand->data_type);
  } else {
    operand->data_type = COIL_TYPE_VOID;
  }
  
  /* Process based on operand type */
  switch (type) {
    case COIL_OPERAND_REGISTER: {
      /* Decode register operand */
      if (*offset + 4 > size) {
        if (diag_context) {
          coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                                COIL_DIAG_CATEGORY_INSTRUCTION, 16,
                                "Incomplete register operand");
        }
        return -1;
      }
      
      /* Read register ID */
      operand->value.reg_id = *((uint32_t*)(data + *offset));
      *offset += 4;
      
      coil_log_debug("Decoded register operand: reg_id=%u", operand->value.reg_id);
      break;
    }
    
    case COIL_OPERAND_IMMEDIATE: {
      /* Decode immediate operand */
      if (*offset + 8 > size) {
        if (diag_context) {
          coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                                COIL_DIAG_CATEGORY_INSTRUCTION, 17,
                                "Incomplete immediate operand");
        }
        return -1;
      }
      
      /* Read immediate value */
      operand->value.imm_value = *((int64_t*)(data + *offset));
      *offset += 8;
      
      coil_log_debug("Decoded immediate operand: value=%ld", operand->value.imm_value);
      break;
    }
    
    case COIL_OPERAND_MEMORY: {
      /* Decode memory operand */
      if (*offset + 12 > size) {
        if (diag_context) {
          coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                                COIL_DIAG_CATEGORY_INSTRUCTION, 18,
                                "Incomplete memory operand");
        }
        return -1;
      }
      
      /* Read base register */
      operand->value.mem.base_reg = *((uint32_t*)(data + *offset));
      *offset += 4;
      
      /* Read index register */
      operand->value.mem.index_reg = *((uint32_t*)(data + *offset));
      *offset += 4;
      
      /* Read offset */
      operand->value.mem.offset = *((int32_t*)(data + *offset));
      *offset += 4;
      
      /* Read scale */
      if (*offset + 1 > size) {
        if (diag_context) {
          coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                                COIL_DIAG_CATEGORY_INSTRUCTION, 19,
                                "Incomplete memory operand scale");
        }
        return -1;
      }
      
      operand->value.mem.scale = data[*offset];
      *offset += 1;
      
      coil_log_debug("Decoded memory operand: base_reg=%u, index_reg=%u, offset=%d, scale=%u",
                   operand->value.mem.base_reg, operand->value.mem.index_reg,
                   operand->value.mem.offset, operand->value.mem.scale);
      break;
    }
    
    case COIL_OPERAND_BLOCK_REF: {
      /* Decode block reference operand */
      if (*offset + 4 > size) {
        if (diag_context) {
          coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                                COIL_DIAG_CATEGORY_INSTRUCTION, 20,
                                "Incomplete block reference operand");
        }
        return -1;
      }
      
      /* Read block ID */
      operand->value.block_id = *((uint32_t*)(data + *offset));
      *offset += 4;
      
      coil_log_debug("Decoded block reference operand: block_id=%u", operand->value.block_id);
      break;
    }
    
    case COIL_OPERAND_FUNC_REF: {
      /* Decode function reference operand */
      if (*offset + 4 > size) {
        if (diag_context) {
          coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                                COIL_DIAG_CATEGORY_INSTRUCTION, 21,
                                "Incomplete function reference operand");
        }
        return -1;
      }
      
      /* Read function ID */
      operand->value.func_id = *((uint32_t*)(data + *offset));
      *offset += 4;
      
      coil_log_debug("Decoded function reference operand: func_id=%u", operand->value.func_id);
      break;
    }
    
    case COIL_OPERAND_TYPE_REF: {
      /* Decode type reference operand */
      /* Type is already read in the data_type field */
      operand->value.type_id = operand->data_type;
      
      coil_log_debug("Decoded type reference operand: type_id=0x%08x", operand->value.type_id);
      break;
    }
    
    case COIL_OPERAND_NONE:
      /* Nothing to decode for NONE type */
      break;
      
    default: {
      /* Unknown operand type */
      if (diag_context) {
        coil_diagnostics_reportf(diag_context, COIL_DIAG_ERROR,
                              COIL_DIAG_CATEGORY_INSTRUCTION, 22,
                              "Unknown operand type: %u", type);
      }
      return -1;
    }
  }
  
  return 0;
}

/**
 * @brief Decode a binary block into a list of instructions
 * @param data Pointer to binary block data
 * @param size Size of data in bytes
 * @param inst_count Pointer to store the number of instructions
 * @param diag_context Diagnostics context (can be NULL)
 * @return Array of decoded instructions or NULL on failure
 */
coil_instruction_t** coil_decode_block(const uint8_t* data, size_t size,
                                     uint32_t* inst_count,
                                     coil_diagnostics_context_t* diag_context) {
  if (!data || !inst_count) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 23,
                            "Invalid parameters for block decoding");
    }
    return NULL;
  }
  
  /* Initialize instruction count */
  *inst_count = 0;
  
  /* Ensure we have enough data for the instruction count */
  if (size < 4) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 24,
                            "Incomplete block header");
    }
    return NULL;
  }
  
  /* Read instruction count */
  uint32_t count = *((uint32_t*)data);
  size_t offset = 4;
  
  coil_log_debug("Decoding block with %u instructions", count);
  
  /* Allocate instruction array */
  coil_instruction_t** instructions = (coil_instruction_t**)coil_calloc(
      count, sizeof(coil_instruction_t*));
  
  if (!instructions) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR,
                            COIL_DIAG_CATEGORY_INSTRUCTION, 25,
                            "Failed to allocate instruction array");
    }
    return NULL;
  }
  
  /* Decode each instruction */
  for (uint32_t i = 0; i < count; i++) {
    instructions[i] = coil_decode_instruction(data, size, &offset, diag_context);
    if (!instructions[i]) {
      /* Failed to decode instruction, free what we've decoded so far */
      for (uint32_t j = 0; j < i; j++) {
        coil_free(instructions[j], sizeof(coil_instruction_t));
      }
      coil_free(instructions, count * sizeof(coil_instruction_t*));
      
      if (diag_context) {
        coil_diagnostics_reportf(diag_context, COIL_DIAG_ERROR,
                              COIL_DIAG_CATEGORY_INSTRUCTION, 26,
                              "Failed to decode instruction %u", i);
      }
      return NULL;
    }
  }
  
  /* Set the instruction count */
  *inst_count = count;
  
  coil_log_debug("Successfully decoded %u instructions", count);
  
  return instructions;
}

/**
 * @brief Free a decoded instruction
 * @param inst Instruction to free
 */
void coil_free_instruction(coil_instruction_t* inst) {
  if (inst) {
    coil_free(inst, sizeof(coil_instruction_t));
  }
}

/**
 * @brief Free a block of decoded instructions
 * @param instructions Array of instructions
 * @param count Number of instructions
 */
void coil_free_instructions(coil_instruction_t** instructions, uint32_t count) {
  if (!instructions) {
    return;
  }
  
  for (uint32_t i = 0; i < count; i++) {
    coil_free_instruction(instructions[i]);
  }
  
  coil_free(instructions, count * sizeof(coil_instruction_t*));
}

/**
 * @brief Create a string representation of an operand
 * @param operand Operand to convert
 * @param buffer Output buffer
 * @param size Size of the buffer
 * @return Number of characters written or -1 on failure
 */
int coil_operand_to_string(const coil_operand_t* operand, char* buffer, size_t size) {
  if (!operand || !buffer || size == 0) {
    return -1;
  }
  
  /* Convert operand to string based on type */
  switch (operand->type) {
    case COIL_OPERAND_REGISTER:
      return snprintf(buffer, size, "r%u", operand->value.reg_id);
      
    case COIL_OPERAND_IMMEDIATE:
      return snprintf(buffer, size, "%ld", operand->value.imm_value);
      
    case COIL_OPERAND_MEMORY: {
      int written = 0;
      
      /* Start with open bracket */
      written += snprintf(buffer, size, "[");
      
      /* Add base register if non-zero */
      if (operand->value.mem.base_reg != 0) {
        written += snprintf(buffer + written, size - written, "r%u", 
                          operand->value.mem.base_reg);
      }
      
      /* Add index register and scale if non-zero */
      if (operand->value.mem.index_reg != 0) {
        if (operand->value.mem.base_reg != 0) {
          written += snprintf(buffer + written, size - written, " + ");
        }
        
        if (operand->value.mem.scale > 1) {
          written += snprintf(buffer + written, size - written, "r%u * %u", 
                            operand->value.mem.index_reg,
                            operand->value.mem.scale);
        } else {
          written += snprintf(buffer + written, size - written, "r%u", 
                            operand->value.mem.index_reg);
        }
      }
      
      /* Add offset if non-zero */
      if (operand->value.mem.offset != 0) {
        if (operand->value.mem.base_reg != 0 || operand->value.mem.index_reg != 0) {
          if (operand->value.mem.offset > 0) {
            written += snprintf(buffer + written, size - written, " + %d", 
                              operand->value.mem.offset);
          } else {
            written += snprintf(buffer + written, size - written, " - %d", 
                              -operand->value.mem.offset);
          }
        } else {
          written += snprintf(buffer + written, size - written, "%d", 
                            operand->value.mem.offset);
        }
      }
      
      /* Close bracket */
      written += snprintf(buffer + written, size - written, "]");
      
      return written;
    }
      
    case COIL_OPERAND_BLOCK_REF:
      return snprintf(buffer, size, "block_%u", operand->value.block_id);
      
    case COIL_OPERAND_FUNC_REF:
      return snprintf(buffer, size, "func_%u", operand->value.func_id);
      
    case COIL_OPERAND_TYPE_REF: {
      char type_str[64];
      coil_type_to_string(operand->value.type_id, type_str, sizeof(type_str));
      return snprintf(buffer, size, "type(%s)", type_str);
    }
      
    case COIL_OPERAND_NONE:
      return snprintf(buffer, size, "none");
      
    default:
      return snprintf(buffer, size, "unknown");
  }
}

/**
 * @brief Get the opcode name for an instruction
 * @param opcode Instruction opcode
 * @return Opcode name or NULL if unknown
 */
const char* coil_get_opcode_name(uint8_t opcode) {
  switch (opcode) {
    /* Arithmetic operations */
    case COIL_INST_ADD: return "ADD";
    case COIL_INST_SUB: return "SUB";
    case COIL_INST_MUL: return "MUL";
    case COIL_INST_DIV: return "DIV";
    case COIL_INST_REM: return "REM";
    case COIL_INST_NEG: return "NEG";
    case COIL_INST_ABS: return "ABS";
    case COIL_INST_MIN: return "MIN";
    case COIL_INST_MAX: return "MAX";
    case COIL_INST_FMA: return "FMA";
    
    /* Logical operations */
    case COIL_INST_AND: return "AND";
    case COIL_INST_OR: return "OR";
    case COIL_INST_XOR: return "XOR";
    case COIL_INST_NOT: return "NOT";
    case COIL_INST_SHL: return "SHL";
    case COIL_INST_SHR: return "SHR";
    case COIL_INST_SAR: return "SAR";
    case COIL_INST_ROL: return "ROL";
    case COIL_INST_ROR: return "ROR";
    
    /* Comparison operations */
    case COIL_INST_CMP_EQ: return "CMP_EQ";
    case COIL_INST_CMP_NE: return "CMP_NE";
    case COIL_INST_CMP_LT: return "CMP_LT";
    case COIL_INST_CMP_LE: return "CMP_LE";
    case COIL_INST_CMP_GT: return "CMP_GT";
    case COIL_INST_CMP_GE: return "CMP_GE";
    
    /* Control flow operations */
    case COIL_INST_BR: return "BR";
    case COIL_INST_BR_COND: return "BR_COND";
    case COIL_INST_SWITCH: return "SWITCH";
    case COIL_INST_CALL: return "CALL";
    case COIL_INST_RET: return "RET";
    
    /* Memory operations */
    case COIL_INST_LOAD: return "LOAD";
    case COIL_INST_STORE: return "STORE";
    case COIL_INST_LEA: return "LEA";
    case COIL_INST_FENCE: return "FENCE";
    
    /* Type conversion operations */
    case COIL_INST_CONVERT: return "CONVERT";
    case COIL_INST_TRUNC: return "TRUNC";
    case COIL_INST_EXTEND: return "EXTEND";
    case COIL_INST_BITCAST: return "BITCAST";
    
    /* Vector operations */
    case COIL_INST_VADD: return "VADD";
    case COIL_INST_VSUB: return "VSUB";
    case COIL_INST_VMUL: return "VMUL";
    case COIL_INST_VDIV: return "VDIV";
    case COIL_INST_VDOT: return "VDOT";
    case COIL_INST_VSPLAT: return "VSPLAT";
    case COIL_INST_VEXTRACT: return "VEXTRACT";
    case COIL_INST_VINSERT: return "VINSERT";
    
    /* Atomic operations */
    case COIL_INST_ATOMIC_ADD: return "ATOMIC_ADD";
    case COIL_INST_ATOMIC_SUB: return "ATOMIC_SUB";
    case COIL_INST_ATOMIC_AND: return "ATOMIC_AND";
    case COIL_INST_ATOMIC_OR: return "ATOMIC_OR";
    case COIL_INST_ATOMIC_XOR: return "ATOMIC_XOR";
    case COIL_INST_ATOMIC_CAS: return "ATOMIC_CAS";
    
    /* Special operations */
    case COIL_INST_NOP: return "NOP";
    case COIL_INST_TRAP: return "TRAP";
    case COIL_INST_UNREACHABLE: return "UNREACHABLE";
    case COIL_INST_TARGET: return "TARGET";
    
    default: return NULL;
  }
}

/**
 * @brief Get a string representation of an instruction
 * @param inst The instruction to convert
 * @param buffer Output buffer
 * @param size Size of the buffer
 * @return The buffer pointer or NULL on failure
 */
char* coil_instruction_to_string(const coil_instruction_t *inst, 
                               char *buffer, size_t size) {
  if (!inst || !buffer || size == 0) {
    return NULL;
  }
  
  /* Get opcode name */
  const char* opcode_name = coil_get_opcode_name(inst->opcode);
  if (!opcode_name) {
    opcode_name = "UNKNOWN";
  }
  
  /* Start with the opcode */
  int written = snprintf(buffer, size, "%s", opcode_name);
  
  /* Add result operand if present */
  if (inst->result.type != COIL_OPERAND_NONE) {
    char result_str[64];
    coil_operand_to_string(&inst->result, result_str, sizeof(result_str));
    
    written += snprintf(buffer + written, size - written, " %s", result_str);
    
    /* Add assignment operator */
    written += snprintf(buffer + written, size - written, " = ");
  }
  
  /* Add source operands */
  for (uint8_t i = 0; i < inst->operand_count; i++) {
    char operand_str[64];
    coil_operand_to_string(&inst->operands[i], operand_str, sizeof(operand_str));
    
    /* Add comma between operands */
    if (i > 0) {
      written += snprintf(buffer + written, size - written, ", ");
    }
    
    written += snprintf(buffer + written, size - written, "%s", operand_str);
  }
  
  return buffer;
}