/**
 * @file optimizations.c
 * @brief Target-specific optimizations for the template architecture
 * @details Implements optimizations specific to the template target.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include "template.h"
#include "coil-assembler/target_internal.h"

/* Forward declarations */
static int optimize_arithmetic_sequence(coil_target_context_t *context,
                                       coil_instruction_t *instructions,
                                       uint32_t count);
static int optimize_memory_access(coil_target_context_t *context,
                                 coil_instruction_t *instructions,
                                 uint32_t count);
static int eliminate_redundant_moves(coil_target_context_t *context,
                                    coil_instruction_t *instructions,
                                    uint32_t count);

/**
 * Register template-specific optimization passes
 */
int template_target_register_optimizations(coil_target_context_t *context) {
  if (!context || !context->target_data) {
    return -1;
  }
  
  /* Register optimization passes with the optimizer */
  /* (Implementation depends on the optimizer API) */
  
  return 0;
}

/**
 * Perform template-specific peephole optimizations
 */
int template_target_optimize_instruction_sequence(coil_target_context_t *context,
                                                coil_instruction_t *instructions,
                                                uint32_t count) {
  if (!context || !context->target_data || !instructions || count == 0) {
    return -1;
  }
  
  /* Apply different optimization strategies */
  if (optimize_arithmetic_sequence(context, instructions, count) != 0) {
    return -1;
  }
  
  if (optimize_memory_access(context, instructions, count) != 0) {
    return -1;
  }
  
  if (eliminate_redundant_moves(context, instructions, count) != 0) {
    return -1;
  }
  
  return 0;
}

/**
 * Optimize sequences of arithmetic operations
 * 
 * Examples of optimizations:
 * - Combine consecutive ADD/SUB operations
 * - Strength reduction (e.g., replace MUL by 2 with shift left by 1)
 * - Constant folding
 */
static int optimize_arithmetic_sequence(coil_target_context_t *context,
                                      coil_instruction_t *instructions,
                                      uint32_t count) {
  if (!context || !context->target_data || !instructions || count == 0) {
    return -1;
  }
  
  /* Loop through instructions looking for optimization opportunities */
  for (uint32_t i = 0; i < count - 1; i++) {
    /* Check for ADD followed by ADD that could be combined */
    if (instructions[i].opcode == COIL_INST_ADD && 
        instructions[i+1].opcode == COIL_INST_ADD) {
      /* Check if output of first ADD is input to second ADD */
      if (instructions[i].result.type == COIL_OPERAND_REGISTER &&
          instructions[i+1].operands[0].type == COIL_OPERAND_REGISTER &&
          instructions[i].result.value.reg_id == instructions[i+1].operands[0].value.reg_id) {
        
        /* Check if second operand of second ADD is immediate */
        if (instructions[i+1].operands[1].type == COIL_OPERAND_IMMEDIATE) {
          /* Combine the operations */
          /* ... implementation ... */
        }
      }
    }
    
    /* Check for multiplication by power of 2 that can be replaced with shift */
    if (instructions[i].opcode == COIL_INST_MUL &&
        instructions[i].operands[1].type == COIL_OPERAND_IMMEDIATE) {
      int64_t value = instructions[i].operands[1].value.imm_value;
      
      /* Check if value is a power of 2 */
      if (value > 0 && (value & (value - 1)) == 0) {
        /* Calculate log2 of value */
        int shift = 0;
        while (value > 1) {
          value >>= 1;
          shift++;
        }
        
        /* Replace MUL with SHL */
        instructions[i].opcode = COIL_INST_SHL;
        instructions[i].operands[1].value.imm_value = shift;
      }
    }
  }
  
  return 0;
}

/**
 * Optimize memory access patterns
 * 
 * Examples of optimizations:
 * - Combine adjacent loads/stores
 * - Eliminate redundant loads
 * - Promote stack variables to registers
 */
static int optimize_memory_access(coil_target_context_t *context,
                                coil_instruction_t *instructions,
                                uint32_t count) {
  if (!context || !context->target_data || !instructions || count == 0) {
    return -1;
  }
  
  /* Loop through instructions looking for optimization opportunities */
  for (uint32_t i = 0; i < count - 1; i++) {
    /* Check for LOAD followed by STORE to the same location */
    if (instructions[i].opcode == COIL_INST_LOAD && 
        instructions[i+1].opcode == COIL_INST_STORE) {
      /* Check if addresses match */
      if (instructions[i].operands[0].type == COIL_OPERAND_MEMORY &&
          instructions[i+1].operands[0].type == COIL_OPERAND_MEMORY) {
        
        /* Check if base registers match */
        if (instructions[i].operands[0].value.mem.base_reg == 
            instructions[i+1].operands[0].value.mem.base_reg &&
            instructions[i].operands[0].value.mem.offset == 
            instructions[i+1].operands[0].value.mem.offset) {
          
          /* Check if the value loaded is the value stored */
          if (instructions[i].result.type == COIL_OPERAND_REGISTER &&
              instructions[i+1].operands[1].type == COIL_OPERAND_REGISTER &&
              instructions[i].result.value.reg_id == 
              instructions[i+1].operands[1].value.reg_id) {
            
            /* Eliminate redundant load-store sequence */
            /* ... implementation ... */
          }
        }
      }
    }
  }
  
  return 0;
}

/**
 * Eliminate redundant move operations
 * 
 * Examples of optimizations:
 * - Eliminate MOV Rx, Rx
 * - Eliminate sequences like MOV Rx, Ry followed by MOV Ry, Rx
 * - Forward propagate register values
 */
static int eliminate_redundant_moves(coil_target_context_t *context,
                                   coil_instruction_t *instructions,
                                   uint32_t count) {
  if (!context || !context->target_data || !instructions || count == 0) {
    return -1;
  }
  
  /* Loop through instructions looking for redundant moves */
  for (uint32_t i = 0; i < count; i++) {
    /* Check for move operations (implemented as ADD with 0) */
    if (instructions[i].opcode == COIL_INST_ADD && 
        instructions[i].operands[1].type == COIL_OPERAND_IMMEDIATE &&
        instructions[i].operands[1].value.imm_value == 0) {
      
      /* Check if source and destination are the same */
      if (instructions[i].operands[0].type == COIL_OPERAND_REGISTER &&
          instructions[i].result.type == COIL_OPERAND_REGISTER &&
          instructions[i].operands[0].value.reg_id == 
          instructions[i].result.value.reg_id) {
        
        /* Mark instruction as NOP (will be removed in a subsequent pass) */
        instructions[i].opcode = COIL_INST_NOP;
      }
    }
  }
  
  return 0;
}