/**
 * @file x86_translator.h
 * @brief x86-specific instruction translation
 * 
 * This module provides x86-specific implementations for translating
 * COIL instructions to native x86 instructions.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef X86_TRANSLATOR_H
#define X86_TRANSLATOR_H

#include <stdint.h>
#include <stddef.h>
#include "../../core/translator.h"
#include "../../core/instruction_decoder.h"
#include "../../utils/error_handling.h"

/**
 * @brief x86 addressing modes
 */
typedef enum {
  X86_ADDR_MODE_DIRECT,       /**< Direct addressing */
  X86_ADDR_MODE_REGISTER,     /**< Register addressing */
  X86_ADDR_MODE_INDIRECT,     /**< Indirect addressing */
  X86_ADDR_MODE_INDEXED,      /**< Indexed addressing */
  X86_ADDR_MODE_BASED_INDEXED /**< Based indexed addressing */
} x86_addr_mode_t;

/**
 * @brief x86 register classes
 */
typedef enum {
  X86_REG_CLASS_GENERAL,      /**< General-purpose registers */
  X86_REG_CLASS_SEGMENT,      /**< Segment registers */
  X86_REG_CLASS_FLOATING,     /**< Floating-point registers */
  X86_REG_CLASS_MMX,          /**< MMX registers */
  X86_REG_CLASS_SSE,          /**< SSE registers */
  X86_REG_CLASS_AVX,          /**< AVX registers */
  X86_REG_CLASS_CONTROL,      /**< Control registers */
  X86_REG_CLASS_DEBUG         /**< Debug registers */
} x86_reg_class_t;

/**
 * @brief x86 register sizes
 */
typedef enum {
  X86_REG_SIZE_8 = 1,         /**< 8-bit registers */
  X86_REG_SIZE_16 = 2,        /**< 16-bit registers */
  X86_REG_SIZE_32 = 4,        /**< 32-bit registers */
  X86_REG_SIZE_64 = 8,        /**< 64-bit registers */
  X86_REG_SIZE_128 = 16,      /**< 128-bit registers (SSE) */
  X86_REG_SIZE_256 = 32,      /**< 256-bit registers (AVX) */
  X86_REG_SIZE_512 = 64       /**< 512-bit registers (AVX-512) */
} x86_reg_size_t;

/**
 * @brief x86 register information
 */
typedef struct {
  const char* name;           /**< Register name */
  x86_reg_class_t reg_class;  /**< Register class */
  x86_reg_size_t size;        /**< Register size */
  uint8_t id;                 /**< Register ID */
  uint8_t encoding;           /**< Register encoding value */
} x86_register_info_t;

/**
 * @brief x86-specific translation context data
 */
typedef struct {
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
  void* register_map;         /**< COIL to x86 register mapping */
} x86_translator_data_t;

/**
 * @brief Initializes x86-specific translation
 *
 * @param[in,out] context Translation context to initialize
 * @return Error code indicating success or failure
 */
error_t x86_translator_init(translator_context_t* context);

/**
 * @brief Cleans up x86-specific translation resources
 *
 * @param[in,out] context Translation context to clean up
 * @return Error code indicating success or failure
 */
error_t x86_translator_cleanup(translator_context_t* context);

/**
 * @brief Gets an x86 register by name
 *
 * @param[in] reg_name Register name
 * @return Pointer to register info, or NULL if not found
 */
const x86_register_info_t* x86_translator_get_register_by_name(const char* reg_name);

/**
 * @brief Gets an x86 register by ID
 *
 * @param[in] reg_id Register ID
 * @param[in] reg_class Register class
 * @param[in] reg_size Register size
 * @return Pointer to register info, or NULL if not found
 */
const x86_register_info_t* x86_translator_get_register_by_id(
  uint8_t reg_id, 
  x86_reg_class_t reg_class,
  x86_reg_size_t reg_size
);

/**
 * @brief Translates a COIL arithmetic instruction to x86
 *
 * @param[in] instruction COIL instruction to translate
 * @param[in,out] context Translation context
 * @return Error code indicating success or failure
 */
error_t x86_translate_arithmetic(
  const coil_instruction_t* instruction,
  translator_context_t* context
);

/**
 * @brief Translates a COIL logical instruction to x86
 *
 * @param[in] instruction COIL instruction to translate
 * @param[in,out] context Translation context
 * @return Error code indicating success or failure
 */
error_t x86_translate_logical(
  const coil_instruction_t* instruction,
  translator_context_t* context
);

/**
 * @brief Translates a COIL memory instruction to x86
 *
 * @param[in] instruction COIL instruction to translate
 * @param[in,out] context Translation context
 * @return Error code indicating success or failure
 */
error_t x86_translate_memory(
  const coil_instruction_t* instruction,
  translator_context_t* context
);

/**
 * @brief Translates a COIL control flow instruction to x86
 *
 * @param[in] instruction COIL instruction to translate
 * @param[in,out] context Translation context
 * @return Error code indicating success or failure
 */
error_t x86_translate_control(
  const coil_instruction_t* instruction,
  translator_context_t* context
);

/**
 * @brief Creates an x86 register operand
 *
 * @param[in] reg_info Register information
 * @param[out] operand Pointer to receive the operand
 * @return Error code indicating success or failure
 */
error_t x86_create_register_operand(
  const x86_register_info_t* reg_info,
  native_operand_t* operand
);

/**
 * @brief Creates an x86 memory operand
 *
 * @param[in] base_reg Base register (or NULL)
 * @param[in] index_reg Index register (or NULL)
 * @param[in] scale Scale factor (0, 1, 2, 4, or 8)
 * @param[in] displacement Memory displacement
 * @param[in] size Operand size in bytes
 * @param[in] segment Segment override (or 0 for default)
 * @param[out] operand Pointer to receive the operand
 * @return Error code indicating success or failure
 */
error_t x86_create_memory_operand(
  const x86_register_info_t* base_reg,
  const x86_register_info_t* index_reg,
  uint8_t scale,
  int32_t displacement,
  uint8_t size,
  uint8_t segment,
  native_operand_t* operand
);

/**
 * @brief Creates an x86 immediate operand
 *
 * @param[in] value Immediate value
 * @param[in] size Operand size in bytes
 * @param[out] operand Pointer to receive the operand
 * @return Error code indicating success or failure
 */
error_t x86_create_immediate_operand(
  int64_t value,
  uint8_t size,
  native_operand_t* operand
);

/**
 * @brief Registers x86 translator with the translation system
 *
 * @return Error code indicating success or failure
 */
error_t x86_translator_register(void);

#endif /* X86_TRANSLATOR_H */