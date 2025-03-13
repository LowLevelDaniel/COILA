/**
 * @file x86_optimizer.h
 * @brief x86-specific optimization passes
 * 
 * This module provides optimization passes specific to the x86 architecture.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef X86_OPTIMIZER_H
#define X86_OPTIMIZER_H

#include <stdint.h>
#include <stddef.h>
#include "../../core/optimization.h"
#include "../../core/translator.h"
#include "../../utils/error_handling.h"

/**
 * @brief x86-specific optimization data
 */
typedef struct {
  bool is_32bit;
  bool is_64bit;              /**< Whether target is 64-bit mode */
  bool has_avx;               /**< Whether target has AVX */
  bool has_avx2;              /**< Whether target has AVX2 */
  bool has_avx512;            /**< Whether target has AVX-512 */
  bool has_sse;               /**< Whether target has SSE */
  bool has_sse2;              /**< Whether target has SSE2 */
  bool has_sse3;              /**< Whether target has SSE3 */
  bool has_sse4_1;            /**< Whether target has SSE4.1 */
  bool has_sse4_2;            /**< Whether target has SSE4.2 */
  bool has_fma;               /**< Whether target has FMA */
  void* target_specific;      /**< Target-specific data */
} x86_optimizer_data_t;

/**
 * @brief Initializes x86-specific optimization
 *
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_optimizer_init(optimization_context_t* context);

/**
 * @brief Cleans up x86-specific optimization resources
 *
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_optimizer_cleanup(optimization_context_t* context);

/**
 * @brief Registers all x86-specific optimization passes
 *
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_optimizer_register_all_passes(optimization_context_t* context);

/**
 * @brief Gets x86-specific optimization data
 *
 * @param[in] context Optimization context
 * @return x86-specific optimization data, or NULL on error
 */
x86_optimizer_data_t* x86_optimizer_get_data(optimization_context_t* context);

/* Standard optimization passes for x86 */

/**
 * @brief Peephole optimization pass for x86
 *
 * This pass applies peephole optimizations to x86 instructions.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] index Current instruction index
 * @param[in] window_size Size of the peephole window
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_peephole_optimize(
  native_instruction_list_t* instructions,
  size_t index,
  size_t window_size,
  optimization_context_t* context
);

/**
 * @brief Instruction scheduling pass for x86
 *
 * This pass schedules x86 instructions for better performance.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] start_index Start index of the basic block
 * @param[in] end_index End index of the basic block
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_instruction_schedule(
  native_instruction_list_t* instructions,
  size_t start_index,
  size_t end_index,
  optimization_context_t* context
);

/**
 * @brief Register allocation pass for x86
 *
 * This pass allocates registers for x86 instructions.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_register_allocate(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Dead code elimination pass for x86
 *
 * This pass removes dead code from x86 instructions.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_eliminate_dead_code(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Common subexpression elimination pass for x86
 *
 * This pass eliminates common subexpressions in x86 instructions.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_eliminate_common_subexpr(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Strength reduction pass for x86
 *
 * This pass replaces expensive operations with cheaper ones.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_strength_reduction(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Vectorization pass for x86
 *
 * This pass vectorizes scalar operations when appropriate.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_vectorize(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Loop unrolling pass for x86
 *
 * This pass unrolls loops for better performance.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_unroll_loops(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Instruction fusion pass for x86
 *
 * This pass fuses multiple instructions into more efficient ones.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_fuse_instructions(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

/**
 * @brief Memory access optimization pass for x86
 *
 * This pass optimizes memory access patterns.
 *
 * @param[in,out] instructions Instruction list to optimize
 * @param[in] context Optimization context
 * @return Error code indicating success or failure
 */
error_t x86_optimize_memory_access(
  native_instruction_list_t* instructions,
  optimization_context_t* context
);

#endif /* X86_OPTIMIZER_H */