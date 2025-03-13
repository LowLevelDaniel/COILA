/**
 * @file instruction_decoder.h
 * @brief Decoder for COIL instructions
 * 
 * This module decodes COIL instructions from binary format into an internal
 * representation that can be processed by the translator and optimizer.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef INSTRUCTION_DECODER_H
#define INSTRUCTION_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include "../utils/error_handling.h"
#include "binary_parser.h"

/**
 * @brief Maximum number of operands per instruction
 */
#define MAX_OPERANDS 4

/**
 * @brief COIL instruction categories
 */
typedef enum {
  COIL_CAT_ARITHMETIC = 0x01,  /**< Arithmetic operations */
  COIL_CAT_LOGICAL = 0x02,     /**< Logical operations */
  COIL_CAT_COMPARISON = 0x03,  /**< Comparison operations */
  COIL_CAT_CONTROL = 0x04,     /**< Control flow operations */
  COIL_CAT_MEMORY = 0x05,      /**< Memory operations */
  COIL_CAT_CONVERSION = 0x06,  /**< Type conversion operations */
  COIL_CAT_VECTOR = 0x07,      /**< Vector operations */
  COIL_CAT_ATOMIC = 0x08,      /**< Atomic operations */
  COIL_CAT_SPECIAL = 0x09      /**< Architecture-specific operations */
} coil_instruction_category_t;

/**
 * @brief COIL core arithmetic instructions
 */
typedef enum {
  COIL_OP_ADD = 0x01,         /**< Addition */
  COIL_OP_SUB = 0x02,         /**< Subtraction */
  COIL_OP_MUL = 0x03,         /**< Multiplication */
  COIL_OP_DIV = 0x04,         /**< Division */
  COIL_OP_REM = 0x05,         /**< Remainder */
  COIL_OP_NEG = 0x06,         /**< Negation */
  COIL_OP_ABS = 0x07,         /**< Absolute value */
  COIL_OP_MIN = 0x08,         /**< Minimum */
  COIL_OP_MAX = 0x09,         /**< Maximum */
  COIL_OP_FMA = 0x0A          /**< Fused multiply-add */
} coil_arithmetic_opcode_t;

/**
 * @brief COIL operand types
 */
typedef enum {
  COIL_OPERAND_REGISTER,      /**< Register operand */
  COIL_OPERAND_IMMEDIATE,     /**< Immediate value operand */
  COIL_OPERAND_MEMORY,        /**< Memory address operand */
  COIL_OPERAND_LABEL          /**< Label reference operand */
} coil_operand_type_t;

/**
 * @brief COIL operand structure
 */
typedef struct {
  coil_operand_type_t type;    /**< Type of operand */
  uint32_t data_type;          /**< Data type of operand (from COIL type system) */
  
  union {
    uint32_t reg_id;           /**< Register identifier */
    int64_t imm_value;         /**< Immediate value */
    struct {
      uint32_t base_reg;       /**< Base register for memory address */
      int32_t displacement;    /**< Displacement for memory address */
      uint32_t index_reg;      /**< Index register for memory address */
      uint8_t scale;           /**< Scale factor for index register */
    } mem;                     /**< Memory address components */
    uint32_t label_id;         /**< Label identifier */
  } value;                     /**< Operand value */
} coil_operand_t;

/**
 * @brief COIL instruction structure
 */
typedef struct {
  uint8_t opcode;              /**< Instruction opcode */
  uint8_t flags;               /**< Instruction-specific flags */
  uint8_t operand_count;       /**< Number of source operands */
  uint8_t destination;         /**< Destination register/value */
  coil_operand_t operands[MAX_OPERANDS]; /**< Source operands */
  uint32_t data_type;          /**< Data type of operation */
} coil_instruction_t;

/**
 * @brief COIL basic block structure
 */
typedef struct coil_basic_block_t {
  uint32_t id;                 /**< Block identifier */
  char* label;                 /**< Block label (optional) */
  coil_instruction_t* instructions; /**< Array of instructions */
  uint32_t instruction_count;  /**< Number of instructions */
  struct coil_basic_block_t** successors; /**< Array of successor blocks */
  uint32_t successor_count;    /**< Number of successors */
  struct coil_basic_block_t** predecessors; /**< Array of predecessor blocks */
  uint32_t predecessor_count;  /**< Number of predecessors */
} coil_basic_block_t;

/**
 * @brief COIL function structure
 */
typedef struct {
  uint32_t id;                 /**< Function identifier */
  char* name;                  /**< Function name */
  uint32_t* param_types;       /**< Array of parameter types */
  uint32_t param_count;        /**< Number of parameters */
  uint32_t return_type;        /**< Return type */
  coil_basic_block_t* entry_block; /**< Entry basic block */
  coil_basic_block_t** blocks; /**< Array of all basic blocks */
  uint32_t block_count;        /**< Number of basic blocks */
} coil_function_t;

/**
 * @brief Decoded COIL module structure
 */
typedef struct {
  coil_function_t** functions; /**< Array of functions */
  uint32_t function_count;     /**< Number of functions */
  void* type_info;             /**< Type information (format depends on implementation) */
  void* globals;               /**< Global variables (format depends on implementation) */
  void* constants;             /**< Constants (format depends on implementation) */
} coil_decoded_module_t;

/**
 * @brief Decodes a COIL module into the internal representation
 *
 * @param[in] module Pointer to the parsed COIL module
 * @param[out] decoded_module Pointer to receive the decoded module
 * @return Error code indicating success or failure
 */
error_t instruction_decoder_decode(
  const coil_module_t* module,
  coil_decoded_module_t** decoded_module
);

/**
 * @brief Frees resources associated with a decoded COIL module
 *
 * @param[in] decoded_module Pointer to the decoded module to free
 * @return Error code indicating success or failure
 */
error_t instruction_decoder_free_module(coil_decoded_module_t* decoded_module);

/**
 * @brief Gets a function by ID from a decoded module
 *
 * @param[in] decoded_module Pointer to the decoded module
 * @param[in] function_id Function identifier to look up
 * @return Pointer to the function, or NULL if not found
 */
coil_function_t* instruction_decoder_get_function(
  const coil_decoded_module_t* decoded_module,
  uint32_t function_id
);

/**
 * @brief Gets a function by name from a decoded module
 *
 * @param[in] decoded_module Pointer to the decoded module
 * @param[in] function_name Function name to look up
 * @return Pointer to the function, or NULL if not found
 */
coil_function_t* instruction_decoder_get_function_by_name(
  const coil_decoded_module_t* decoded_module,
  const char* function_name
);

/**
 * @brief Decodes a single instruction from binary data
 *
 * @param[in] binary Pointer to binary instruction data
 * @param[in] size Size of binary data in bytes
 * @param[out] instruction Pointer to receive the decoded instruction
 * @param[out] bytes_read Pointer to receive the number of bytes read
 * @return Error code indicating success or failure
 */
error_t instruction_decoder_decode_instruction(
  const uint8_t* binary,
  size_t size,
  coil_instruction_t* instruction,
  size_t* bytes_read
);

#endif /* INSTRUCTION_DECODER_H */