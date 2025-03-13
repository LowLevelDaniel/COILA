/**
 * @file test_optimization.c
 * @brief Tests for the optimization framework
 * 
 * This file contains tests for the optimization framework of the COIL assembler.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "../src/core/optimization.h"
#include "../src/core/translator.h"
#include "../src/utils/memory_management.h"

/**
 * @brief Simple optimization pass that increments a counter
 *
 * @param[in,out] instructions Instructions to optimize
 * @param[in,out] context Optimization context
 * @return ERROR_NONE to indicate a change, ERROR_UNSUPPORTED otherwise
 */
static error_t test_pass_increment(
  native_instruction_list_t* instructions,
  optimization_context_t* context
) {
  if (instructions == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Increment count stored in arch_data */
  int* count = (int*)optimization_get_arch_data(context);
  if (count != NULL) {
    (*count)++;
    return ERROR_NONE;  /* Report a change */
  }
  
  return ERROR_UNSUPPORTED;
}

/**
 * @brief Simple optimization pass that changes an instruction
 *
 * @param[in,out] instructions Instructions to optimize
 * @param[in,out] context Optimization context
 * @return ERROR_NONE to indicate a change, ERROR_UNSUPPORTED otherwise
 */
static error_t test_pass_modify(
  native_instruction_list_t* instructions,
  optimization_context_t* context
) {
  if (instructions == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if there are any instructions */
  if (instructions->count == 0) {
    return ERROR_UNSUPPORTED;
  }
  
  /* Modify the first instruction's mnemonic */
  strcpy(instructions->instructions[0].mnemonic, "modified");
  
  return ERROR_NONE;  /* Report a change */
}

/**
 * @brief Tests optimization context creation and management
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
static bool test_optimization_context(test_results_t* results) {
  bool success = true;
  
  /* Create a dummy target configuration */
  target_config_t* target = NULL;
  error_t target_result = target_config_create_empty(&target);
  
  if (!test_assert_error(results, target_result, ERROR_NONE, "Create target config")) {
    success = false;
    return success;
  }
  
  /* Set target properties */
  target->name = memory_alloc(32);
  target->architecture = memory_alloc(32);
  target->vendor = memory_alloc(32);
  
  if (!test_assert(results, target->name != NULL && target->architecture != NULL && target->vendor != NULL,
                 "Allocate target strings")) {
    success = false;
    goto cleanup_target;
  }
  
  strcpy(target->name, "x86_64-test");
  strcpy(target->architecture, "x86_64");
  strcpy(target->vendor, "test");
  
  /* Add x86_64 feature */
  target->features = memory_calloc(1, sizeof(char*));
  if (!test_assert(results, target->features != NULL, "Allocate features array")) {
    success = false;
    goto cleanup_target;
  }
  
  target->features[0] = memory_alloc(32);
  if (!test_assert(results, target->features[0] != NULL, "Allocate feature string")) {
    success = false;
    goto cleanup_target;
  }
  
  strcpy(target->features[0], "x86_64");
  target->feature_count = 1;
  
  /* Create optimization context */
  optimization_context_t* context = NULL;
  error_t context_result = optimization_create_context(target, 1, &context);
  
  if (!test_assert_error(results, context_result, ERROR_NONE, "Create optimization context")) {
    success = false;
    goto cleanup_target;
  }
  
  /* Test optimization level getter/setter */
  uint32_t level = optimization_get_level(context);
  
  if (!test_assert_int(results, level, 1, "Initial optimization level")) {
    success = false;
  }
  
  error_t set_level_result = optimization_set_level(context, 2);
  
  if (!test_assert_error(results, set_level_result, ERROR_NONE, "Set optimization level")) {
    success = false;
  }
  
  level = optimization_get_level(context);
  
  if (!test_assert_int(results, level, 2, "Updated optimization level")) {
    success = false;
  }
  
  /* Test architecture-specific data getter/setter */
  int* count = memory_alloc(sizeof(int));
  if (!test_assert(results, count != NULL, "Allocate arch data")) {
    success = false;
    goto cleanup_context;
  }
  
  *count = 0;
  
  error_t set_data_result = optimization_set_arch_data(context, count);
  
  if (!test_assert_error(results, set_data_result, ERROR_NONE, "Set arch data")) {
    success = false;
    memory_free(count);
    goto cleanup_context;
  }
  
  void* arch_data = optimization_get_arch_data(context);
  
  if (!test_assert(results, arch_data == count, "Get arch data")) {
    success = false;
  }
  
  /* Test cleanup */
  error_t destroy_result = optimization_destroy_context(context);
  
  if (!test_assert_error(results, destroy_result, ERROR_NONE, "Destroy context")) {
    success = false;
    memory_free(count);  /* Free manually since context failed to destroy */
  }
  
  goto cleanup_target;
  
cleanup_context:
  optimization_destroy_context(context);
  
cleanup_target:
  target_config_free(target);
  return success;
}

/**
 * @brief Tests optimization pass registration and management
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
static bool test_optimization_passes(test_results_t* results) {
  bool success = true;
  
  /* Create a dummy target configuration */
  target_config_t* target = NULL;
  error_t target_result = target_config_create_empty(&target);
  
  if (!test_assert_error(results, target_result, ERROR_NONE, "Create target config")) {
    success = false;
    return success;
  }
  
  /* Set target properties */
  target->name = memory_alloc(32);
  target->architecture = memory_alloc(32);
  target->vendor = memory_alloc(32);
  
  if (!test_assert(results, target->name != NULL && target->architecture != NULL && target->vendor != NULL,
                 "Allocate target strings")) {
    success = false;
    goto cleanup_target;
  }
  
  strcpy(target->name, "x86_64-test");
  strcpy(target->architecture, "x86_64");
  strcpy(target->vendor, "test");
  
  /* Add x86_64 feature */
  target->features = memory_calloc(1, sizeof(char*));
  if (!test_assert(results, target->features != NULL, "Allocate features array")) {
    success = false;
    goto cleanup_target;
  }
  
  target->features[0] = memory_alloc(32);
  if (!test_assert(results, target->features[0] != NULL, "Allocate feature string")) {
    success = false;
    goto cleanup_target;
  }
  
  strcpy(target->features[0], "x86_64");
  target->feature_count = 1;
  
  /* Create optimization context */
  optimization_context_t* context = NULL;
  error_t context_result = optimization_create_context(target, 1, &context);
  
  if (!test_assert_error(results, context_result, ERROR_NONE, "Create optimization context")) {
    success = false;
    goto cleanup_target;
  }
  
  /* Set architecture-specific data for pass testing */
  int* count = memory_alloc(sizeof(int));
  if (!test_assert(results, count != NULL, "Allocate arch data")) {
    success = false;
    goto cleanup_context;
  }
  
  *count = 0;
  
  error_t set_data_result = optimization_set_arch_data(context, count);
  
  if (!test_assert_error(results, set_data_result, ERROR_NONE, "Set arch data")) {
    success = false;
    memory_free(count);
    goto cleanup_context;
  }
  
  /* Register a test optimization pass */
  optimization_pass_info_t pass_info = {
    "test_increment",
    "Test pass that increments a counter",
    1,
    true
  };
  
  error_t register_result = optimization_register_pass(
    context,
    &pass_info,
    test_pass_increment
  );
  
  if (!test_assert_error(results, register_result, ERROR_NONE, "Register test pass")) {
    success = false;
    goto cleanup_context;
  }
  
  /* Register a second test pass */
  optimization_pass_info_t pass_info2 = {
    "test_modify",
    "Test pass that modifies an instruction",
    1,
    true
  };
  
  register_result = optimization_register_pass(
    context,
    &pass_info2,
    test_pass_modify
  );
  
  if (!test_assert_error(results, register_result, ERROR_NONE, "Register second test pass")) {
    success = false;
    goto cleanup_context;
  }
  
  /* Get pass info */
  const optimization_pass_info_t* retrieved_info = NULL;
  error_t get_info_result = optimization_get_pass_info(
    context,
    "test_increment",
    &retrieved_info
  );
  
  if (!test_assert_error(results, get_info_result, ERROR_NONE, "Get pass info")) {
    success = false;
  } else {
    if (!test_assert_string(results, retrieved_info->name, "test_increment", "Pass name")) {
      success = false;
    }
    
    if (!test_assert_string(results, retrieved_info->description, "Test pass that increments a counter", "Pass description")) {
      success = false;
    }
  }
  
  /* Get all passes */
  const optimization_pass_info_t** all_passes = NULL;
  size_t pass_count = 0;
  
  error_t get_all_result = optimization_get_all_passes(
    context,
    &all_passes,
    &pass_count
  );
  
  if (!test_assert_error(results, get_all_result, ERROR_NONE, "Get all passes")) {
    success = false;
  } else {
    if (!test_assert_int(results, pass_count, 2, "Pass count")) {
      success = false;
    }
    
    memory_free(all_passes);
  }
  
  /* Enable/disable pass */
  error_t enable_result = optimization_enable_pass(
    context,
    "test_increment",
    false
  );
  
  if (!test_assert_error(results, enable_result, ERROR_NONE, "Disable pass")) {
    success = false;
  }
  
  /* Create a sample instruction list */
  native_instruction_list_t* instructions = NULL;
  error_t create_list_result = translator_create_instruction_list(10, &instructions);
  
  if (!test_assert_error(results, create_list_result, ERROR_NONE, "Create instruction list")) {
    success = false;
    goto cleanup_context;
  }
  
  /* Add a sample instruction */
  native_instruction_t instr;
  memset(&instr, 0, sizeof(native_instruction_t));
  strcpy(instr.mnemonic, "test");
  instr.operand_count = 0;
  
  error_t add_instr_result = translator_add_instruction(instructions, &instr);
  
  if (!test_assert_error(results, add_instr_result, ERROR_NONE, "Add instruction")) {
    success = false;
    goto cleanup_instructions;
  }
  
  /* Apply a specific pass */
  error_t apply_result = optimization_apply_pass(
    context,
    "test_modify",
    instructions
  );
  
  if (!test_assert_error(results, apply_result, ERROR_NONE, "Apply specific pass")) {
    success = false;
  } else {
    if (!test_assert_string(results, instructions->instructions[0].mnemonic, "modified", "Modified mnemonic")) {
      success = false;
    }
  }
  
  /* Check that disabled passes are not applied */
  apply_result = optimization_apply_pass(
    context,
    "test_increment",
    instructions
  );
  
  if (!test_assert_error(results, apply_result, ERROR_UNSUPPORTED, "Apply disabled pass")) {
    success = false;
  }
  
  /* Re-enable the pass */
  enable_result = optimization_enable_pass(
    context,
    "test_increment",
    true
  );
  
  if (!test_assert_error(results, enable_result, ERROR_NONE, "Enable pass")) {
    success = false;
  }
  
  /* Apply all passes */
  error_t apply_all_result = optimization_apply_all_passes(
    context,
    instructions
  );
  
  if (!test_assert_error(results, apply_all_result, ERROR_NONE, "Apply all passes")) {
    success = false;
  }
  
  /* Check that counter was incremented */
  if (!test_assert_int(results, *count, 1, "Incremented counter")) {
    success = false;
  }
  
cleanup_instructions:
  translator_free_instruction_list(instructions);
  
cleanup_context:
  optimization_destroy_context(context);  /* This will free count as well */
  
cleanup_target:
  target_config_free(target);
  return success;
}

/**
 * @brief Main test function for optimization framework
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
bool test_optimization(test_results_t* results) {
  bool success = true;
  
  /* Initialize memory management */
  memory_init(true);
  
  /* Run sub-tests */
  if (!test_optimization_context(results)) {
    success = false;
  }
  
  if (!test_optimization_passes(results)) {
    success = false;
  }
  
  /* Cleanup memory management */
  memory_shutdown();
  
  return success;
}