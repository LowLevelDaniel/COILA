/**
 * @file optimization.h
 * @brief Optimization framework for the COIL assembler
 * 
 * This module provides the optimization framework for transforming
 * native instructions to improve performance.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H

#include <stdint.h>
#include <stdbool.h>
#include "error_handling.h"
#include "translator.h"
#include "target_config.h"

/**
 * @brief Optimization pass information
 */
typedef struct {
  const char* name;            /**< Pass name */
  const char* description;     /**< Pass description */
  uint32_t min_opt_level;      /**< Minimum optimization level for this pass */
  bool enabled_by_default;     /**< Whether the pass is enabled by default */
} optimization_pass_info_t;

/**
 * @brief Optimization context
 */
typedef struct optimization_context_t optimization_context_t;

/**
 * @brief Optimization pass function type
 */
typedef error_t (*optimization_pass_t)(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Function-level optimization pass function type
 */
typedef error_t (*function_optimization_pass_t)(
  const char* function_name,
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Basic block optimization pass function type
 */
typedef error_t (*basic_block_optimization_pass_t)(
  native_instruction_list_t* instructions,
  size_t start_index,
  size_t end_index,
  optimization_context_t* context
);

/**
 * @brief Peephole optimization pass function type
 */
typedef error_t (*peephole_optimization_pass_t)(
  native_instruction_list_t* instructions,
  size_t index,
  size_t window_size,
  optimization_context_t* context
);

/**
 * @brief Instruction scheduling pass function type
 */
typedef error_t (*scheduling_pass_t)(
  native_instruction_list_t* instructions,
  size_t start_index,
  size_t end_index,
  optimization_context_t* context
);

/**
 * @brief Register allocation pass function type
 */
typedef error_t (*register_allocation_pass_t)(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Creates an optimization context
 *
 * @param[in] target Target configuration
 * @param[in] opt_level Optimization level
 * @param[out] context Pointer to receive the created context
 * @return Error code indicating success or failure
 */
error_t optimization_create_context(
  const target_config_t* target,
  uint32_t opt_level,
  optimization_context_t** context
);

/**
 * @brief Destroys an optimization context and frees associated resources
 *
 * @param[in] context Pointer to the context to destroy
 * @return Error code indicating success or failure
 */
error_t optimization_destroy_context(optimization_context_t* context);

/**
 * @brief Registers an optimization pass
 *
 * @param[in] context Optimization context
 * @param[in] pass_info Pass information
 * @param[in] pass_func Pass function
 * @return Error code indicating success or failure
 */
error_t optimization_register_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  optimization_pass_t pass_func
);

/**
 * @brief Registers a function-level optimization pass
 *
 * @param[in] context Optimization context
 * @param[in] pass_info Pass information
 * @param[in] pass_func Pass function
 * @return Error code indicating success or failure
 */
error_t optimization_register_function_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  function_optimization_pass_t pass_func
);

/**
 * @brief Registers a basic block optimization pass
 *
 * @param[in] context Optimization context
 * @param[in] pass_info Pass information
 * @param[in] pass_func Pass function
 * @return Error code indicating success or failure
 */
error_t optimization_register_basic_block_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  basic_block_optimization_pass_t pass_func
);

/**
 * @brief Registers a peephole optimization pass
 *
 * @param[in] context Optimization context
 * @param[in] pass_info Pass information
 * @param[in] pass_func Pass function
 * @param[in] window_size Size of the peephole window
 * @return Error code indicating success or failure
 */
error_t optimization_register_peephole_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  peephole_optimization_pass_t pass_func,
  size_t window_size
);

/**
 * @brief Registers an instruction scheduling pass
 *
 * @param[in] context Optimization context
 * @param[in] pass_info Pass information
 * @param[in] pass_func Pass function
 * @return Error code indicating success or failure
 */
error_t optimization_register_scheduling_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  scheduling_pass_t pass_func
);

/**
 * @brief Registers a register allocation pass
 *
 * @param[in] context Optimization context
 * @param[in] pass_info Pass information
 * @param[in] pass_func Pass function
 * @return Error code indicating success or failure
 */
error_t optimization_register_register_allocation_pass(
  optimization_context_t* context,
  const optimization_pass_info_t* pass_info,
  register_allocation_pass_t pass_func
);

/**
 * @brief Enables or disables an optimization pass
 *
 * @param[in] context Optimization context
 * @param[in] pass_name Pass name
 * @param[in] enable Whether to enable the pass
 * @return Error code indicating success or failure
 */
error_t optimization_enable_pass(
  optimization_context_t* context,
  const char* pass_name,
  bool enable
);

/**
 * @brief Gets information about a registered optimization pass
 *
 * @param[in] context Optimization context
 * @param[in] pass_name Pass name
 * @param[out] pass_info Pointer to receive pass information
 * @return Error code indicating success or failure
 */
error_t optimization_get_pass_info(
  const optimization_context_t* context,
  const char* pass_name,
  const optimization_pass_info_t** pass_info
);

/**
 * @brief Gets a list of all registered optimization passes
 *
 * @param[in] context Optimization context
 * @param[out] pass_infos Pointer to receive array of pass information pointers
 * @param[out] pass_count Pointer to receive the number of passes
 * @return Error code indicating success or failure
 */
error_t optimization_get_all_passes(
  const optimization_context_t* context,
  const optimization_pass_info_t*** pass_infos,
  size_t* pass_count
);

/**
 * @brief Applies all enabled optimization passes to an instruction list
 *
 * @param[in] context Optimization context
 * @param[in,out] instructions Instruction list to optimize
 * @return Error code indicating success or failure
 */
error_t optimization_apply_all_passes(
  optimization_context_t* context,
  native_instruction_list_t* instructions
);

/**
 * @brief Applies a specific optimization pass to an instruction list
 *
 * @param[in] context Optimization context
 * @param[in] pass_name Pass name
 * @param[in,out] instructions Instruction list to optimize
 * @return Error code indicating success or failure
 */
error_t optimization_apply_pass(
  optimization_context_t* context,
  const char* pass_name,
  native_instruction_list_t* instructions
);

/**
 * @brief Sets the optimization level
 *
 * @param[in] context Optimization context
 * @param[in] opt_level Optimization level to set
 * @return Error code indicating success or failure
 */
error_t optimization_set_level(
  optimization_context_t* context,
  uint32_t opt_level
);

/**
 * @brief Gets the current optimization level
 *
 * @param[in] context Optimization context
 * @return Current optimization level
 */
uint32_t optimization_get_level(const optimization_context_t* context);

/**
 * @brief Sets architecture-specific optimization data
 *
 * @param[in] context Optimization context
 * @param[in] arch_data Architecture-specific data (implementation-defined)
 * @return Error code indicating success or failure
 */
error_t optimization_set_arch_data(
  optimization_context_t* context,
  void* arch_data
);

/**
 * @brief Gets architecture-specific optimization data
 *
 * @param[in] context Optimization context
 * @return Architecture-specific data (implementation-defined)
 */
void* optimization_get_arch_data(const optimization_context_t* context);

#endif /* OPTIMIZATION_H */