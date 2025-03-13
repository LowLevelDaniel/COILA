/**
 * @file test_x86_translator.c
 * @brief Tests for the x86 translator
 * 
 * This file contains tests for the x86-specific translator module
 * of the COIL assembler.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "../src/targets/x86/x86_translator.h"
#include "../src/core/translator.h"
#include "../src/core/instruction_decoder.h"
#include "../src/utils/memory_management.h"

/**
 * @brief Creates a mock COIL instruction for testing
 *
 * @param[out] instruction Pointer to receive the instruction
 * @param[in] opcode Instruction opcode
 * @param[in] dest Destination register
 * @param[in] operand_count Number of operands
 */
static void create_mock_instruction(
  coil_instruction_t* instruction,
  uint8_t opcode,
  uint8_t dest,
  uint8_t operand_count
) {
  if (instruction == NULL) {
    return;
  }
  
  /* Initialize instruction */
  memset(instruction, 0, sizeof(coil_instruction_t));
  
  instruction->opcode = opcode;
  instruction->destination = dest;
  instruction->operand_count = operand_count;
  instruction->data_type = 3;  /* 32-bit data */
  
  /* Set operands based on opcode and count */
  switch (opcode) {
    case COIL_OP_ADD:
      if (operand_count >= 2) {
        /* First operand (already in destination) */
        instruction->operands[0].type = COIL_OPERAND_REGISTER;
        instruction->operands[0].data_type = 3;  /* 32-bit data */
        instruction->operands[0].value.reg_id = dest;
        
        /* Second operand */
        instruction->operands[1].type = COIL_OPERAND_REGISTER;
        instruction->operands[1].data_type = 3;  /* 32-bit data */
        instruction->operands[1].value.reg_id = 1;  /* Use ECX/RCX */
      }
      break;
    
    case COIL_OP_SUB:
      if (operand_count >= 2) {
        /* First operand (already in destination) */
        instruction->operands[0].type = COIL_OPERAND_REGISTER;
        instruction->operands[0].data_type = 3;  /* 32-bit data */
        instruction->operands[0].value.reg_id = dest;
        
        /* Second operand */
        instruction->operands[1].type = COIL_OPERAND_IMMEDIATE;
        instruction->operands[1].data_type = 3;  /* 32-bit data */
        instruction->operands[1].value.imm_value = 42;
      }
      break;
    
    case COIL_OP_MUL:
      if (operand_count >= 2) {
        /* First operand (already in destination) */
        instruction->operands[0].type = COIL_OPERAND_REGISTER;
        instruction->operands[0].data_type = 3;  /* 32-bit data */
        instruction->operands[0].value.reg_id = dest;
        
        /* Second operand */
        instruction->operands[1].type = COIL_OPERAND_REGISTER;
        instruction->operands[1].data_type = 3;  /* 32-bit data */
        instruction->operands[1].value.reg_id = 2;  /* Use EDX/RDX */
      }
      break;
    
    case 0x01:  /* Assuming 0x01 is LOAD in memory category */
      if (operand_count >= 1) {
        /* Memory operand */
        instruction->operands[0].type = COIL_OPERAND_MEMORY;
        instruction->operands[0].data_type = 3;  /* 32-bit data */
        instruction->operands[0].value.mem.base_reg = 3;  /* Use EBX/RBX */
        instruction->operands[0].value.mem.index_reg = 0;
        instruction->operands[0].value.mem.scale = 1;
        instruction->operands[0].value.mem.displacement = 8;
      }
      break;
    
    case 0x03:  /* Assuming 0x03 is CALL in control category */
      if (operand_count >= 1) {
        /* Label operand */
        instruction->operands[0].type = COIL_OPERAND_LABEL;
        instruction->operands[0].data_type = 0;
        instruction->operands[0].value.label_id = 1;
      }
      break;
  }
}

/**
 * @brief Tests x86 register handling
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
static bool test_x86_registers(test_results_t* results) {
  bool success = true;
  
  /* Test register lookup by name */
  const x86_register_info_t* reg_info = x86_translator_get_register_by_name("eax");
  
  if (!test_assert(results, reg_info != NULL, "Get EAX register")) {
    success = false;
  } else {
    if (!test_assert_string(results, reg_info->name, "eax", "Register name")) {
      success = false;
    }
    
    if (!test_assert_int(results, reg_info->reg_class, X86_REG_CLASS_GENERAL, "Register class")) {
      success = false;
    }
    
    if (!test_assert_int(results, reg_info->size, X86_REG_SIZE_32, "Register size")) {
      success = false;
    }
    
    if (!test_assert_int(results, reg_info->id, 0, "Register ID")) {
      success = false;
    }
  }
  
  /* Test register lookup by ID */
  reg_info = x86_translator_get_register_by_id(1, X86_REG_CLASS_GENERAL, X86_REG_SIZE_64);
  
  if (!test_assert(results, reg_info != NULL, "Get RCX register")) {
    success = false;
  } else {
    if (!test_assert_string(results, reg_info->name, "rcx", "Register name")) {
      success = false;
    }
  }
  
  /* Test invalid register lookups */
  reg_info = x86_translator_get_register_by_name("invalid");
  
  if (!test_assert(results, reg_info == NULL, "Invalid register name")) {
    success = false;
  }
  
  reg_info = x86_translator_get_register_by_id(100, X86_REG_CLASS_GENERAL, X86_REG_SIZE_32);
  
  if (!test_assert(results, reg_info == NULL, "Invalid register ID")) {
    success = false;
  }
  
  return success;
}

/**
 * @brief Tests x86 translator initialization
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
static bool test_x86_translator_init(test_results_t* results) {
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
    goto cleanup;
  }
  
  strcpy(target->name, "x86_64-test");
  strcpy(target->architecture, "x86_64");
  strcpy(target->vendor, "test");
  
  /* Add x86_64 feature */
  target->features = memory_calloc(1, sizeof(char*));
  if (!test_assert(results, target->features != NULL, "Allocate features array")) {
    success = false;
    goto cleanup;
  }
  
  target->features[0] = memory_alloc(32);
  if (!test_assert(results, target->features[0] != NULL, "Allocate feature string")) {
    success = false;
    goto cleanup;
  }
  
  strcpy(target->features[0], "x86_64");
  target->feature_count = 1;
  
  /* Create translator context */
  translator_context_t* context = NULL;
  error_t context_result = translator_create_context(target, &context);
  
  if (!test_assert_error(results, context_result, ERROR_NONE, "Create translator context")) {
    success = false;
    goto cleanup;
  }
  
  /* Test translator initialization */
  error_t init_result = x86_translator_init(context);
  
  if (!test_assert_error(results, init_result, ERROR_NONE, "Initialize x86 translator")) {
    success = false;
  }
  
  /* Test that x86-specific data was created */
  if (!test_assert(results, context->arch_specific != NULL, "Arch-specific data not NULL")) {
    success = false;
  }
  
  /* Test cleanup */
  error_t cleanup_result = x86_translator_cleanup(context);
  
  if (!test_assert_error(results, cleanup_result, ERROR_NONE, "Cleanup x86 translator")) {
    success = false;
  }
  
  /* Free translator context */
  translator_destroy_context(context);
  
cleanup:
  target_config_free(target);
  return success;
}

/**
 * @brief Tests x86 translation of arithmetic instructions
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
static bool test_x86_arithmetic_translation(test_results_t* results) {
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
    goto cleanup;
  }
  
  strcpy(target->name, "x86_64-test");
  strcpy(target->architecture, "x86_64");
  strcpy(target->vendor, "test");
  
  /* Add x86_64 feature */
  target->features = memory_calloc(1, sizeof(char*));
  if (!test_assert(results, target->features != NULL, "Allocate features array")) {
    success = false;
    goto cleanup;
  }
  
  target->features[0] = memory_alloc(32);
  if (!test_assert(results, target->features[0] != NULL, "Allocate feature string")) {
    success = false;
    goto cleanup;
  }
  
  strcpy(target->features[0], "x86_64");
  target->feature_count = 1;
  
  /* Create translator context */
  translator_context_t* context = NULL;
  error_t context_result = translator_create_context(target, &context);
  
  if (!test_assert_error(results, context_result, ERROR_NONE, "Create translator context")) {
    success = false;
    goto cleanup;
  }
  
  /* Initialize x86 translator */
  error_t init_result = x86_translator_init(context);
  
  if (!test_assert_error(results, init_result, ERROR_NONE, "Initialize x86 translator")) {
    success = false;
    goto cleanup_context;
  }
  
  /* Test translation of ADD instruction */
  coil_instruction_t add_instruction;
  create_mock_instruction(&add_instruction, COIL_OP_ADD, 0, 2);
  
  error_t add_result = x86_translate_arithmetic(&add_instruction, context);
  
  if (!test_assert_error(results, add_result, ERROR_NONE, "Translate ADD instruction")) {
    success = false;
  }
  
  /* Check that the output has at least one instruction */
  if (!test_assert(results, context->output->count >= 1, "ADD output has instructions")) {
    success = false;
  } else {
    /* Verify the output instruction */
    native_instruction_t* output = &context->output->instructions[context->output->count - 1];
    
    if (!test_assert_string(results, output->mnemonic, "addl", "ADD mnemonic")) {
      success = false;
    }
    
    if (!test_assert_int(results, output->operand_count, 2, "ADD operand count")) {
      success = false;
    }
  }
  
  /* Test translation of SUB instruction */
  coil_instruction_t sub_instruction;
  create_mock_instruction(&sub_instruction, COIL_OP_SUB, 0, 2);
  
  /* Reset the output */
  context->output->count = 0;
  
  error_t sub_result = x86_translate_arithmetic(&sub_instruction, context);
  
  if (!test_assert_error(results, sub_result, ERROR_NONE, "Translate SUB instruction")) {
    success = false;
  }
  
  /* Check that the output has at least one instruction */
  if (!test_assert(results, context->output->count >= 1, "SUB output has instructions")) {
    success = false;
  } else {
    /* Verify the output instruction */
    native_instruction_t* output = &context->output->instructions[context->output->count - 1];
    
    if (!test_assert_string(results, output->mnemonic, "subl", "SUB mnemonic")) {
      success = false;
    }
    
    if (!test_assert_int(results, output->operand_count, 2, "SUB operand count")) {
      success = false;
    }
    
    if (!test_assert_int(results, output->operands[1].type, NATIVE_OPERAND_IMMEDIATE, "SUB operand type")) {
      success = false;
    }
  }
  
  /* Test translation of MUL instruction */
  coil_instruction_t mul_instruction;
  create_mock_instruction(&mul_instruction, COIL_OP_MUL, 0, 2);
  
  /* Reset the output */
  context->output->count = 0;
  
  error_t mul_result = x86_translate_arithmetic(&mul_instruction, context);
  
  if (!test_assert_error(results, mul_result, ERROR_NONE, "Translate MUL instruction")) {
    success = false;
  }
  
  /* Check that the output has at least one instruction */
  if (!test_assert(results, context->output->count >= 1, "MUL output has instructions")) {
    success = false;
  } else {
    /* Verify the output instruction */
    native_instruction_t* output = &context->output->instructions[context->output->count - 1];
    
    if (!test_assert_string(results, output->mnemonic, "imull", "MUL mnemonic")) {
      success = false;
    }
  }
  
cleanup_context:
  /* Free translator context */
  translator_destroy_context(context);
  
cleanup:
  target_config_free(target);
  return success;
}

/**
 * @brief Main test function for x86 translator
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
bool test_x86_translator(test_results_t* results) {
  bool success = true;
  
  /* Initialize memory management */
  memory_init(true);
  
  /* Run sub-tests */
  if (!test_x86_registers(results)) {
    success = false;
  }
  
  if (!test_x86_translator_init(results)) {
    success = false;
  }
  
  if (!test_x86_arithmetic_translation(results)) {
    success = false;
  }
  
  /* Cleanup memory management */
  memory_shutdown();
  
  return success;
}