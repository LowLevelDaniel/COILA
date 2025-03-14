/**
 * @file optimizer.c
 * @brief COIL code optimizer implementation
 * @details Implementation of the core optimization engine for the COIL assembler.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include "coil-assembler/assembler.h"
#include "coil-assembler/target.h"
#include "coil-assembler/diagnostics.h"
#include "../utils/memory.c"
#include "../utils/logging.c"

/**
 * @brief Optimization pass type
 */
typedef enum {
  COIL_OPT_PASS_PEEPHOLE = 0,    /**< Peephole optimization */
  COIL_OPT_PASS_DCE = 1,         /**< Dead code elimination */
  COIL_OPT_PASS_CSE = 2,         /**< Common subexpression elimination */
  COIL_OPT_PASS_INLINING = 3,    /**< Function inlining */
  COIL_OPT_PASS_LICM = 4,        /**< Loop-invariant code motion */
  COIL_OPT_PASS_CONSTANT_PROP = 5, /**< Constant propagation */
  COIL_OPT_PASS_VECTORIZATION = 6, /**< Auto-vectorization */
  COIL_OPT_PASS_TARGET = 7       /**< Target-specific optimization */
} coil_opt_pass_type_t;

/**
 * @brief Optimization pass function type
 * @param context Target context
 * @param function Function to optimize
 * @param opt_level Optimization level
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
typedef int (*coil_opt_pass_fn_t)(coil_target_context_t* context,
                                coil_function_t* function,
                                coil_optimization_level_t opt_level,
                                coil_diagnostics_context_t* diag_context);

/**
 * @brief Optimization pass structure
 */
typedef struct {
  const char* name;                  /**< Pass name */
  coil_opt_pass_type_t type;         /**< Pass type */
  coil_opt_pass_fn_t function;       /**< Pass function */
  coil_optimization_level_t min_level; /**< Minimum optimization level */
  int enabled;                       /**< Whether the pass is enabled */
} coil_opt_pass_t;

/**
 * @brief Optimizer context structure
 */
typedef struct {
  coil_target_context_t* target_context;  /**< Target context */
  coil_diagnostics_context_t* diag_context; /**< Diagnostics context */
  coil_optimization_level_t opt_level;    /**< Optimization level */
  int optimize_size;                       /**< Whether to optimize for size */
  int debug_info;                          /**< Whether debug info is present */
  
  coil_opt_pass_t* passes;                /**< Optimization passes */
  uint32_t pass_count;                     /**< Number of passes */
  uint32_t pass_capacity;                  /**< Capacity of passes array */
  
  uint32_t stats_instructions_before;      /**< Number of instructions before optimization */
  uint32_t stats_instructions_after;       /**< Number of instructions after optimization */
  uint32_t stats_blocks_before;            /**< Number of blocks before optimization */
  uint32_t stats_blocks_after;             /**< Number of blocks after optimization */
} coil_optimizer_t;

/* Forward declarations of optimization passes */
static int opt_pass_peephole(coil_target_context_t* context,
                           coil_function_t* function,
                           coil_optimization_level_t opt_level,
                           coil_diagnostics_context_t* diag_context);

static int opt_pass_dce(coil_target_context_t* context,
                      coil_function_t* function,
                      coil_optimization_level_t opt_level,
                      coil_diagnostics_context_t* diag_context);

static int opt_pass_cse(coil_target_context_t* context,
                      coil_function_t* function,
                      coil_optimization_level_t opt_level,
                      coil_diagnostics_context_t* diag_context);

static int opt_pass_constant_prop(coil_target_context_t* context,
                                coil_function_t* function,
                                coil_optimization_level_t opt_level,
                                coil_diagnostics_context_t* diag_context);

static int opt_pass_licm(coil_target_context_t* context,
                       coil_function_t* function,
                       coil_optimization_level_t opt_level,
                       coil_diagnostics_context_t* diag_context);

static int opt_pass_target(coil_target_context_t* context,
                         coil_function_t* function,
                         coil_optimization_level_t opt_level,
                         coil_diagnostics_context_t* diag_context);

/* Default optimization passes */
static coil_opt_pass_t default_passes[] = {
  { "Peephole", COIL_OPT_PASS_PEEPHOLE, opt_pass_peephole, COIL_OPT_LEVEL_1, 1 },
  { "DeadCodeElimination", COIL_OPT_PASS_DCE, opt_pass_dce, COIL_OPT_LEVEL_1, 1 },
  { "ConstantPropagation", COIL_OPT_PASS_CONSTANT_PROP, opt_pass_constant_prop, COIL_OPT_LEVEL_1, 1 },
  { "CommonSubexpressionElimination", COIL_OPT_PASS_CSE, opt_pass_cse, COIL_OPT_LEVEL_2, 1 },
  { "LoopInvariantCodeMotion", COIL_OPT_PASS_LICM, opt_pass_licm, COIL_OPT_LEVEL_2, 1 },
  { "TargetSpecific", COIL_OPT_PASS_TARGET, opt_pass_target, COIL_OPT_LEVEL_2, 1 }
};

/**
 * @brief Create a new optimizer
 * @param target_context Target context
 * @param diag_context Diagnostics context (can be NULL)
 * @return New optimizer or NULL on failure
 */
coil_optimizer_t* coil_optimizer_create(coil_target_context_t* target_context,
                                      coil_diagnostics_context_t* diag_context) {
  if (!target_context) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_OPTIMIZER,
                            1, "NULL target context");
    }
    return NULL;
  }
  
  /* Allocate optimizer */
  coil_optimizer_t* optimizer = (coil_optimizer_t*)coil_calloc(1, sizeof(coil_optimizer_t));
  if (!optimizer) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_OPTIMIZER,
                            2, "Failed to allocate optimizer");
    }
    return NULL;
  }
  
  /* Initialize optimizer */
  optimizer->target_context = target_context;
  optimizer->diag_context = diag_context;
  optimizer->opt_level = COIL_OPT_LEVEL_1;
  optimizer->optimize_size = 0;
  optimizer->debug_info = 0;
  
  /* Initialize statistics */
  optimizer->stats_instructions_before = 0;
  optimizer->stats_instructions_after = 0;
  optimizer->stats_blocks_before = 0;
  optimizer->stats_blocks_after = 0;
  
  /* Initialize passes */
  optimizer->pass_capacity = sizeof(default_passes) / sizeof(default_passes[0]);
  optimizer->passes = (coil_opt_pass_t*)coil_calloc(optimizer->pass_capacity, sizeof(coil_opt_pass_t));
  if (!optimizer->passes) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_OPTIMIZER,
                            3, "Failed to allocate optimization passes");
    }
    coil_free(optimizer, sizeof(coil_optimizer_t));
    return NULL;
  }
  
  /* Copy default passes */
  for (uint32_t i = 0; i < optimizer->pass_capacity; i++) {
    optimizer->passes[i] = default_passes[i];
    optimizer->pass_count++;
  }
  
  return optimizer;
}

/**
 * @brief Destroy an optimizer
 * @param optimizer Optimizer to destroy
 */
void coil_optimizer_destroy(coil_optimizer_t* optimizer) {
  if (!optimizer) {
    return;
  }
  
  /* Free passes */
  if (optimizer->passes) {
    coil_free(optimizer->passes, optimizer->pass_capacity * sizeof(coil_opt_pass_t));
  }
  
  /* Free optimizer */
  coil_free(optimizer, sizeof(coil_optimizer_t));
}

/**
 * @brief Set the optimization level
 * @param optimizer Optimizer
 * @param level Optimization level
 * @return 0 on success, non-zero on failure
 */
int coil_optimizer_set_level(coil_optimizer_t* optimizer, coil_optimization_level_t level) {
  if (!optimizer) {
    return -1;
  }
  
  optimizer->opt_level = level;
  
  /* Check if this is size optimization */
  optimizer->optimize_size = (level == COIL_OPT_LEVEL_S);
  
  return 0;
}

/**
 * @brief Set whether debug info is present
 * @param optimizer Optimizer
 * @param debug_info Whether debug info is present
 * @return 0 on success, non-zero on failure
 */
int coil_optimizer_set_debug_info(coil_optimizer_t* optimizer, int debug_info) {
  if (!optimizer) {
    return -1;
  }
  
  optimizer->debug_info = debug_info;
  
  return 0;
}

/**
 * @brief Add a new optimization pass
 * @param optimizer Optimizer
 * @param name Pass name
 * @param type Pass type
 * @param function Pass function
 * @param min_level Minimum optimization level
 * @return 0 on success, non-zero on failure
 */
int coil_optimizer_add_pass(coil_optimizer_t* optimizer,
                           const char* name,
                           coil_opt_pass_type_t type,
                           coil_opt_pass_fn_t function,
                           coil_optimization_level_t min_level) {
  if (!optimizer || !name || !function) {
    return -1;
  }
  
  /* Check if we need to resize the passes array */
  if (optimizer->pass_count >= optimizer->pass_capacity) {
    uint32_t new_capacity = optimizer->pass_capacity * 2;
    coil_opt_pass_t* new_passes = (coil_opt_pass_t*)coil_realloc(
        optimizer->passes,
        optimizer->pass_capacity * sizeof(coil_opt_pass_t),
        new_capacity * sizeof(coil_opt_pass_t));
    
    if (!new_passes) {
      if (optimizer->diag_context) {
        coil_diagnostics_report(optimizer->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_OPTIMIZER,
                              4, "Failed to resize optimization passes");
      }
      return -1;
    }
    
    optimizer->passes = new_passes;
    optimizer->pass_capacity = new_capacity;
  }
  
  /* Add the pass */
  optimizer->passes[optimizer->pass_count].name = name;
  optimizer->passes[optimizer->pass_count].type = type;
  optimizer->passes[optimizer->pass_count].function = function;
  optimizer->passes[optimizer->pass_count].min_level = min_level;
  optimizer->passes[optimizer->pass_count].enabled = 1;
  optimizer->pass_count++;
  
  return 0;
}

/**
 * @brief Enable or disable an optimization pass
 * @param optimizer Optimizer
 * @param index Pass index
 * @param enabled Whether to enable the pass
 * @return 0 on success, non-zero on failure
 */
int coil_optimizer_enable_pass(coil_optimizer_t* optimizer, uint32_t index, int enabled) {
  if (!optimizer || index >= optimizer->pass_count) {
    return -1;
  }
  
  optimizer->passes[index].enabled = enabled;
  
  return 0;
}

/**
 * @brief Find an optimization pass by name
 * @param optimizer Optimizer
 * @param name Pass name
 * @return Pass index or -1 if not found
 */
int coil_optimizer_find_pass(coil_optimizer_t* optimizer, const char* name) {
  if (!optimizer || !name) {
    return -1;
  }
  
  for (uint32_t i = 0; i < optimizer->pass_count; i++) {
    if (strcmp(optimizer->passes[i].name, name) == 0) {
      return i;
    }
  }
  
  return -1;
}

/**
 * @brief Count instructions in a function
 * @param function Function
 * @return Number of instructions
 */
static uint32_t count_instructions(coil_function_t* function) {
  if (!function) {
    return 0;
  }
  
  uint32_t count = 0;
  
  for (uint32_t i = 0; i < function->block_count; i++) {
    if (function->blocks[i]) {
      count += function->blocks[i]->instruction_count;
    }
  }
  
  return count;
}

/**
 * @brief Optimize a function
 * @param optimizer Optimizer
 * @param function Function to optimize
 * @return 0 on success, non-zero on failure
 */
int coil_optimizer_optimize_function(coil_optimizer_t* optimizer,
                                   coil_function_t* function) {
  if (!optimizer || !function) {
    return -1;
  }
  
  /* Record statistics before optimization */
  optimizer->stats_instructions_before = count_instructions(function);
  optimizer->stats_blocks_before = function->block_count;
  
  /* Log optimization start */
  coil_log_debug("Optimizing function '%s' with %u instructions in %u blocks",
                function->name,
                optimizer->stats_instructions_before,
                optimizer->stats_blocks_before);
  
  /* Run all enabled passes */
  for (uint32_t i = 0; i < optimizer->pass_count; i++) {
    coil_opt_pass_t* pass = &optimizer->passes[i];
    
    /* Skip disabled passes */
    if (!pass->enabled) {
      continue;
    }
    
    /* Skip passes below the current optimization level */
    if (pass->min_level > optimizer->opt_level && optimizer->opt_level != COIL_OPT_LEVEL_S) {
      continue;
    }
    
    /* Run the pass */
    coil_log_debug("Running optimization pass: %s", pass->name);
    if (pass->function(optimizer->target_context, function, optimizer->opt_level, optimizer->diag_context) != 0) {
      if (optimizer->diag_context) {
        coil_diagnostics_reportf(optimizer->diag_context, COIL_DIAG_ERROR, 
                               COIL_DIAG_CATEGORY_OPTIMIZER,
                               5, "Failed to run optimization pass: %s", pass->name);
      }
      return -1;
    }
  }
  
  /* Record statistics after optimization */
  optimizer->stats_instructions_after = count_instructions(function);
  optimizer->stats_blocks_after = function->block_count;
  
  /* Log optimization results */
  coil_log_info("Optimized function '%s': %u -> %u instructions, %u -> %u blocks",
               function->name,
               optimizer->stats_instructions_before,
               optimizer->stats_instructions_after,
               optimizer->stats_blocks_before,
               optimizer->stats_blocks_after);
  
  return 0;
}

/**
 * @brief Get optimization statistics
 * @param optimizer Optimizer
 * @param instructions_before Pointer to store instructions before optimization
 * @param instructions_after Pointer to store instructions after optimization
 * @param blocks_before Pointer to store blocks before optimization
 * @param blocks_after Pointer to store blocks after optimization
 * @return 0 on success, non-zero on failure
 */
int coil_optimizer_get_stats(coil_optimizer_t* optimizer,
                           uint32_t* instructions_before,
                           uint32_t* instructions_after,
                           uint32_t* blocks_before,
                           uint32_t* blocks_after) {
  if (!optimizer) {
    return -1;
  }
  
  if (instructions_before) {
    *instructions_before = optimizer->stats_instructions_before;
  }
  
  if (instructions_after) {
    *instructions_after = optimizer->stats_instructions_after;
  }
  
  if (blocks_before) {
    *blocks_before = optimizer->stats_blocks_before;
  }
  
  if (blocks_after) {
    *blocks_after = optimizer->stats_blocks_after;
  }
  
  return 0;
}

/* Implementation of optimization passes */

/**
 * @brief Peephole optimization pass
 * @param context Target context
 * @param function Function to optimize
 * @param opt_level Optimization level
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
static int opt_pass_peephole(coil_target_context_t* context,
                           coil_function_t* function,
                           coil_optimization_level_t opt_level,
                           coil_diagnostics_context_t* diag_context) {
  if (!context || !function) {
    return -1;
  }
  
  int changes = 0;
  
  /* Process each basic block */
  for (uint32_t i = 0; i < function->block_count; i++) {
    coil_basic_block_t* block = function->blocks[i];
    
    if (!block) {
      continue;
    }
    
    /* Process window of instructions */
    for (uint32_t j = 0; j < block->instruction_count; j++) {
      /* Look for simple optimizations */
      coil_instruction_t* inst = &block->instructions[j];
      
      /* 1. NOP elimination */
      if (inst->opcode == COIL_INST_NOP) {
        /* Mark as changes for later removal */
        inst->opcode = COIL_INST_NOP;
        changes++;
        continue;
      }
      
      /* 2. Redundant move elimination - MOV R1, R1 */
      if (inst->opcode == COIL_INST_ADD && 
          inst->operand_count == 2 &&
          inst->operands[1].type == COIL_OPERAND_IMMEDIATE &&
          inst->operands[1].value.imm_value == 0 &&
          inst->operands[0].type == COIL_OPERAND_REGISTER &&
          inst->result.type == COIL_OPERAND_REGISTER &&
          inst->operands[0].value.reg_id == inst->result.value.reg_id) {
        
        /* Mark as NOP for later removal */
        inst->opcode = COIL_INST_NOP;
        changes++;
        continue;
      }
      
      /* 3. Constant folding - add two immediates */
      if (j + 1 < block->instruction_count) {
        coil_instruction_t* next_inst = &block->instructions[j + 1];
        
        /* Look for instruction pairs that can be combined */
      }
    }
    
    /* TODO: Remove NOPs from the block */
  }
  
  return 0;
}

/**
 * @brief Dead code elimination pass
 * @param context Target context
 * @param function Function to optimize
 * @param opt_level Optimization level
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
static int opt_pass_dce(coil_target_context_t* context,
                      coil_function_t* function,
                      coil_optimization_level_t opt_level,
                      coil_diagnostics_context_t* diag_context) {
  if (!context || !function) {
    return -1;
  }
  
  /* TODO: Implement dead code elimination */
  
  return 0;
}

/**
 * @brief Common subexpression elimination pass
 * @param context Target context
 * @param function Function to optimize
 * @param opt_level Optimization level
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
static int opt_pass_cse(coil_target_context_t* context,
                      coil_function_t* function,
                      coil_optimization_level_t opt_level,
                      coil_diagnostics_context_t* diag_context) {
  if (!context || !function) {
    return -1;
  }
  
  /* TODO: Implement common subexpression elimination */
  
  return 0;
}

/**
 * @brief Constant propagation pass
 * @param context Target context
 * @param function Function to optimize
 * @param opt_level Optimization level
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
static int opt_pass_constant_prop(coil_target_context_t* context,
                                coil_function_t* function,
                                coil_optimization_level_t opt_level,
                                coil_diagnostics_context_t* diag_context) {
  if (!context || !function) {
    return -1;
  }
  
  /* TODO: Implement constant propagation */
  
  return 0;
}

/**
 * @brief Loop-invariant code motion pass
 * @param context Target context
 * @param function Function to optimize
 * @param opt_level Optimization level
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
static int opt_pass_licm(coil_target_context_t* context,
                       coil_function_t* function,
                       coil_optimization_level_t opt_level,
                       coil_diagnostics_context_t* diag_context) {
  if (!context || !function) {
    return -1;
  }
  
  /* TODO: Implement loop-invariant code motion */
  
  return 0;
}

/**
 * @brief Target-specific optimization pass
 * @param context Target context
 * @param function Function to optimize
 * @param opt_level Optimization level
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
static int opt_pass_target(coil_target_context_t* context,
                         coil_function_t* function,
                         coil_optimization_level_t opt_level,
                         coil_diagnostics_context_t* diag_context) {
  if (!context || !function) {
    return -1;
  }
  
  /* Get target descriptor */
  const coil_target_descriptor_t* target = coil_target_get_descriptor(context);
  if (!target) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_TARGET,
                            1, "Failed to get target descriptor");
    }
    return -1;
  }
  
  /* Call target-specific optimization function if available */
  /* This would typically be implemented in the target backend */
  
  return 0;
}