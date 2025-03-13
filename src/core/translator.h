/**
 * @file translator.h
 * @brief Interface for instruction translation
 * 
 * This module defines the interface for translating COIL instructions
 * to native instructions for specific target architectures.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stdint.h>
#include <stddef.h>
#include "../utils/error_handling.h"
#include "instruction_decoder.h"
#include "target_config.h"

/**
 * @brief Maximum length of a native instruction mnemonic
 */
#define MAX_MNEMONIC_LENGTH 16

/**
 * @brief Maximum number of operands in a native instruction
 */
#define MAX_NATIVE_OPERANDS 4

/**
 * @brief Native operand types
 */
typedef enum {
  NATIVE_OPERAND_REGISTER,     /**< Register operand */
  NATIVE_OPERAND_IMMEDIATE,    /**< Immediate value operand */
  NATIVE_OPERAND_MEMORY,       /**< Memory operand */
  NATIVE_OPERAND_LABEL         /**< Label reference operand */
} native_operand_type_t;

/**
 * @brief Native operand structure
 */
typedef struct {
  native_operand_type_t type;  /**< Type of operand */
  
  union {
    struct {
      uint32_t id;            /**< Register identifier */
      char name[8];           /**< Register name */
    } reg;                    /**< Register operand data */
    
    int64_t imm;              /**< Immediate value */
    
    struct {
      uint32_t base_reg;      /**< Base register identifier */
      uint32_t index_reg;     /**< Index register identifier */
      uint8_t scale;          /**< Scale factor (0, 1, 2, 4, 8) */
      int32_t displacement;   /**< Memory displacement */
      uint8_t segment;        /**< Segment override (x86-specific) */
    } mem;                    /**< Memory operand data */
    
    struct {
      uint32_t id;            /**< Label identifier */
      char name[32];          /**< Label name */
    } label;                  /**< Label operand data */
  } value;                    /**< Operand-specific data */
  
  uint8_t size;               /**< Operand size in bytes */
} native_operand_t;

/**
 * @brief Native instruction structure
 */
typedef struct {
  char mnemonic[MAX_MNEMONIC_LENGTH]; /**< Instruction mnemonic */
  uint8_t opcode[4];                  /**< Raw opcode bytes */
  uint8_t opcode_size;                /**< Number of opcode bytes */
  uint8_t prefix[4];                  /**< Instruction prefixes */
  uint8_t prefix_count;               /**< Number of prefixes */
  native_operand_t operands[MAX_NATIVE_OPERANDS]; /**< Instruction operands */
  uint8_t operand_count;              /**< Number of operands */
  uint8_t size;                       /**< Total instruction size in bytes */
  uint8_t flags;                      /**< Instruction flags */
} native_instruction_t;

/**
 * @brief Native instruction list
 */
typedef struct {
  native_instruction_t* instructions; /**< Array of native instructions */
  size_t count;                       /**< Number of instructions */
  size_t capacity;                    /**< Capacity of instructions array */
} native_instruction_list_t;

/**
 * @brief Translation context
 */
typedef struct {
  const target_config_t* target;      /**< Target configuration */
  void* register_allocator;           /**< Register allocator (implementation-specific) */
  native_instruction_list_t* output;  /**< Output instruction list */
  void* arch_specific;                /**< Architecture-specific data */
} translator_context_t;

/**
 * @brief Function pointer type for instruction translators
 */
typedef error_t (*instruction_translator_t)(
  const coil_instruction_t* instruction,
  translator_context_t* context
);

/**
 * @brief Creates a new translation context
 *
 * @param[in] target Target configuration
 * @param[out] context Pointer to receive the created context
 * @return Error code indicating success or failure
 */
error_t translator_create_context(
  const target_config_t* target,
  translator_context_t** context
);

/**
 * @brief Destroys a translation context and frees associated resources
 *
 * @param[in] context Pointer to the context to destroy
 * @return Error code indicating success or failure
 */
error_t translator_destroy_context(translator_context_t* context);

/**
 * @brief Translates a COIL instruction to native instructions
 *
 * @param[in] instruction COIL instruction to translate
 * @param[in,out] context Translation context
 * @return Error code indicating success or failure
 */
error_t translator_translate_instruction(
  const coil_instruction_t* instruction,
  translator_context_t* context
);

/**
 * @brief Translates a COIL function to native instructions
 *
 * @param[in] function COIL function to translate
 * @param[in,out] context Translation context
 * @return Error code indicating success or failure
 */
error_t translator_translate_function(
  const coil_function_t* function,
  translator_context_t* context
);

/**
 * @brief Creates a new native instruction list
 *
 * @param[in] initial_capacity Initial capacity of the list
 * @param[out] list Pointer to receive the created list
 * @return Error code indicating success or failure
 */
error_t translator_create_instruction_list(
  size_t initial_capacity,
  native_instruction_list_t** list
);

/**
 * @brief Adds a native instruction to an instruction list
 *
 * @param[in,out] list Instruction list to add to
 * @param[in] instruction Instruction to add
 * @return Error code indicating success or failure
 */
error_t translator_add_instruction(
  native_instruction_list_t* list,
  const native_instruction_t* instruction
);

/**
 * @brief Frees resources associated with an instruction list
 *
 * @param[in] list Instruction list to free
 * @return Error code indicating success or failure
 */
error_t translator_free_instruction_list(native_instruction_list_t* list);

/**
 * @brief Maps a COIL register to a native register
 *
 * @param[in] coil_reg COIL register ID
 * @param[in] context Translation context
 * @param[out] native_reg Pointer to receive the native register
 * @return Error code indicating success or failure
 */
error_t translator_map_register(
  uint32_t coil_reg,
  translator_context_t* context,
  native_operand_t* native_reg
);

/**
 * @brief Maps a COIL operand to a native operand
 *
 * @param[in] coil_operand COIL operand to map
 * @param[in] context Translation context
 * @param[out] native_operand Pointer to receive the native operand
 * @return Error code indicating success or failure
 */
error_t translator_map_operand(
  const coil_operand_t* coil_operand,
  translator_context_t* context,
  native_operand_t* native_operand
);

#endif /* TRANSLATOR_H */