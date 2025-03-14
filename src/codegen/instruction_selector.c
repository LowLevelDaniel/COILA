/**
 * @file instruction_selector.c
 * @brief Instruction selection implementation
 * @details Implementation of the instruction selector component for the COIL assembler.
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
#include "../utils/logging.c"

/**
 * @brief Instruction pattern structure
 */
typedef struct {
  uint8_t opcode;        /**< COIL opcode to match */
  uint8_t operand_count; /**< Number of operands to match */
  uint8_t flags;         /**< Flags to match */
  uint8_t cost;          /**< Cost of the pattern (lower is better) */
  const char* name;      /**< Pattern name */
  
  /* Matching function returns true if pattern matches */
  int (*match)(const coil_instruction_t* inst,
              coil_target_context_t* target_context);
  
  /* Selection function that generates the target instructions */
  int (*select)(const coil_instruction_t* inst,
               coil_target_context_t* target_context,
               void* output_buffer);
} instruction_pattern_t;

/**
 * @brief Instruction selector structure
 */
typedef struct {
  coil_target_context_t* target_context;    /**< Target context */
  coil_diagnostics_context_t* diag_context; /**< Diagnostics context */
  
  instruction_pattern_t* patterns;   /**< Instruction patterns */
  uint32_t pattern_count;            /**< Number of patterns */
  uint32_t pattern_capacity;         /**< Capacity of patterns array */
  
  int (*default_selection)(const coil_instruction_t* inst,
                         coil_target_context_t* target_context,
                         void* output_buffer);  /**< Default selection function */
  
  int optimize;                      /**< Whether to optimize selection */
  int verbose;                       /**< Whether to log verbose information */
} instruction_selector_t;

/**
 * @brief Create a new instruction selector
 * @param target_context Target context
 * @param diag_context Diagnostics context (can be NULL)
 * @return New instruction selector or NULL on failure
 */
instruction_selector_t* instruction_selector_create(
    coil_target_context_t* target_context,
    coil_diagnostics_context_t* diag_context) {
  
  if (!target_context) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            1, "NULL target context");
    }
    return NULL;
  }
  
  /* Allocate instruction selector */
  instruction_selector_t* selector = (instruction_selector_t*)coil_calloc(
      1, sizeof(instruction_selector_t));
  
  if (!selector) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            2, "Failed to allocate instruction selector");
    }
    return NULL;
  }
  
  /* Initialize selector */
  selector->target_context = target_context;
  selector->diag_context = diag_context;
  selector->patterns = NULL;
  selector->pattern_count = 0;
  selector->pattern_capacity = 0;
  selector->default_selection = NULL;
  selector->optimize = 1;
  selector->verbose = 0;
  
  /* Get target descriptor */
  const coil_target_descriptor_t* target = coil_target_get_descriptor(target_context);
  if (!target) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            3, "Failed to get target descriptor");
    }
    coil_free(selector, sizeof(instruction_selector_t));
    return NULL;
  }
  
  /* Set default selection function based on target */
  selector->default_selection = target->map_instruction;
  
  return selector;
}

/**
 * @brief Destroy an instruction selector
 * @param selector Instruction selector to destroy
 */
void instruction_selector_destroy(instruction_selector_t* selector) {
  if (!selector) {
    return;
  }
  
  /* Free patterns array */
  if (selector->patterns) {
    coil_free(selector->patterns, 
            selector->pattern_capacity * sizeof(instruction_pattern_t));
  }
  
  /* Free selector */
  coil_free(selector, sizeof(instruction_selector_t));
}

/**
 * @brief Add an instruction pattern
 * @param selector Instruction selector
 * @param opcode COIL opcode to match
 * @param operand_count Number of operands to match
 * @param flags Flags to match
 * @param cost Cost of the pattern (lower is better)
 * @param name Pattern name
 * @param match Matching function
 * @param select Selection function
 * @return 0 on success, non-zero on failure
 */
int instruction_selector_add_pattern(
    instruction_selector_t* selector,
    uint8_t opcode,
    uint8_t operand_count,
    uint8_t flags,
    uint8_t cost,
    const char* name,
    int (*match)(const coil_instruction_t* inst,
                coil_target_context_t* target_context),
    int (*select)(const coil_instruction_t* inst,
                 coil_target_context_t* target_context,
                 void* output_buffer)) {
  
  if (!selector || !match || !select || !name) {
    return -1;
  }
  
  /* Check if we need to allocate or expand the patterns array */
  if (!selector->patterns) {
    selector->pattern_capacity = 32;
    selector->patterns = (instruction_pattern_t*)coil_calloc(
        selector->pattern_capacity, sizeof(instruction_pattern_t));
    
    if (!selector->patterns) {
      if (selector->diag_context) {
        coil_diagnostics_report(selector->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              4, "Failed to allocate patterns array");
      }
      return -1;
    }
  } else if (selector->pattern_count >= selector->pattern_capacity) {
    uint32_t new_capacity = selector->pattern_capacity * 2;
    instruction_pattern_t* new_patterns = (instruction_pattern_t*)coil_realloc(
        selector->patterns,
        selector->pattern_capacity * sizeof(instruction_pattern_t),
        new_capacity * sizeof(instruction_pattern_t));
    
    if (!new_patterns) {
      if (selector->diag_context) {
        coil_diagnostics_report(selector->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              5, "Failed to expand patterns array");
      }
      return -1;
    }
    
    selector->patterns = new_patterns;
    selector->pattern_capacity = new_capacity;
  }
  
  /* Add the pattern */
  selector->patterns[selector->pattern_count].opcode = opcode;
  selector->patterns[selector->pattern_count].operand_count = operand_count;
  selector->patterns[selector->pattern_count].flags = flags;
  selector->patterns[selector->pattern_count].cost = cost;
  selector->patterns[selector->pattern_count].name = name;
  selector->patterns[selector->pattern_count].match = match;
  selector->patterns[selector->pattern_count].select = select;
  
  selector->pattern_count++;
  
  if (selector->verbose) {
    coil_log_debug("Added instruction pattern '%s' for opcode %u", name, opcode);
  }
  
  return 0;
}

/**
 * @brief Set whether to optimize instruction selection
 * @param selector Instruction selector
 * @param optimize Whether to optimize selection
 * @return 0 on success, non-zero on failure
 */
int instruction_selector_set_optimize(instruction_selector_t* selector, int optimize) {
  if (!selector) {
    return -1;
  }
  
  selector->optimize = optimize;
  
  return 0;
}

/**
 * @brief Set whether to log verbose information
 * @param selector Instruction selector
 * @param verbose Whether to log verbose information
 * @return 0 on success, non-zero on failure
 */
int instruction_selector_set_verbose(instruction_selector_t* selector, int verbose) {
  if (!selector) {
    return -1;
  }
  
  selector->verbose = verbose;
  
  return 0;
}

/**
 * @brief Set the default selection function
 * @param selector Instruction selector
 * @param default_selection Default selection function
 * @return 0 on success, non-zero on failure
 */
int instruction_selector_set_default_selection(
    instruction_selector_t* selector,
    int (*default_selection)(const coil_instruction_t* inst,
                           coil_target_context_t* target_context,
                           void* output_buffer)) {
  
  if (!selector || !default_selection) {
    return -1;
  }
  
  selector->default_selection = default_selection;
  
  return 0;
}

/**
 * @brief Select target instructions for a COIL instruction
 * @param selector Instruction selector
 * @param inst COIL instruction
 * @param output_buffer Output buffer
 * @return 0 on success, non-zero on failure
 */
int instruction_selector_select(
    instruction_selector_t* selector,
    const coil_instruction_t* inst,
    void* output_buffer) {
  
  if (!selector || !inst || !output_buffer) {
    return -1;
  }
  
  /* If not optimizing, just use default selection */
  if (!selector->optimize) {
    if (!selector->default_selection) {
      if (selector->diag_context) {
        coil_diagnostics_report(selector->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              6, "No default selection function");
      }
      return -1;
    }
    
    return selector->default_selection(inst, selector->target_context, output_buffer);
  }
  
  /* Find matching patterns */
  instruction_pattern_t* best_pattern = NULL;
  uint8_t best_cost = UINT8_MAX;
  
  for (uint32_t i = 0; i < selector->pattern_count; i++) {
    instruction_pattern_t* pattern = &selector->patterns[i];
    
    /* Check if pattern matches opcode */
    if (pattern->opcode != inst->opcode) {
      continue;
    }
    
    /* Check if pattern matches operand count */
    if (pattern->operand_count != 0 && 
        pattern->operand_count != inst->operand_count) {
      continue;
    }
    
    /* Check if pattern matches flags */
    if (pattern->flags != 0 && 
        (pattern->flags & inst->flags) != pattern->flags) {
      continue;
    }
    
    /* Call pattern match function */
    if (!pattern->match(inst, selector->target_context)) {
      continue;
    }
    
    /* Pattern matches, check if it's better than current best */
    if (pattern->cost < best_cost) {
      best_pattern = pattern;
      best_cost = pattern->cost;
    }
  }
  
  /* Use best pattern if found, otherwise use default selection */
  if (best_pattern) {
    if (selector->verbose) {
      coil_log_debug("Selected pattern '%s' for opcode %u", 
                    best_pattern->name, inst->opcode);
    }
    
    return best_pattern->select(inst, selector->target_context, output_buffer);
  } else {
    if (!selector->default_selection) {
      if (selector->diag_context) {
        coil_diagnostics_report(selector->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              7, "No default selection function");
      }
      return -1;
    }
    
    if (selector->verbose) {
      coil_log_debug("Using default selection for opcode %u", inst->opcode);
    }
    
    return selector->default_selection(inst, selector->target_context, output_buffer);
  }
}

/**
 * @brief Select target instructions for a basic block
 * @param selector Instruction selector
 * @param block COIL basic block
 * @param output_buffer Output buffer
 * @return 0 on success, non-zero on failure
 */
int instruction_selector_select_block(
    instruction_selector_t* selector,
    const coil_basic_block_t* block,
    void* output_buffer) {
  
  if (!selector || !block || !output_buffer) {
    return -1;
  }
  
  /* Select instructions in order */
  for (uint32_t i = 0; i < block->instruction_count; i++) {
    coil_instruction_t* inst = &block->instructions[i];
    
    if (instruction_selector_select(selector, inst, output_buffer) != 0) {
      if (selector->diag_context) {
        coil_diagnostics_reportf(selector->diag_context, COIL_DIAG_ERROR, 
                               COIL_DIAG_CATEGORY_GENERATOR,
                               8, "Failed to select instruction %u in block", i);
      }
      return -1;
    }
  }
  
  return 0;
}

/**
 * @brief Select target instructions for a function
 * @param selector Instruction selector
 * @param function COIL function
 * @param output_buffer Output buffer
 * @return 0 on success, non-zero on failure
 */
int instruction_selector_select_function(
    instruction_selector_t* selector,
    const coil_function_t* function,
    void* output_buffer) {
  
  if (!selector || !function || !output_buffer) {
    return -1;
  }
  
  /* Log function selection */
  if (selector->verbose) {
    coil_log_info("Selecting instructions for function '%s'", function->name);
  }
  
  /* Select blocks in order */
  for (uint32_t i = 0; i < function->block_count; i++) {
    coil_basic_block_t* block = function->blocks[i];
    
    if (selector->verbose) {
      coil_log_debug("Selecting instructions for block %u", i);
    }
    
    if (instruction_selector_select_block(selector, block, output_buffer) != 0) {
      if (selector->diag_context) {
        coil_diagnostics_reportf(selector->diag_context, COIL_DIAG_ERROR, 
                               COIL_DIAG_CATEGORY_GENERATOR,
                               9, "Failed to select block %u in function '%s'", 
                               i, function->name);
      }
      return -1;
    }
  }
  
  if (selector->verbose) {
    coil_log_info("Completed instruction selection for function '%s'", function->name);
  }
  
  return 0;
}

/* Example pattern matching functions */

/**
 * @brief Match a simple ADD instruction
 * @param inst COIL instruction
 * @param target_context Target context
 * @return 1 if pattern matches, 0 if not
 */
static int match_simple_add(const coil_instruction_t* inst,
                          coil_target_context_t* target_context) {
  /* Check if instruction is a simple ADD instruction */
  if (inst->opcode != COIL_INST_ADD) {
    return 0;
  }
  
  /* Check operand count */
  if (inst->operand_count != 2) {
    return 0;
  }
  
  /* Check operand types */
  if (inst->operands[0].type != COIL_OPERAND_REGISTER ||
      (inst->operands[1].type != COIL_OPERAND_REGISTER && 
       inst->operands[1].type != COIL_OPERAND_IMMEDIATE)) {
    return 0;
  }
  
  /* Check result type */
  if (inst->result.type != COIL_OPERAND_REGISTER) {
    return 0;
  }
  
  /* Pattern matches */
  return 1;
}

/**
 * @brief Match a simple SUB instruction
 * @param inst COIL instruction
 * @param target_context Target context
 * @return 1 if pattern matches, 0 if not
 */
static int match_simple_sub(const coil_instruction_t* inst,
                          coil_target_context_t* target_context) {
  /* Check if instruction is a simple SUB instruction */
  if (inst->opcode != COIL_INST_SUB) {
    return 0;
  }
  
  /* Check operand count */
  if (inst->operand_count != 2) {
    return 0;
  }
  
  /* Check operand types */
  if (inst->operands[0].type != COIL_OPERAND_REGISTER ||
      (inst->operands[1].type != COIL_OPERAND_REGISTER && 
       inst->operands[1].type != COIL_OPERAND_IMMEDIATE)) {
    return 0;
  }
  
  /* Check result type */
  if (inst->result.type != COIL_OPERAND_REGISTER) {
    return 0;
  }
  
  /* Pattern matches */
  return 1;
}

/**
 * @brief Match a simple MUL instruction
 * @param inst COIL instruction
 * @param target_context Target context
 * @return 1 if pattern matches, 0 if not
 */
static int match_simple_mul(const coil_instruction_t* inst,
                          coil_target_context_t* target_context) {
  /* Check if instruction is a simple MUL instruction */
  if (inst->opcode != COIL_INST_MUL) {
    return 0;
  }
  
  /* Check operand count */
  if (inst->operand_count != 2) {
    return 0;
  }
  
  /* Check operand types */
  if (inst->operands[0].type != COIL_OPERAND_REGISTER ||
      (inst->operands[1].type != COIL_OPERAND_REGISTER && 
       inst->operands[1].type != COIL_OPERAND_IMMEDIATE)) {
    return 0;
  }
  
  /* Check result type */
  if (inst->result.type != COIL_OPERAND_REGISTER) {
    return 0;
  }
  
  /* Pattern matches */
  return 1;
}

/**
 * @brief Match a memory load instruction
 * @param inst COIL instruction
 * @param target_context Target context
 * @return 1 if pattern matches, 0 if not
 */
static int match_load(const coil_instruction_t* inst,
                    coil_target_context_t* target_context) {
  /* Check if instruction is a LOAD instruction */
  if (inst->opcode != COIL_INST_LOAD) {
    return 0;
  }
  
  /* Check operand count */
  if (inst->operand_count != 1) {
    return 0;
  }
  
  /* Check operand type */
  if (inst->operands[0].type != COIL_OPERAND_MEMORY) {
    return 0;
  }
  
  /* Check result type */
  if (inst->result.type != COIL_OPERAND_REGISTER) {
    return 0;
  }
  
  /* Pattern matches */
  return 1;
}

/**
 * @brief Match a memory store instruction
 * @param inst COIL instruction
 * @param target_context Target context
 * @return 1 if pattern matches, 0 if not
 */
static int match_store(const coil_instruction_t* inst,
                     coil_target_context_t* target_context) {
  /* Check if instruction is a STORE instruction */
  if (inst->opcode != COIL_INST_STORE) {
    return 0;
  }
  
  /* Check operand count */
  if (inst->operand_count != 2) {
    return 0;
  }
  
  /* Check operand types */
  if (inst->operands[0].type != COIL_OPERAND_MEMORY ||
      inst->operands[1].type != COIL_OPERAND_REGISTER) {
    return 0;
  }
  
  /* Pattern matches */
  return 1;
}

/**
 * @brief Match a conditional branch instruction
 * @param inst COIL instruction
 * @param target_context Target context
 * @return 1 if pattern matches, 0 if not
 */
static int match_conditional_branch(const coil_instruction_t* inst,
                                  coil_target_context_t* target_context) {
  /* Check if instruction is a conditional branch instruction */
  if (inst->opcode != COIL_INST_BR_COND) {
    return 0;
  }
  
  /* Check operand count */
  if (inst->operand_count != 3) {
    return 0;
  }
  
  /* Check operand types */
  if (inst->operands[0].type != COIL_OPERAND_REGISTER ||
      inst->operands[1].type != COIL_OPERAND_BLOCK_REF ||
      inst->operands[2].type != COIL_OPERAND_BLOCK_REF) {
    return 0;
  }
  
  /* Pattern matches */
  return 1;
}

/**
 * @brief Match a function call instruction
 * @param inst COIL instruction
 * @param target_context Target context
 * @return 1 if pattern matches, 0 if not
 */
static int match_call(const coil_instruction_t* inst,
                    coil_target_context_t* target_context) {
  /* Check if instruction is a CALL instruction */
  if (inst->opcode != COIL_INST_CALL) {
    return 0;
  }
  
  /* Check operand types */
  if (inst->operand_count < 1 || inst->operands[0].type != COIL_OPERAND_FUNC_REF) {
    return 0;
  }
  
  /* Pattern matches */
  return 1;
}

/**
 * @brief Register standard instruction patterns
 * @param selector Instruction selector
 * @return 0 on success, non-zero on failure
 */
int instruction_selector_register_standard_patterns(instruction_selector_t* selector) {
  if (!selector) {
    return -1;
  }
  
  /* Get target descriptor */
  const coil_target_descriptor_t* target = coil_target_get_descriptor(
      selector->target_context);
  
  if (!target) {
    if (selector->diag_context) {
      coil_diagnostics_report(selector->diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            10, "Failed to get target descriptor");
    }
    return -1;
  }
  
  /* Register standard patterns
   * Note: We don't provide the select functions here since they would
   * be target-specific. Instead, we use the target's map_instruction function. */
  
  /* Basic arithmetic */
  instruction_selector_add_pattern(selector, 
                                 COIL_INST_ADD, 2, 0, 10,
                                 "SimpleAdd", match_simple_add,
                                 target->map_instruction);
  
  instruction_selector_add_pattern(selector, 
                                 COIL_INST_SUB, 2, 0, 10,
                                 "SimpleSub", match_simple_sub,
                                 target->map_instruction);
  
  instruction_selector_add_pattern(selector, 
                                 COIL_INST_MUL, 2, 0, 10,
                                 "SimpleMul", match_simple_mul,
                                 target->map_instruction);
  
  /* Memory operations */
  instruction_selector_add_pattern(selector, 
                                 COIL_INST_LOAD, 1, 0, 10,
                                 "Load", match_load,
                                 target->map_instruction);
  
  instruction_selector_add_pattern(selector, 
                                 COIL_INST_STORE, 2, 0, 10,
                                 "Store", match_store,
                                 target->map_instruction);
  
  /* Control flow */
  instruction_selector_add_pattern(selector, 
                                 COIL_INST_BR_COND, 3, 0, 10,
                                 "CondBranch", match_conditional_branch,
                                 target->map_instruction);
  
  instruction_selector_add_pattern(selector, 
                                 COIL_INST_CALL, 0, 0, 10,
                                 "Call", match_call,
                                 target->map_instruction);
  
  return 0;
}

/**
 * @brief Get the number of registered patterns
 * @param selector Instruction selector
 * @return Number of patterns or 0 if selector is NULL
 */
uint32_t instruction_selector_get_pattern_count(instruction_selector_t* selector) {
  if (!selector) {
    return 0;
  }
  
  return selector->pattern_count;
}

/**
 * @brief Get a pattern by index
 * @param selector Instruction selector
 * @param index Pattern index
 * @param name Pointer to store pattern name
 * @param opcode Pointer to store pattern opcode
 * @param cost Pointer to store pattern cost
 * @return 0 on success, non-zero on failure
 */
int instruction_selector_get_pattern(instruction_selector_t* selector,
                                   uint32_t index,
                                   const char** name,
                                   uint8_t* opcode,
                                   uint8_t* cost) {
  if (!selector || index >= selector->pattern_count) {
    return -1;
  }
  
  if (name) {
    *name = selector->patterns[index].name;
  }
  
  if (opcode) {
    *opcode = selector->patterns[index].opcode;
  }
  
  if (cost) {
    *cost = selector->patterns[index].cost;
  }
  
  return 0;
}

/**
 * @brief Dump instruction selector information for debugging
 * @param selector Instruction selector
 */
void instruction_selector_dump(instruction_selector_t* selector) {
  if (!selector) {
    return;
  }
  
  coil_log_info("Instruction selector information:");
  coil_log_info("  Pattern count: %u", selector->pattern_count);
  coil_log_info("  Optimized selection: %s", selector->optimize ? "enabled" : "disabled");
  
  if (selector->verbose) {
    coil_log_info("Registered patterns:");
    for (uint32_t i = 0; i < selector->pattern_count; i++) {
      coil_log_info("  Pattern %u: '%s', opcode %u, cost %u",
                   i, selector->patterns[i].name,
                   selector->patterns[i].opcode,
                   selector->patterns[i].cost);
    }
  }
}