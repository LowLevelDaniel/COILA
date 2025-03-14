/**
 * @file abi.c
 * @brief x86_64 ABI implementation
 * @details Implements the System V AMD64 ABI for the x86_64 target backend.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include "x86_64.h"
#include "coil-assembler/target_internal.h"
#include "coil-assembler/diagnostics.h"
#include "../../utils/memory.c"
#include "../../utils/logging.c"

/* System V AMD64 ABI register usage */

/* Registers used for integer parameter passing */
static const uint32_t int_param_regs[] = {
  X86_64_REG_RDI,  /* 1st argument */
  X86_64_REG_RSI,  /* 2nd argument */
  X86_64_REG_RDX,  /* 3rd argument */
  X86_64_REG_RCX,  /* 4th argument */
  X86_64_REG_R8,   /* 5th argument */
  X86_64_REG_R9    /* 6th argument */
};

/* Registers used for floating-point parameter passing */
static const uint32_t float_param_regs[] = {
  X86_64_REG_XMM0,  /* 1st argument */
  X86_64_REG_XMM1,  /* 2nd argument */
  X86_64_REG_XMM2,  /* 3rd argument */
  X86_64_REG_XMM3,  /* 3rd argument */
  X86_64_REG_XMM4,  /* 5th argument */
  X86_64_REG_XMM5,  /* 6th argument */
  X86_64_REG_XMM6,  /* 7th argument */
  X86_64_REG_XMM7   /* 8th argument */
};

/* Return value registers */
static const uint32_t int_return_reg = X86_64_REG_RAX;
static const uint32_t float_return_reg = X86_64_REG_XMM0;

/* Caller-saved registers (volatile) */
static const uint32_t caller_saved_regs[] = {
  X86_64_REG_RAX,
  X86_64_REG_RCX,
  X86_64_REG_RDX,
  X86_64_REG_RSI,
  X86_64_REG_RDI,
  X86_64_REG_R8,
  X86_64_REG_R9,
  X86_64_REG_R10,
  X86_64_REG_R11,
  /* XMM0-XMM15 are all caller-saved */
  X86_64_REG_XMM0,
  X86_64_REG_XMM1,
  X86_64_REG_XMM2,
  X86_64_REG_XMM3,
  X86_64_REG_XMM4,
  X86_64_REG_XMM5,
  X86_64_REG_XMM6,
  X86_64_REG_XMM7,
  X86_64_REG_XMM8,
  X86_64_REG_XMM9,
  X86_64_REG_XMM10,
  X86_64_REG_XMM11,
  X86_64_REG_XMM12,
  X86_64_REG_XMM13,
  X86_64_REG_XMM14,
  X86_64_REG_XMM15
};

/* Callee-saved registers (non-volatile) */
static const uint32_t callee_saved_regs[] = {
  X86_64_REG_RBX,
  X86_64_REG_RBP,
  X86_64_REG_R12,
  X86_64_REG_R13,
  X86_64_REG_R14,
  X86_64_REG_R15
};

/**
 * @brief Determine if a type is passed in registers or on the stack
 * @param type Data type
 * @return 1 if passed in registers, 0 if passed on stack
 */
static int is_register_passed_type(coil_type_t type) {
  /* In System V AMD64 ABI:
   * - Integers and pointers up to 64 bits are passed in registers
   * - Floating-point values up to 64 bits are passed in registers
   * - Aggregates (structs) are passed on stack unless they're small enough
   *   to fit in registers (we'll simplify and always pass structs on stack)
   * - Vectors up to 128 bits are passed in registers (with SSE2)
   */
  
  switch (coil_type_get_category(type)) {
    case COIL_TYPE_CATEGORY_INTEGER:
    case COIL_TYPE_CATEGORY_BOOLEAN:
      /* Integers and booleans passed in registers if <= 64 bits */
      return (coil_type_get_width(type) <= 64);
      
    case COIL_TYPE_CATEGORY_FLOAT:
      /* Floating-point values passed in registers if <= 64 bits */
      return (coil_type_get_width(type) <= 64);
      
    case COIL_TYPE_CATEGORY_POINTER:
      /* Pointers always passed in registers */
      return 1;
      
    case COIL_TYPE_CATEGORY_VECTOR:
      /* Vectors passed in registers if <= 128 bits */
      return (coil_type_get_width(type) <= 128);
      
    case COIL_TYPE_CATEGORY_STRUCT:
    case COIL_TYPE_CATEGORY_ARRAY:
      /* Simplified: Always pass structs and arrays on stack */
      return 0;
      
    default:
      /* Unknown type, assume stack */
      return 0;
  }
}

/**
 * @brief Get the register class for a type
 * @param type Data type
 * @return Register class
 */
static int get_register_class(coil_type_t type) {
  switch (coil_type_get_category(type)) {
    case COIL_TYPE_CATEGORY_INTEGER:
    case COIL_TYPE_CATEGORY_BOOLEAN:
    case COIL_TYPE_CATEGORY_POINTER:
      return X86_64_REG_CLASS_GPR;
      
    case COIL_TYPE_CATEGORY_FLOAT:
    case COIL_TYPE_CATEGORY_VECTOR:
      return X86_64_REG_CLASS_XMM;
      
    default:
      /* Unknown type, assume GPR */
      return X86_64_REG_CLASS_GPR;
  }
}

/**
 * @brief Apply System V AMD64 ABI calling convention to a function
 * @param context Target context
 * @param function COIL function
 * @return 0 on success, non-zero on failure
 */
int x86_64_target_apply_calling_convention(coil_target_context_t *context,
                                         coil_function_t *function) {
  if (!context || !function) {
    return -1;
  }
  
  /* Only handle functions with valid type */
  if (function->type == 0) {
    coil_diagnostics_reportf(NULL, COIL_DIAG_ERROR, 
                           COIL_DIAG_CATEGORY_TARGET,
                           1, "Function '%s' has invalid type", function->name);
    return -1;
  }
  
  /* Get function type information */
  coil_function_type_t func_type;
  /* In a real implementation, we would get this from the type system */
  /* For this example, we'll just assume some parameters */
  
  uint32_t param_count = function->param_count;
  
  /* Allocate parameter register array if needed */
  if (!function->param_regs && param_count > 0) {
    function->param_regs = (uint32_t*)coil_calloc(param_count, sizeof(uint32_t));
    if (!function->param_regs) {
      coil_diagnostics_reportf(NULL, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_TARGET,
                             2, "Failed to allocate parameter registers for '%s'", 
                             function->name);
      return -1;
    }
  }
  
  /* Assign registers for parameters */
  uint32_t int_reg_index = 0;
  uint32_t float_reg_index = 0;
  uint32_t stack_offset = 0;  /* Relative to stack pointer at entry */
  
  for (uint32_t i = 0; i < param_count; i++) {
    /* In a real implementation, we would get the parameter type from func_type */
    /* For this example, we'll just assume all parameters are integers */
    coil_type_t param_type = COIL_TYPE_INT32;
    
    /* Determine if the parameter is passed in register or on stack */
    if (is_register_passed_type(param_type)) {
      /* Get register class */
      int reg_class = get_register_class(param_type);
      
      if (reg_class == X86_64_REG_CLASS_GPR && int_reg_index < 6) {
        /* Integer parameter in register */
        function->param_regs[i] = int_param_regs[int_reg_index++];
      } else if (reg_class == X86_64_REG_CLASS_XMM && float_reg_index < 8) {
        /* Floating-point parameter in register */
        function->param_regs[i] = float_param_regs[float_reg_index++];
      } else {
        /* No registers left, pass on stack */
        function->param_regs[i] = 0;  /* 0 indicates stack */
        
        /* Align stack offset to parameter size */
        uint32_t param_size = coil_type_get_width(param_type) / 8;
        if (param_size > 0 && stack_offset % param_size != 0) {
          stack_offset = (stack_offset + param_size - 1) & ~(param_size - 1);
        }
        
        /* Store stack offset in param_regs */
        /* In a real implementation, we would store this in a separate array */
        function->param_regs[i] = stack_offset;
        
        stack_offset += param_size;
      }
    } else {
      /* Parameter passed on stack */
      function->param_regs[i] = 0;  /* 0 indicates stack */
      
      /* Align stack offset to parameter size */
      uint32_t param_size = coil_type_get_width(param_type) / 8;
      if (param_size > 0 && stack_offset % param_size != 0) {
        stack_offset = (stack_offset + param_size - 1) & ~(param_size - 1);
      }
      
      /* Store stack offset in param_regs */
      /* In a real implementation, we would store this in a separate array */
      function->param_regs[i] = stack_offset;
      
      stack_offset += param_size;
    }
  }
  
  /* Store parameter count */
  function->param_count = param_count;
  
  /* Log calling convention application */
  coil_log_debug("Applied System V AMD64 ABI calling convention to function '%s'",
                function->name);
  
  return 0;
}

/**
 * @brief Get register used for return value
 * @param type Return value type
 * @return Return register ID
 */
uint32_t x86_64_target_get_return_register(coil_type_t type) {
  /* In System V AMD64 ABI:
   * - Integer and pointer return values use RAX
   * - Floating-point return values use XMM0
   * - Small structs (≤ 128 bits) can be returned in registers,
   *   but we'll simplify and assume they're returned via hidden
   *   pointer argument
   */
  
  switch (coil_type_get_category(type)) {
    case COIL_TYPE_CATEGORY_INTEGER:
    case COIL_TYPE_CATEGORY_BOOLEAN:
    case COIL_TYPE_CATEGORY_POINTER:
      return int_return_reg;
      
    case COIL_TYPE_CATEGORY_FLOAT:
      return float_return_reg;
      
    case COIL_TYPE_CATEGORY_VECTOR:
      if (coil_type_get_width(type) <= 128) {
        return float_return_reg;
      } else {
        /* Return via hidden pointer */
        return 0;
      }
      
    case COIL_TYPE_CATEGORY_STRUCT:
    case COIL_TYPE_CATEGORY_ARRAY:
      /* Simplified: Always return structs and arrays via hidden pointer */
      return 0;
      
    default:
      /* Unknown type, assume RAX */
      return int_return_reg;
  }
}

/**
 * @brief Get caller-saved registers
 * @param count Pointer to store number of registers
 * @return Array of caller-saved register IDs
 */
const uint32_t* x86_64_target_get_caller_saved_registers(uint32_t *count) {
  if (count) {
    *count = sizeof(caller_saved_regs) / sizeof(caller_saved_regs[0]);
  }
  
  return caller_saved_regs;
}

/**
 * @brief Get callee-saved registers
 * @param count Pointer to store number of registers
 * @return Array of callee-saved register IDs
 */
const uint32_t* x86_64_target_get_callee_saved_registers(uint32_t *count) {
  if (count) {
    *count = sizeof(callee_saved_regs) / sizeof(callee_saved_regs[0]);
  }
  
  return callee_saved_regs;
}

/**
 * @brief Get stack alignment requirement
 * @return Stack alignment in bytes
 */
uint32_t x86_64_target_get_stack_alignment(void) {
  /* System V AMD64 ABI requires 16-byte stack alignment */
  return 16;
}

/**
 * @brief Check if a function needs a frame pointer
 * @param function COIL function
 * @return 1 if frame pointer is needed, 0 if not
 */
int x86_64_target_needs_frame_pointer(coil_function_t *function) {
  /* Simplified: Always use frame pointer */
  /* In a real implementation, we would check for:
   * - Variable-sized stack allocations
   * - Calls to functions that can throw exceptions
   * - Debug info requirements
   * - Optimization level
   */
  return 1;
}

/**
 * @brief Get a name for register
 * @param reg_id Register ID
 * @return Register name or NULL if unknown
 */
const char* x86_64_target_get_register_name(uint32_t reg_id) {
  /* GPRs */
  if (reg_id >= X86_64_REG_RAX && reg_id <= X86_64_REG_R15) {
    static const char* gpr_names[] = {
      "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    return gpr_names[reg_id];
  }
  
  /* XMM registers */
  if (reg_id >= X86_64_REG_XMM0 && reg_id <= X86_64_REG_XMM15) {
    static const char* xmm_names[] = {
      "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
      "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"
    };
    return xmm_names[reg_id - X86_64_REG_XMM0];
  }
  
  /* Unknown register */
  return NULL;
}