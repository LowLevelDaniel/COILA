/**
 * @file template.h
 * @brief Template target backend definitions
 * @details This file contains target-specific definitions for the template backend.
 *          Copy and adapt this file when implementing a new target.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_TARGET_TEMPLATE_H
#define COIL_TARGET_TEMPLATE_H

#include "coil-assembler/target.h"

/**
 * @brief Initialize the template target backend
 * @param context Target context to initialize
 * @return 0 on success, non-zero on failure
 */
int template_target_initialize(coil_target_context_t *context);

/**
 * @brief Finalize the template target backend
 * @param context Target context to finalize
 * @return 0 on success, non-zero on failure
 */
int template_target_finalize(coil_target_context_t *context);

/**
 * @brief Map a COIL instruction to template instructions
 * @param context Target context
 * @param instruction COIL instruction to map
 * @return 0 on success, non-zero on failure
 */
int template_target_map_instruction(coil_target_context_t *context,
                                  coil_instruction_t *instruction);

/**
 * @brief Generate native code for a function
 * @param context Target context
 * @param function COIL function to generate
 * @return 0 on success, non-zero on failure
 */
int template_target_generate_function(coil_target_context_t *context,
                                    coil_function_t *function);

/**
 * @brief Register the template target with the framework
 */
void register_template_target(void);

/**
 * @brief Apply template ABI calling convention to a function
 * @param context Target context
 * @param function COIL function to process
 * @return 0 on success, non-zero on failure
 */
int template_target_apply_calling_convention(coil_target_context_t *context,
                                           coil_function_t *function);

/**
 * @brief Register template-specific optimization passes
 * @param context Target context
 * @return 0 on success, non-zero on failure
 */
int template_target_register_optimizations(coil_target_context_t *context);

/**
 * @brief Perform template-specific peephole optimizations
 * @param context Target context
 * @param instructions Instructions to optimize
 * @param count Number of instructions
 * @return 0 on success, non-zero on failure
 */
int template_target_optimize_instruction_sequence(coil_target_context_t *context,
                                                coil_instruction_t *instructions,
                                                uint32_t count);

/* Target-specific constants and defines below */

/**
 * @brief Template register classes
 */
typedef enum {
  TEMPLATE_REG_CLASS_GENERAL = 0,  /**< General-purpose registers */
  TEMPLATE_REG_CLASS_FLOAT = 1,    /**< Floating-point registers */
  TEMPLATE_REG_CLASS_VECTOR = 2,   /**< Vector registers */
  TEMPLATE_REG_CLASS_SPECIAL = 3   /**< Special registers */
} template_register_class_t;

/**
 * @brief Template register IDs
 */
typedef enum {
  /* General-purpose registers */
  TEMPLATE_REG_R0 = 0,
  TEMPLATE_REG_R1 = 1,
  /* ... add more registers as needed */
  
  /* Floating-point registers */
  TEMPLATE_REG_F0 = 32,
  TEMPLATE_REG_F1 = 33,
  /* ... add more registers as needed */
  
  /* Vector registers */
  TEMPLATE_REG_V0 = 64,
  TEMPLATE_REG_V1 = 65,
  /* ... add more registers as needed */
  
  /* Special registers */
  TEMPLATE_REG_SP = 96,  /**< Stack pointer */
  TEMPLATE_REG_LR = 97,  /**< Link register */
  TEMPLATE_REG_PC = 98   /**< Program counter */
} template_register_id_t;

/**
 * @brief Template instruction opcodes
 */
typedef enum {
  TEMPLATE_INST_NOP = 0,    /**< No operation */
  TEMPLATE_INST_MOV = 1,    /**< Move data */
  TEMPLATE_INST_ADD = 2,    /**< Addition */
  TEMPLATE_INST_SUB = 3,    /**< Subtraction */
  /* ... add more instructions as needed */
} template_instruction_opcode_t;

#endif /* COIL_TARGET_TEMPLATE_H */