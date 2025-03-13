/**
 * @file optimization.c
 * @brief Implementation of optimization framework for the COIL assembler
 * 
 * This module implements the optimization framework for transforming
 * native instructions to improve performance.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <string.h>
#include "optimization.h"
#include "../utils/memory_management.h"
#include "../utils/logging.h"

/**
 * @brief Maximum number of optimization passes
 */
#define MAX_OPT_PASSES 32

/**
 * @brief Optimization pass record
 */
typedef struct {
  optimization_pass_info_t info;          /**< Pass information */
  optimization_pass_t func;               /**< Pass function */
  void* data;                             /**< Pass-specific data */
  uint8_t pass_type;                      /**< Type of pass */
  bool enabled;                           /**< Whether pass is enabled */
  size_t window_size;                     /**< Window size for peephole passes */
} optimization_pass_record_t;

/**
 * @brief Pass types
 */
typedef enum {
  PASS_TYPE_GENERAL = 0,
  PASS_TYPE_FUNCTION = 1,
  PASS_TYPE_BASIC_BLOCK = 2,
  PASS_TYPE_PEEPHOLE = 3,
  PASS_TYPE_SCHEDULING = 4,
  PASS_TYPE_REGISTER_ALLOCATION = 5
} pass_type_t;

/**
 * @brief Basic block information
 */
typedef struct {
  size_t start_index;                     /**< Start index in instruction list */
  size_t end_index;                       /**< End index in instruction list */
} basic_block_info_t;

/**
 * @brief Optimization context implementation
 */
struct optimization_context_t {
  const target_config_t* target;           /**< Target configuration */
  uint32_t opt_level;                      /**< Optimization level */
  optimization_pass_record_t passes[MAX_OPT_PASSES]; /**< Registered passes */
  size_t pass_count;                       /**< Number of passes */
  void* arch_data;                         /**< Architecture-specific data */
};

/**
 * @brief Finds basic blocks in an instruction list
 *
 * @param[in] instructions Instruction list
 * @param[out] blocks Pointer to array of blocks (allocated by this function)
 * @param[out] block_count Pointer to receive number of blocks
 * @return Error code indicating success or failure
 */
static error_t find_basic_blocks(
  const native_instruction_list_t* instructions,
  basic_block_info_t** blocks,
  size_t* block_count
) {
  if (instructions == NULL || blocks == NULL || block_count == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *blocks = NULL;
  *block_count = 0;
  
  if (instructions->count == 0) {
    return ERROR_NONE;
  }
  
  /* Initial capacity for blocks array */
  size_t capacity = 16;
  basic_block_info_t* block_array = memory_alloc(capacity * sizeof(basic_block_info_t));
  if (block_array == NULL) {
    return ERROR_MEMORY;
  }
  
  size_t count = 0;
  size_t block_start = 0;
  
  /* Add initial block */
  block_array[count].start_index = block_start;
  
  /* Scan for control flow instructions that end blocks */
  for (size_t i = 0; i < instructions->count; i++) {
    const native_instruction_t* instr = &instructions->instructions[i];
    bool is_control_flow = false;
    
    /* Check if this is a control flow instruction */
    /* This is a simplified implementation that would need to be expanded
     * for real architectures with more complex control flow. */
    if (strncmp(instr->mnemonic, "jmp", 3) == 0 ||
        strncmp(instr->mnemonic, "ret", 3) == 0 ||
        strncmp(instr->mnemonic, "call", 4) == 0 ||
        (strncmp(instr->mnemonic, "j", 1) == 0 && strlen(instr->mnemonic) > 1)) {
      is_control_flow = true;
    }
    
    if (is_control_flow) {
      /* End current block */
      block_array[count].end_index = i;
      count++;
      
      /* Start new block if this isn't the last instruction */
      if (i + 1 < instructions->count) {
        /* Check if we need to resize the array */
        if (count >= capacity) {
          capacity *= 2;
          basic_block_info_t* new_array = memory_realloc(block_array, capacity * sizeof(basic_block_info_t));
          if (new_array == NULL) {
            memory_free(block_array);
            return ERROR_MEMORY;
          }
          block_array = new_array;
        }
        
        block_start = i + 1;
        block_array[count].start_index = block_start;
      }
    }
  }
  
  /* Close the last block if it wasn't closed by a control flow instruction */
  if (count == 0 || block_array[count - 1].start_index != block_start) {
    block_array[count].end_index = instructions->count - 1;
    count++;
  }
  
  *blocks = block_array;
  *block_count = count;
  
  return ERROR_NONE;
}

error_t optimization_create_context(
  const target_config_t* target,
  uint32_t opt_level,
  optimization_context_t** context
) {
  if (target == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *context = NULL;
  
  /* Allocate optimization context */
  optimization_context_t* new_context = memory_calloc(1, sizeof(optimization_context_t));
  if (new_context == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Initialize context fields */
  new_context->target = target;
  new_context->opt_level = opt_level;
  new_context->pass_count = 0;
  new_context->arch_data = NULL;
  
  /* Initialize architecture-specific optimizations */
  const char* arch = target->architecture;
  
  if (strncmp(arch, "x86", 3) == 0) {
    /* Initialize x86-specific optimization */
    extern error_t x86_optimizer_init(optimization_context_t* context);
    error_t init_result = x86_optimizer_init(new_context);
    if (init_result != ERROR_NONE) {
      memory_free(new_context);
      return init_result;
    }
  } else if (strncmp(arch, "arm", 3) == 0 || strncmp(arch, "aarch", 5) == 0) {
    /* Initialize ARM-specific optimization */
    log_error("ARM optimization not yet implemented");
    memory_free(new_context);
    return ERROR_UNSUPPORTED;
  } else {
    /* Unsupported architecture */
    log_error("Unsupported architecture for optimization: %s", arch);
    memory_free(new_context);
    return ERROR_UNSUPPORTED;
  }
  
  *context = new_context;
  return ERROR_NONE;
}

error_t optimization_destroy_context(optimization_context_t* context) {
  if (context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Cleanup architecture-specific optimization */
  const char* arch = context->target->architecture;
  
  if (strncmp(arch, "x86", 3) == 0) {
    /* Cleanup x86-specific optimization */
    extern error_t x86_optimizer_cleanup(optimization_context_t* context);
    error_t cleanup_result = x86_optimizer_cleanup(context);
    if (cleanup_result != ERROR_NONE) {
      log_warning("Failed to cleanup x86 optimizer: %s", error_message(cleanup_result));
    }
  } else if (strncmp(arch, "arm", 3) == 0 || strncmp(arch, "aarch", 5) == 0) {
    /* Cleanup ARM-specific optimization */
    /* Not implemented yet */
  }
  
  /* Free context */
  memory_free(context);
  
  return ERROR_NONE;
}

error_t optimization_register_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  optimization_pass_t pass_func
) {
  if (context == NULL || pass_info == NULL || pass_func == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Ensure we haven't exceeded maximum pass count */
  if (context->pass_count >= MAX_OPT_PASSES) {
    log_error("Maximum optimization pass count exceeded");
    return ERROR_OPTIMIZATION;
  }
  
  /* Check if pass with this name already exists */
  for (size_t i = 0; i < context->pass_count; i++) {
    if (strcmp(context->passes[i].info.name, pass_info->name) == 0) {
      log_error("Optimization pass with name '%s' already registered", pass_info->name);
      return ERROR_OPTIMIZATION;
    }
  }
  
  /* Register the pass */
  optimization_pass_record_t* pass = &context->passes[context->pass_count];
  pass->info = *pass_info;
  pass->func = pass_func;
  pass->data = NULL;
  pass->pass_type = PASS_TYPE_GENERAL;
  pass->enabled = pass_info->enabled_by_default;
  pass->window_size = 0;
  
  context->pass_count++;
  
  log_debug("Registered optimization pass: %s", pass_info->name);
  
  return ERROR_NONE;
}

error_t optimization_register_function_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  function_optimization_pass_t pass_func
) {
  if (context == NULL || pass_info == NULL || pass_func == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Ensure we haven't exceeded maximum pass count */
  if (context->pass_count >= MAX_OPT_PASSES) {
    log_error("Maximum optimization pass count exceeded");
    return ERROR_OPTIMIZATION;
  }
  
  /* Check if pass with this name already exists */
  for (size_t i = 0; i < context->pass_count; i++) {
    if (strcmp(context->passes[i].info.name, pass_info->name) == 0) {
      log_error("Optimization pass with name '%s' already registered", pass_info->name);
      return ERROR_OPTIMIZATION;
    }
  }
  
  /* Register the pass */
  optimization_pass_record_t* pass = &context->passes[context->pass_count];
  pass->info = *pass_info;
  pass->func = (optimization_pass_t)pass_func;
  pass->data = NULL;
  pass->pass_type = PASS_TYPE_FUNCTION;
  pass->enabled = pass_info->enabled_by_default;
  pass->window_size = 0;
  
  context->pass_count++;
  
  log_debug("Registered function optimization pass: %s", pass_info->name);
  
  return ERROR_NONE;
}

error_t optimization_register_basic_block_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  basic_block_optimization_pass_t pass_func
) {
  if (context == NULL || pass_info == NULL || pass_func == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Ensure we haven't exceeded maximum pass count */
  if (context->pass_count >= MAX_OPT_PASSES) {
    log_error("Maximum optimization pass count exceeded");
    return ERROR_OPTIMIZATION;
  }
  
  /* Check if pass with this name already exists */
  for (size_t i = 0; i < context->pass_count; i++) {
    if (strcmp(context->passes[i].info.name, pass_info->name) == 0) {
      log_error("Optimization pass with name '%s' already registered", pass_info->name);
      return ERROR_OPTIMIZATION;
    }
  }
  
  /* Register the pass */
  optimization_pass_record_t* pass = &context->passes[context->pass_count];
  pass->info = *pass_info;
  pass->func = (optimization_pass_t)pass_func;
  pass->data = NULL;
  pass->pass_type = PASS_TYPE_BASIC_BLOCK;
  pass->enabled = pass_info->enabled_by_default;
  pass->window_size = 0;
  
  context->pass_count++;
  
  log_debug("Registered basic block optimization pass: %s", pass_info->name);
  
  return ERROR_NONE;
}

error_t optimization_register_peephole_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  peephole_optimization_pass_t pass_func,
  size_t window_size
) {
  if (context == NULL || pass_info == NULL || pass_func == NULL || window_size == 0) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Ensure we haven't exceeded maximum pass count */
  if (context->pass_count >= MAX_OPT_PASSES) {
    log_error("Maximum optimization pass count exceeded");
    return ERROR_OPTIMIZATION;
  }
  
  /* Check if pass with this name already exists */
  for (size_t i = 0; i < context->pass_count; i++) {
    if (strcmp(context->passes[i].info.name, pass_info->name) == 0) {
      log_error("Optimization pass with name '%s' already registered", pass_info->name);
      return ERROR_OPTIMIZATION;
    }
  }
  
  /* Register the pass */
  optimization_pass_record_t* pass = &context->passes[context->pass_count];
  pass->info = *pass_info;
  pass->func = (optimization_pass_t)pass_func;
  pass->data = NULL;
  pass->pass_type = PASS_TYPE_PEEPHOLE;
  pass->enabled = pass_info->enabled_by_default;
  pass->window_size = window_size;
  
  context->pass_count++;
  
  log_debug("Registered peephole optimization pass: %s (window size: %zu)", 
           pass_info->name, window_size);
  
  return ERROR_NONE;
}

error_t optimization_register_scheduling_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  scheduling_pass_t pass_func
) {
  if (context == NULL || pass_info == NULL || pass_func == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Ensure we haven't exceeded maximum pass count */
  if (context->pass_count >= MAX_OPT_PASSES) {
    log_error("Maximum optimization pass count exceeded");
    return ERROR_OPTIMIZATION;
  }
  
  /* Check if pass with this name already exists */
  for (size_t i = 0; i < context->pass_count; i++) {
    if (strcmp(context->passes[i].info.name, pass_info->name) == 0) {
      log_error("Optimization pass with name '%s' already registered", pass_info->name);
      return ERROR_OPTIMIZATION;
    }
  }
  
  /* Register the pass */
  optimization_pass_record_t* pass = &context->passes[context->pass_count];
  pass->info = *pass_info;
  pass->func = (optimization_pass_t)pass_func;
  pass->data = NULL;
  pass->pass_type = PASS_TYPE_SCHEDULING;
  pass->enabled = pass_info->enabled_by_default;
  pass->window_size = 0;
  
  context->pass_count++;
  
  log_debug("Registered scheduling optimization pass: %s", pass_info->name);
  
  return ERROR_NONE;
}

error_t optimization_register_register_allocation_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  register_allocation_pass_t pass_func
) {
  if (context == NULL || pass_info == NULL || pass_func == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Ensure we haven't exceeded maximum pass count */
  if (context->pass_count >= MAX_OPT_PASSES) {
    log_error("Maximum optimization pass count exceeded");
    return ERROR_OPTIMIZATION;
  }
  
  /* Check if pass with this name already exists */
  for (size_t i = 0; i < context->pass_count; i++) {
    if (strcmp(context->passes[i].info.name, pass_info->name) == 0) {
      log_error("Optimization pass with name '%s' already registered", pass_info->name);
      return ERROR_OPTIMIZATION;
    }
  }
  
  /* Register the pass */
  optimization_pass_record_t* pass = &context->passes[context->pass_count];
  pass->info = *pass_info;
  pass->func = (optimization_pass_t)pass_func;
  pass->data = NULL;
  pass->pass_type = PASS_TYPE_REGISTER_ALLOCATION;
  pass->enabled = pass_info->enabled_by_default;
  pass->window_size = 0;
  
  context->pass_count++;
  
  log_debug("Registered register allocation optimization pass: %s", pass_info->name);
  
  return ERROR_NONE;
}

error_t optimization_enable_pass(
  optimization_context_t* context,
  const char* pass_name,
  bool enable
) {
  if (context == NULL || pass_name == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Find the pass */
  for (size_t i = 0; i < context->pass_count; i++) {
    if (strcmp(context->passes[i].info.name, pass_name) == 0) {
      context->passes[i].enabled = enable;
      log_debug("%s optimization pass: %s", 
               enable ? "Enabled" : "Disabled", pass_name);
      return ERROR_NONE;
    }
  }
  
  log_error("Optimization pass not found: %s", pass_name);
  return ERROR_NOT_FOUND;
}

error_t optimization_get_pass_info(
  const optimization_context_t* context,
  const char* pass_name,
  const optimization_pass_info_t** pass_info
) {
  if (context == NULL || pass_name == NULL || pass_info == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *pass_info = NULL;
  
  /* Find the pass */
  for (size_t i = 0; i < context->pass_count; i++) {
    if (strcmp(context->passes[i].info.name, pass_name) == 0) {
      *pass_info = &context->passes[i].info;
      return ERROR_NONE;
    }
  }
  
  return ERROR_NOT_FOUND;
}

error_t optimization_get_all_passes(
  const optimization_context_t* context,
  const optimization_pass_info_t*** pass_infos,
  size_t* pass_count
) {
  if (context == NULL || pass_infos == NULL || pass_count == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *pass_infos = NULL;
  *pass_count = 0;
  
  if (context->pass_count == 0) {
    return ERROR_NONE;
  }
  
  /* Allocate array of pointers to pass infos */
  const optimization_pass_info_t** infos = 
    memory_alloc(context->pass_count * sizeof(optimization_pass_info_t*));
  if (infos == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Fill array */
  for (size_t i = 0; i < context->pass_count; i++) {
    infos[i] = &context->passes[i].info;
  }
  
  *pass_infos = infos;
  *pass_count = context->pass_count;
  
  return ERROR_NONE;
}

error_t optimization_apply_all_passes(
  optimization_context_t* context,
  native_instruction_list_t* instructions
) {
  if (context == NULL || instructions == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Find basic blocks for block-level passes */
  basic_block_info_t* blocks = NULL;
  size_t block_count = 0;
  
  error_t block_result = find_basic_blocks(instructions, &blocks, &block_count);
  if (block_result != ERROR_NONE) {
    return block_result;
  }
  
  bool made_changes = false;
  
  /* Apply register allocation passes first */
  for (size_t i = 0; i < context->pass_count; i++) {
    optimization_pass_record_t* pass = &context->passes[i];
    
    /* Skip disabled passes or passes with higher optimization level */
    if (!pass->enabled || pass->info.min_opt_level > context->opt_level) {
      continue;
    }
    
    /* Apply register allocation pass */
    if (pass->pass_type == PASS_TYPE_REGISTER_ALLOCATION) {
      register_allocation_pass_t reg_pass = (register_allocation_pass_t)pass->func;
      error_t result = reg_pass(instructions, context);
      
      if (result != ERROR_NONE && result != ERROR_UNSUPPORTED) {
        log_error("Register allocation pass '%s' failed: %s", 
                 pass->info.name, error_message(result));
        memory_free(blocks);
        return result;
      }
      
      if (result == ERROR_NONE) {
        made_changes = true;
        log_debug("Applied register allocation pass: %s", pass->info.name);
      }
    }
  }
  
  /* Apply general passes */
  for (size_t i = 0; i < context->pass_count; i++) {
    optimization_pass_record_t* pass = &context->passes[i];
    
    /* Skip disabled passes or passes with higher optimization level */
    if (!pass->enabled || pass->info.min_opt_level > context->opt_level) {
      continue;
    }
    
    /* Apply general pass */
    if (pass->pass_type == PASS_TYPE_GENERAL) {
      error_t result = pass->func(instructions, context);
      
      if (result != ERROR_NONE && result != ERROR_UNSUPPORTED) {
        log_error("Optimization pass '%s' failed: %s", 
                 pass->info.name, error_message(result));
        memory_free(blocks);
        return result;
      }
      
      if (result == ERROR_NONE) {
        made_changes = true;
        log_debug("Applied optimization pass: %s", pass->info.name);
      }
    }
  }
  
  /* Apply function-level passes */
  for (size_t i = 0; i < context->pass_count; i++) {
    optimization_pass_record_t* pass = &context->passes[i];
    
    /* Skip disabled passes or passes with higher optimization level */
    if (!pass->enabled || pass->info.min_opt_level > context->opt_level) {
      continue;
    }
    
    /* Apply function pass */
    if (pass->pass_type == PASS_TYPE_FUNCTION) {
      function_optimization_pass_t func_pass = (function_optimization_pass_t)pass->func;
      error_t result = func_pass("main", instructions, context);  /* Assuming "main" function */
      
      if (result != ERROR_NONE && result != ERROR_UNSUPPORTED) {
        log_error("Function optimization pass '%s' failed: %s", 
                 pass->info.name, error_message(result));
        memory_free(blocks);
        return result;
      }
      
      if (result == ERROR_NONE) {
        made_changes = true;
        log_debug("Applied function optimization pass: %s", pass->info.name);
      }
    }
  }
  
  /* Apply basic block passes */
  for (size_t i = 0; i < context->pass_count; i++) {
    optimization_pass_record_t* pass = &context->passes[i];
    
    /* Skip disabled passes or passes with higher optimization level */
    if (!pass->enabled || pass->info.min_opt_level > context->opt_level) {
      continue;
    }
    
    /* Apply basic block pass */
    if (pass->pass_type == PASS_TYPE_BASIC_BLOCK) {
      basic_block_optimization_pass_t block_pass = (basic_block_optimization_pass_t)pass->func;
      
      for (size_t j = 0; j < block_count; j++) {
        error_t result = block_pass(
          instructions,
          blocks[j].start_index,
          blocks[j].end_index,
          context
        );
        
        if (result != ERROR_NONE && result != ERROR_UNSUPPORTED) {
          log_error("Basic block optimization pass '%s' failed: %s", 
                   pass->info.name, error_message(result));
          memory_free(blocks);
          return result;
        }
        
        if (result == ERROR_NONE) {
          made_changes = true;
          log_debug("Applied basic block optimization pass: %s to block %zu", 
                   pass->info.name, j);
        }
      }
    }
    /* Apply scheduling pass */
    else if (pass->pass_type == PASS_TYPE_SCHEDULING) {
      scheduling_pass_t sched_pass = (scheduling_pass_t)pass->func;
      
      for (size_t j = 0; j < block_count; j++) {
        error_t result = sched_pass(
          instructions,
          blocks[j].start_index,
          blocks[j].end_index,
          context
        );
        
        if (result != ERROR_NONE && result != ERROR_UNSUPPORTED) {
          log_error("Scheduling optimization pass '%s' failed: %s", 
                   pass->info.name, error_message(result));
          memory_free(blocks);
          return result;
        }
        
        if (result == ERROR_NONE) {
          made_changes = true;
          log_debug("Applied scheduling optimization pass: %s to block %zu", 
                   pass->info.name, j);
        }
      }
    }
  }
  
  /* Apply peephole passes */
  for (size_t i = 0; i < context->pass_count; i++) {
    optimization_pass_record_t* pass = &context->passes[i];
    
    /* Skip disabled passes or passes with higher optimization level */
    if (!pass->enabled || pass->info.min_opt_level > context->opt_level) {
      continue;
    }
    
    /* Apply peephole pass */
    if (pass->pass_type == PASS_TYPE_PEEPHOLE) {
      peephole_optimization_pass_t peep_pass = (peephole_optimization_pass_t)pass->func;
      
      /* Find all positions where the peephole window fits */
      size_t window_size = pass->window_size;
      if (window_size > instructions->count) {
        continue;  /* Window doesn't fit */
      }
      
      for (size_t pos = 0; pos <= instructions->count - window_size; pos++) {
        error_t result = peep_pass(instructions, pos, window_size, context);
        
        if (result != ERROR_NONE && result != ERROR_UNSUPPORTED) {
          log_error("Peephole optimization pass '%s' failed: %s", 
                   pass->info.name, error_message(result));
          memory_free(blocks);
          return result;
        }
        
        if (result == ERROR_NONE) {
          made_changes = true;
          log_debug("Applied peephole optimization pass: %s at position %zu", 
                   pass->info.name, pos);
        }
      }
    }
  }
  
  /* Free basic blocks */
  memory_free(blocks);
  
  log_info("Applied %zu optimization passes, made changes: %s",
          context->pass_count, made_changes ? "yes" : "no");
  
  return ERROR_NONE;
}

error_t optimization_apply_pass(
  optimization_context_t* context,
  const char* pass_name,
  native_instruction_list_t* instructions
) {
  if (context == NULL || pass_name == NULL || instructions == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Find the pass */
  size_t pass_index = 0;
  bool found = false;
  
  for (size_t i = 0; i < context->pass_count; i++) {
    if (strcmp(context->passes[i].info.name, pass_name) == 0) {
      pass_index = i;
      found = true;
      break;
    }
  }
  
  if (!found) {
    log_error("Optimization pass not found: %s", pass_name);
    return ERROR_NOT_FOUND;
  }
  
  optimization_pass_record_t* pass = &context->passes[pass_index];
  
  /* Skip disabled passes or passes with higher optimization level */
  if (!pass->enabled || pass->info.min_opt_level > context->opt_level) {
    log_warning("Optimization pass '%s' is disabled or requires higher optimization level",
               pass_name);
    return ERROR_UNSUPPORTED;
  }
  
  /* Apply the pass based on its type */
  switch (pass->pass_type) {
    case PASS_TYPE_GENERAL:
      return pass->func(instructions, context);
    
    case PASS_TYPE_FUNCTION:
      return ((function_optimization_pass_t)pass->func)("main", instructions, context);
    
    case PASS_TYPE_BASIC_BLOCK:
    case PASS_TYPE_SCHEDULING: {
      /* Find basic blocks */
      basic_block_info_t* blocks = NULL;
      size_t block_count = 0;
      
      error_t block_result = find_basic_blocks(instructions, &blocks, &block_count);
      if (block_result != ERROR_NONE) {
        return block_result;
      }
      
      error_t result = ERROR_UNSUPPORTED;
      
      /* Apply to each block */
      for (size_t j = 0; j < block_count; j++) {
        if (pass->pass_type == PASS_TYPE_BASIC_BLOCK) {
          result = ((basic_block_optimization_pass_t)pass->func)(
            instructions,
            blocks[j].start_index,
            blocks[j].end_index,
            context
          );
        } else {  /* PASS_TYPE_SCHEDULING */
          result = ((scheduling_pass_t)pass->func)(
            instructions,
            blocks[j].start_index,
            blocks[j].end_index,
            context
          );
        }
        
        if (result != ERROR_NONE && result != ERROR_UNSUPPORTED) {
          memory_free(blocks);
          return result;
        }
      }
      
      memory_free(blocks);
      return result;
    }
    
    case PASS_TYPE_PEEPHOLE: {
      /* Find all positions where the peephole window fits */
      size_t window_size = pass->window_size;
      if (window_size > instructions->count) {
        return ERROR_UNSUPPORTED;  /* Window doesn't fit */
      }
      
      error_t result = ERROR_UNSUPPORTED;
      
      for (size_t pos = 0; pos <= instructions->count - window_size; pos++) {
        error_t pos_result = ((peephole_optimization_pass_t)pass->func)(
          instructions,
          pos,
          window_size,
          context
        );
        
        if (pos_result != ERROR_NONE && pos_result != ERROR_UNSUPPORTED) {
          return pos_result;
        }
        
        if (pos_result == ERROR_NONE) {
          result = ERROR_NONE;
        }
      }
      
      return result;
    }
    
    case PASS_TYPE_REGISTER_ALLOCATION:
      return ((register_allocation_pass_t)pass->func)(instructions, context);
    
    default:
      log_error("Unknown pass type: %u", pass->pass_type);
      return ERROR_INTERNAL;
  }
}

error_t optimization_set_level(
  optimization_context_t* context,
  uint32_t opt_level
) {
  if (context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  context->opt_level = opt_level;
  
  return ERROR_NONE;
}

uint32_t optimization_get_level(const optimization_context_t* context) {
  if (context == NULL) {
    return 0;
  }
  
  return context->opt_level;
}

error_t optimization_set_arch_data(
  optimization_context_t* context,
  void* arch_data
) {
  if (context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  context->arch_data = arch_data;
  
  return ERROR_NONE;
}

void* optimization_get_arch_data(const optimization_context_t* context) {
  if (context == NULL) {
    return NULL;
  }
  
  return context->arch_data;
}