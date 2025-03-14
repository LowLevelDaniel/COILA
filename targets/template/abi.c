/**
 * @file abi.c
 * @brief ABI implementation for the template target
 * @details Implements the Application Binary Interface for the template target.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include "template.h"
#include "coil-assembler/target_internal.h"

/* Registers used for parameter passing according to the ABI */
static const uint32_t param_registers[] = {
  TEMPLATE_REG_R0,
  TEMPLATE_REG_R1,
  TEMPLATE_REG_R2,
  TEMPLATE_REG_R3
};

/* Registers used for floating-point parameter passing */
static const uint32_t float_param_registers[] = {
  TEMPLATE_REG_F0,
  TEMPLATE_REG_F1,
  TEMPLATE_REG_F2,
  TEMPLATE_REG_F3
};

/* Registers that must be preserved across function calls (callee-saved) */
static const uint32_t callee_saved_registers[] = {
  TEMPLATE_REG_R4,
  TEMPLATE_REG_R5,
  TEMPLATE_REG_R6,
  TEMPLATE_REG_R7
};

/* Registers that need not be preserved (caller-saved) */
static const uint32_t caller_saved_registers[] = {
  TEMPLATE_REG_R0,
  TEMPLATE_REG_R1,
  TEMPLATE_REG_R2,
  TEMPLATE_REG_R3
};

/**
 * Set up the template target ABI
 */
int template_target_setup_abi(coil_target_context_t *context) {
  if (!context || !context->target_data) {
    return -1;
  }
  
  /* Store ABI information in context (implementation-specific) */
  
  return 0;
}

/**
 * Apply calling convention to a function
 */
int template_target_apply_calling_convention(coil_target_context_t *context,
                                           coil_function_t *function) {
  if (!context || !context->target_data || !function) {
    return -1;
  }
  
  /* Get function type information */
  coil_function_type_t *func_type = NULL;
  /* ... implementation to get function type ... */
  
  if (!func_type) {
    return -1;
  }
  
  /* Allocate parameter register array */
  function->param_regs = calloc(func_type->param_count, sizeof(uint32_t));
  if (!function->param_regs) {
    return -1;
  }
  
  /* Assign registers for parameters */
  uint32_t int_reg_index = 0;
  uint32_t float_reg_index = 0;
  
  for (uint32_t i = 0; i < func_type->param_count; i++) {
    coil_type_t param_type = func_type->params[i].type;
    
    /* Check if parameter is floating-point */
    if (coil_type_get_category(param_type) == COIL_TYPE_CATEGORY_FLOAT) {
      /* Use float parameter registers */
      if (float_reg_index < sizeof(float_param_registers) / sizeof(uint32_t)) {
        function->param_regs[i] = float_param_registers[float_reg_index++];
      } else {
        /* Parameter passed on stack */
        function->param_regs[i] = 0; /* 0 indicates stack parameter */
      }
    } else {
      /* Use integer parameter registers */
      if (int_reg_index < sizeof(param_registers) / sizeof(uint32_t)) {
        function->param_regs[i] = param_registers[int_reg_index++];
      } else {
        /* Parameter passed on stack */
        function->param_regs[i] = 0; /* 0 indicates stack parameter */
      }
    }
  }
  
  /* Set function parameter count */
  function->param_count = func_type->param_count;
  
  return 0;
}

/**
 * Get the list of callee-saved registers
 */
const uint32_t* template_target_get_callee_saved_registers(uint32_t *count) {
  if (count) {
    *count = sizeof(callee_saved_registers) / sizeof(uint32_t);
  }
  return callee_saved_registers;
}

/**
 * Get the list of caller-saved registers
 */
const uint32_t* template_target_get_caller_saved_registers(uint32_t *count) {
  if (count) {
    *count = sizeof(caller_saved_registers) / sizeof(uint32_t);
  }
  return caller_saved_registers;
}

/**
 * Get the register used for return values
 */
uint32_t template_target_get_return_register(coil_type_t type) {
  /* Check if return type is floating-point */
  if (coil_type_get_category(type) == COIL_TYPE_CATEGORY_FLOAT) {
    return TEMPLATE_REG_F0; /* First float register */
  } else {
    return TEMPLATE_REG_R0; /* First integer register */
  }
}

/**
 * Determine if a type is passed in registers or on the stack
 */
bool template_target_is_register_passed_type(coil_type_t type) {
  /* Implementation depends on the ABI */
  uint32_t size = coil_type_get_size(type);
  
  /* Simple example: types <= 64 bits are passed in registers */
  return size <= 8;
}