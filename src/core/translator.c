/**
 * @file translator.c
 * @brief Implementation of instruction translation framework
 * 
 * This module implements the framework for translating COIL instructions
 * to native instructions for specific target architectures.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <string.h>
#include "translator.h"
#include "../utils/memory_management.h"
#include "../utils/logging.h"

/**
 * @brief Maximum number of registered translators per category
 */
#define MAX_TRANSLATORS_PER_CATEGORY 16

/**
 * @brief Instruction translator registry
 */
typedef struct {
  instruction_translator_t translators[COIL_CAT_SPECIAL + 1][MAX_TRANSLATORS_PER_CATEGORY];
  uint8_t translator_count[COIL_CAT_SPECIAL + 1];
  bool initialized;
} translator_registry_t;

/**
 * @brief Global translator registry
 */
static translator_registry_t registry = {
  .initialized = false
};

/**
 * @brief Initializes the translator registry
 *
 * @return Error code indicating success or failure
 */
static error_t initialize_registry() {
  if (registry.initialized) {
    return ERROR_NONE;
  }
  
  /* Clear the registry */
  memset(registry.translators, 0, sizeof(registry.translators));
  memset(registry.translator_count, 0, sizeof(registry.translator_count));
  
  registry.initialized = true;
  return ERROR_NONE;
}

/**
 * @brief Registers an instruction translator for a specific category
 *
 * @param[in] category Instruction category
 * @param[in] translator Translator function
 * @return Error code indicating success or failure
 */
static error_t register_translator(
  uint8_t category,
  instruction_translator_t translator
) {
  if (category > COIL_CAT_SPECIAL || translator == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Initialize registry if needed */
  if (!registry.initialized) {
    error_t init_result = initialize_registry();
    if (init_result != ERROR_NONE) {
      return init_result;
    }
  }
  
  /* Check if we've reached the maximum number of translators for this category */
  if (registry.translator_count[category] >= MAX_TRANSLATORS_PER_CATEGORY) {
    log_error("Maximum number of translators for category %u exceeded", category);
    return ERROR_UNSUPPORTED;
  }
  
  /* Register the translator */
  registry.translators[category][registry.translator_count[category]++] = translator;
  
  return ERROR_NONE;
}

/**
 * @brief Finds and calls an appropriate translator for an instruction
 *
 * @param[in] instruction COIL instruction to translate
 * @param[in,out] context Translation context
 * @return Error code indicating success or failure
 */
static error_t find_and_call_translator(
  const coil_instruction_t* instruction,
  translator_context_t* context
) {
  if (instruction == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Determine instruction category */
  uint8_t category = 0;
  
  /* In a real implementation, we would extract the category from the instruction
   * For now, we'll use the opcode's high bits as a simple category indicator
   */
  category = (instruction->opcode >> 4) & 0x0F;
  
  /* Ensure category is valid */
  if (category > COIL_CAT_SPECIAL) {
    log_error("Invalid instruction category: %u", category);
    return ERROR_INVALID_FORMAT;
  }
  
  /* Try each registered translator for this category */
  for (uint8_t i = 0; i < registry.translator_count[category]; i++) {
    instruction_translator_t translator = registry.translators[category][i];
    
    /* Skip NULL translators (shouldn't happen, but just in case) */
    if (translator == NULL) {
      continue;
    }
    
    /* Try to translate the instruction */
    error_t translate_result = translator(instruction, context);
    
    /* If the translator succeeded or returned a specific error, return it */
    if (translate_result != ERROR_UNSUPPORTED) {
      return translate_result;
    }
    
    /* Otherwise, try the next translator */
  }
  
  /* No suitable translator found */
  log_error("No suitable translator found for instruction with opcode 0x%02X", instruction->opcode);
  return ERROR_UNSUPPORTED;
}

error_t translator_create_context(
  const target_config_t* target,
  translator_context_t** context
) {
  if (target == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *context = NULL;
  
  /* Allocate context structure */
  translator_context_t* new_context = memory_calloc(1, sizeof(translator_context_t));
  if (new_context == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Initialize context fields */
  new_context->target = target;
  new_context->register_allocator = NULL;  /* Will be initialized by target-specific code */
  new_context->arch_specific = NULL;  /* Will be initialized by target-specific code */
  
  /* Create output instruction list */
  native_instruction_list_t* output = NULL;
  error_t list_result = translator_create_instruction_list(256, &output);
  if (list_result != ERROR_NONE) {
    memory_free(new_context);
    return list_result;
  }
  
  new_context->output = output;
  
  /* Initialize target-specific context */
  const char* arch = target->architecture;
  
  if (strncmp(arch, "x86", 3) == 0) {
    /* Initialize x86-specific translator context */
    /* This would typically be handled by a function in x86_translator.c */
    log_info("Initializing x86 translator context");
    
    /* For now, we'll defer to the x86_translator_init function, which
     * we'll assume is defined in the x86-specific implementation
     */
    extern error_t x86_translator_init(translator_context_t* context);
    error_t init_result = x86_translator_init(new_context);
    if (init_result != ERROR_NONE) {
      translator_free_instruction_list(new_context->output);
      memory_free(new_context);
      return init_result;
    }
  } else if (strncmp(arch, "arm", 3) == 0 || strncmp(arch, "aarch", 5) == 0) {
    /* Initialize ARM-specific translator context */
    /* This would typically be handled by a function in arm_translator.c */
    log_info("Initializing ARM translator context");
    
    /* For now, we'll just log that ARM translation is not yet implemented */
    log_error("ARM translation not yet implemented");
    translator_free_instruction_list(new_context->output);
    memory_free(new_context);
    return ERROR_UNSUPPORTED;
  } else {
    /* Unsupported architecture */
    log_error("Unsupported architecture: %s", arch);
    translator_free_instruction_list(new_context->output);
    memory_free(new_context);
    return ERROR_UNSUPPORTED;
  }
  
  *context = new_context;
  return ERROR_NONE;
}

error_t translator_destroy_context(translator_context_t* context) {
  if (context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Destroy architecture-specific context */
  const char* arch = context->target->architecture;
  
  if (strncmp(arch, "x86", 3) == 0) {
    /* Cleanup x86-specific translator context */
    /* This would typically be handled by a function in x86_translator.c */
    extern error_t x86_translator_cleanup(translator_context_t* context);
    error_t cleanup_result = x86_translator_cleanup(context);
    if (cleanup_result != ERROR_NONE) {
      log_warning("Failed to cleanup x86 translator context: %s",
                 error_message(cleanup_result));
    }
  } else if (strncmp(arch, "arm", 3) == 0 || strncmp(arch, "aarch", 5) == 0) {
    /* Cleanup ARM-specific translator context */
    /* Not implemented yet */
  }
  
  /* Free instruction list */
  translator_free_instruction_list(context->output);
  
  /* Free context structure */
  memory_free(context);
  
  return ERROR_NONE;
}

error_t translator_translate_instruction(
  const coil_instruction_t* instruction,
  translator_context_t* context
) {
  if (instruction == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  return find_and_call_translator(instruction, context);
}

error_t translator_translate_function(
  const coil_function_t* function,
  translator_context_t* context
) {
  if (function == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Reset the output instruction list */
  context->output->count = 0;
  
  /* Process each basic block */
  for (uint32_t i = 0; i < function->block_count; i++) {
    coil_basic_block_t* block = function->blocks[i];
    
    /* Add a label for the block if it has one */
    if (block->label != NULL) {
      /* In a real implementation, we would add a label instruction to the output
       * For simplicity, we'll just log the label for now
       */
      log_debug("Translating block with label: %s", block->label);
    }
    
    /* Translate each instruction in the block */
    for (uint32_t j = 0; j < block->instruction_count; j++) {
      error_t translate_result = translator_translate_instruction(
        &block->instructions[j],
        context
      );
      
      if (translate_result != ERROR_NONE) {
        log_error("Failed to translate instruction %u in block %u of function %s: %s",
                 j, i, function->name, error_message(translate_result));
        return translate_result;
      }
    }
  }
  
  log_info("Successfully translated function %s with %u instructions",
          function->name, context->output->count);
  
  return ERROR_NONE;
}

error_t translator_create_instruction_list(
  size_t initial_capacity,
  native_instruction_list_t** list
) {
  if (initial_capacity == 0 || list == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *list = NULL;
  
  /* Allocate list structure */
  native_instruction_list_t* new_list = memory_calloc(1, sizeof(native_instruction_list_t));
  if (new_list == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Allocate instruction array */
  new_list->instructions = memory_calloc(initial_capacity, sizeof(native_instruction_t));
  if (new_list->instructions == NULL) {
    memory_free(new_list);
    return ERROR_MEMORY;
  }
  
  new_list->count = 0;
  new_list->capacity = initial_capacity;
  
  *list = new_list;
  return ERROR_NONE;
}

error_t translator_add_instruction(
  native_instruction_list_t* list,
  const native_instruction_t* instruction
) {
  if (list == NULL || instruction == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if we need to resize the instruction array */
  if (list->count >= list->capacity) {
    size_t new_capacity = list->capacity * 2;
    native_instruction_t* new_instructions = memory_realloc(
      list->instructions,
      new_capacity * sizeof(native_instruction_t)
    );
    
    if (new_instructions == NULL) {
      return ERROR_MEMORY;
    }
    
    list->instructions = new_instructions;
    list->capacity = new_capacity;
  }
  
  /* Copy the instruction to the list */
  memcpy(&list->instructions[list->count], instruction, sizeof(native_instruction_t));
  list->count++;
  
  return ERROR_NONE;
}

error_t translator_free_instruction_list(native_instruction_list_t* list) {
  if (list == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Free instruction array */
  memory_free(list->instructions);
  
  /* Free list structure */
  memory_free(list);
  
  return ERROR_NONE;
}

error_t translator_map_register(
  uint32_t coil_reg,
  translator_context_t* context,
  native_operand_t* native_reg
) {
  if (context == NULL || native_reg == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Register mapping is target-specific, defer to architecture-specific implementation */
  const char* arch = context->target->architecture;
  
  if (strncmp(arch, "x86", 3) == 0) {
    /* Use x86-specific register mapping */
    /* This would typically be handled by a function in x86_translator.c */
    extern error_t x86_map_register(uint32_t coil_reg, translator_context_t* context, native_operand_t* native_reg);
    return x86_map_register(coil_reg, context, native_reg);
  } else if (strncmp(arch, "arm", 3) == 0 || strncmp(arch, "aarch", 5) == 0) {
    /* Use ARM-specific register mapping */
    /* Not implemented yet */
    log_error("ARM register mapping not yet implemented");
    return ERROR_UNSUPPORTED;
  } else {
    /* Unsupported architecture */
    log_error("Unsupported architecture for register mapping: %s", arch);
    return ERROR_UNSUPPORTED;
  }
}

error_t translator_map_operand(
  const coil_operand_t* coil_operand,
  translator_context_t* context,
  native_operand_t* native_operand
) {
  if (coil_operand == NULL || context == NULL || native_operand == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Initialize native operand */
  memset(native_operand, 0, sizeof(native_operand_t));
  
  /* Map operand based on type */
  switch (coil_operand->type) {
    case COIL_OPERAND_REGISTER:
      return translator_map_register(coil_operand->value.reg_id, context, native_operand);
    
    case COIL_OPERAND_IMMEDIATE:
      native_operand->type = NATIVE_OPERAND_IMMEDIATE;
      native_operand->value.imm = coil_operand->value.imm_value;
      
      /* Determine size based on data type */
      /* This is a simplified implementation */
      switch (coil_operand->data_type) {
        case 1:  /* Assuming 1 is 8-bit integer */
          native_operand->size = 1;
          break;
        case 2:  /* Assuming 2 is 16-bit integer */
          native_operand->size = 2;
          break;
        case 3:  /* Assuming 3 is 32-bit integer */
          native_operand->size = 4;
          break;
        case 4:  /* Assuming 4 is 64-bit integer */
          native_operand->size = 8;
          break;
        case 5:  /* Assuming 5 is 32-bit float */
          native_operand->size = 4;
          break;
        case 6:  /* Assuming 6 is 64-bit float */
          native_operand->size = 8;
          break;
        default:
          /* Default to 4 bytes */
          native_operand->size = 4;
          break;
      }
      
      return ERROR_NONE;
    
    case COIL_OPERAND_MEMORY:
      native_operand->type = NATIVE_OPERAND_MEMORY;
      
      /* Map base register */
      native_operand_t base_reg;
      if (coil_operand->value.mem.base_reg != 0) {
        error_t base_result = translator_map_register(
          coil_operand->value.mem.base_reg,
          context,
          &base_reg
        );
        
        if (base_result != ERROR_NONE) {
          return base_result;
        }
        
        native_operand->value.mem.base_reg = base_reg.value.reg.id;
      } else {
        native_operand->value.mem.base_reg = 0;
      }
      
      /* Map index register */
      native_operand_t index_reg;
      if (coil_operand->value.mem.index_reg != 0) {
        error_t index_result = translator_map_register(
          coil_operand->value.mem.index_reg,
          context,
          &index_reg
        );
        
        if (index_result != ERROR_NONE) {
          return index_result;
        }
        
        native_operand->value.mem.index_reg = index_reg.value.reg.id;
      } else {
        native_operand->value.mem.index_reg = 0;
      }
      
      /* Copy displacement and scale */
      native_operand->value.mem.displacement = coil_operand->value.mem.displacement;
      native_operand->value.mem.scale = coil_operand->value.mem.scale;
      
      /* Set segment (x86-specific, defaults to 0) */
      native_operand->value.mem.segment = 0;
      
      /* Determine size based on data type */
      /* Same as immediate operand sizing */
      switch (coil_operand->data_type) {
        case 1:  /* Assuming 1 is 8-bit integer */
          native_operand->size = 1;
          break;
        case 2:  /* Assuming 2 is 16-bit integer */
          native_operand->size = 2;
          break;
        case 3:  /* Assuming 3 is 32-bit integer */
          native_operand->size = 4;
          break;
        case 4:  /* Assuming 4 is 64-bit integer */
          native_operand->size = 8;
          break;
        case 5:  /* Assuming 5 is 32-bit float */
          native_operand->size = 4;
          break;
        case 6:  /* Assuming 6 is 64-bit float */
          native_operand->size = 8;
          break;
        default:
          /* Default to 4 bytes */
          native_operand->size = 4;
          break;
      }
      
      return ERROR_NONE;
    
    case COIL_OPERAND_LABEL:
      native_operand->type = NATIVE_OPERAND_LABEL;
      native_operand->value.label.id = coil_operand->value.label_id;
      
      /* We'd normally convert the label ID to a name here,
       * but that requires additional context we don't have
       * in this simplified implementation.
       */
      snprintf(native_operand->value.label.name, sizeof(native_operand->value.label.name),
              "L%u", coil_operand->value.label_id);
      
      return ERROR_NONE;
    
    default:
      log_error("Unknown operand type: %u", coil_operand->type);
      return ERROR_INVALID_FORMAT;
  }
}