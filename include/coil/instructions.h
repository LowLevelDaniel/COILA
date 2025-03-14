/**
 * @file instructions.h
 * @brief COIL instruction set
 * @details This file contains definitions for the COIL instruction set,
 *          including opcodes, operands, and instruction format.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_INSTRUCTIONS_H
#define COIL_INSTRUCTIONS_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

/**
 * @brief COIL instruction categories
 */
typedef enum {
  COIL_INST_CAT_ARITHMETIC = 0x00, /**< Basic arithmetic operations */
  COIL_INST_CAT_LOGICAL    = 0x10, /**< Bitwise and logical operations */
  COIL_INST_CAT_COMPARISON = 0x20, /**< Value comparison operations */
  COIL_INST_CAT_CONTROL    = 0x30, /**< Control flow operations */
  COIL_INST_CAT_MEMORY     = 0x40, /**< Memory access operations */
  COIL_INST_CAT_CONVERSION = 0x50, /**< Type conversion operations */
  COIL_INST_CAT_VECTOR     = 0x60, /**< Vector operations */
  COIL_INST_CAT_ATOMIC     = 0x70, /**< Atomic operations */
  COIL_INST_CAT_SPECIAL    = 0xF0  /**< Special operations */
} coil_instruction_category_t;

/**
 * @brief COIL instruction opcodes
 */
typedef enum {
  /* Arithmetic operations */
  COIL_INST_ADD     = 0x01, /**< Addition */
  COIL_INST_SUB     = 0x02, /**< Subtraction */
  COIL_INST_MUL     = 0x03, /**< Multiplication */
  COIL_INST_DIV     = 0x04, /**< Division */
  COIL_INST_REM     = 0x05, /**< Remainder */
  COIL_INST_NEG     = 0x06, /**< Negation */
  COIL_INST_ABS     = 0x07, /**< Absolute value */
  COIL_INST_MIN     = 0x08, /**< Minimum */
  COIL_INST_MAX     = 0x09, /**< Maximum */
  COIL_INST_FMA     = 0x0A, /**< Fused multiply-add */
  
  /* Logical operations */
  COIL_INST_AND     = 0x11, /**< Bitwise AND */
  COIL_INST_OR      = 0x12, /**< Bitwise OR */
  COIL_INST_XOR     = 0x13, /**< Bitwise XOR */
  COIL_INST_NOT     = 0x14, /**< Bitwise NOT */
  COIL_INST_SHL     = 0x15, /**< Shift left */
  COIL_INST_SHR     = 0x16, /**< Shift right */
  COIL_INST_SAR     = 0x17, /**< Arithmetic shift right */
  COIL_INST_ROL     = 0x18, /**< Rotate left */
  COIL_INST_ROR     = 0x19, /**< Rotate right */
  
  /* Comparison operations */
  COIL_INST_CMP_EQ  = 0x21, /**< Equal */
  COIL_INST_CMP_NE  = 0x22, /**< Not equal */
  COIL_INST_CMP_LT  = 0x23, /**< Less than */
  COIL_INST_CMP_LE  = 0x24, /**< Less than or equal */
  COIL_INST_CMP_GT  = 0x25, /**< Greater than */
  COIL_INST_CMP_GE  = 0x26, /**< Greater than or equal */
  
  /* Control flow operations */
  COIL_INST_BR      = 0x31, /**< Branch */
  COIL_INST_BR_COND = 0x32, /**< Conditional branch */
  COIL_INST_SWITCH  = 0x33, /**< Switch branch */
  COIL_INST_CALL    = 0x34, /**< Function call */
  COIL_INST_RET     = 0x35, /**< Function return */
  
  /* Memory operations */
  COIL_INST_LOAD    = 0x41, /**< Load from memory */
  COIL_INST_STORE   = 0x42, /**< Store to memory */
  COIL_INST_LEA     = 0x43, /**< Load effective address */
  COIL_INST_FENCE   = 0x44, /**< Memory fence */
  
  /* Type conversion operations */
  COIL_INST_CONVERT = 0x51, /**< Convert type */
  COIL_INST_TRUNC   = 0x52, /**< Truncate value */
  COIL_INST_EXTEND  = 0x53, /**< Extend value */
  COIL_INST_BITCAST = 0x54, /**< Reinterpret bits */
  
  /* Vector operations */
  COIL_INST_VADD    = 0x61, /**< Vector addition */
  COIL_INST_VSUB    = 0x62, /**< Vector subtraction */
  COIL_INST_VMUL    = 0x63, /**< Vector multiplication */
  COIL_INST_VDIV    = 0x64, /**< Vector division */
  COIL_INST_VDOT    = 0x65, /**< Vector dot product */
  COIL_INST_VSPLAT  = 0x66, /**< Splat scalar to vector */
  COIL_INST_VEXTRACT = 0x67, /**< Extract vector element */
  COIL_INST_VINSERT = 0x68, /**< Insert vector element */
  
  /* Atomic operations */
  COIL_INST_ATOMIC_ADD = 0x71, /**< Atomic add */
  COIL_INST_ATOMIC_SUB = 0x72, /**< Atomic subtract */
  COIL_INST_ATOMIC_AND = 0x73, /**< Atomic AND */
  COIL_INST_ATOMIC_OR  = 0x74, /**< Atomic OR */
  COIL_INST_ATOMIC_XOR = 0x75, /**< Atomic XOR */
  COIL_INST_ATOMIC_CAS = 0x76, /**< Compare and swap */
  
  /* Special operations */
  COIL_INST_NOP     = 0xF0, /**< No operation */
  COIL_INST_TRAP    = 0xF1, /**< Trap instruction */
  COIL_INST_UNREACHABLE = 0xF2, /**< Unreachable code */
  COIL_INST_TARGET  = 0xFF  /**< Target-specific operation */
} coil_instruction_opcode_t;

/**
 * @brief COIL instruction flags
 */
typedef enum {
  COIL_INST_FLAG_NONE      = 0x00, /**< No flags */
  COIL_INST_FLAG_COMMUTATIVE = 0x01, /**< Commutative operation */
  COIL_INST_FLAG_ASSOCIATIVE = 0x02, /**< Associative operation */
  COIL_INST_FLAG_IDEMPOTENT  = 0x04, /**< Idempotent operation */
  COIL_INST_FLAG_VOLATILE    = 0x08, /**< Volatile memory access */
  COIL_INST_FLAG_ATOMIC      = 0x10  /**< Atomic operation */
} coil_instruction_flag_t;

/**
 * @brief Memory ordering models
 */
typedef enum {
  COIL_MEM_ORDER_RELAXED    = 0, /**< Relaxed ordering */
  COIL_MEM_ORDER_ACQUIRE    = 1, /**< Acquire ordering */
  COIL_MEM_ORDER_RELEASE    = 2, /**< Release ordering */
  COIL_MEM_ORDER_ACQ_REL    = 3, /**< Acquire-release ordering */
  COIL_MEM_ORDER_SEQ_CST    = 4  /**< Sequentially consistent ordering */
} coil_memory_order_t;

/**
 * @brief Operand types
 */
typedef enum {
  COIL_OPERAND_NONE     = 0, /**< No operand */
  COIL_OPERAND_REGISTER = 1, /**< Register */
  COIL_OPERAND_IMMEDIATE = 2, /**< Immediate value */
  COIL_OPERAND_MEMORY   = 3, /**< Memory operand */
  COIL_OPERAND_BLOCK_REF = 4, /**< Basic block reference */
  COIL_OPERAND_FUNC_REF  = 5, /**< Function reference */
  COIL_OPERAND_TYPE_REF  = 6  /**< Type reference */
} coil_operand_type_t;

/**
 * @brief COIL operand structure
 */
typedef struct {
  uint8_t type;          /**< Operand type (coil_operand_type_t) */
  coil_type_t data_type; /**< Data type of the operand */
  union {
    uint32_t reg_id;     /**< Register ID */
    int64_t  imm_value;  /**< Immediate value */
    struct {
      uint32_t base_reg; /**< Base register for memory operand */
      uint32_t index_reg; /**< Index register for memory operand */
      int32_t  offset;   /**< Offset for memory operand */
      uint8_t  scale;    /**< Scale for indexed memory operand */
    } mem;               /**< Memory operand */
    uint32_t block_id;   /**< Basic block ID */
    uint32_t func_id;    /**< Function ID */
    coil_type_t type_id; /**< Type ID */
  } value;               /**< Operand value */
} coil_operand_t;

/**
 * @brief COIL instruction structure
 */
typedef struct {
  uint8_t  opcode;       /**< Instruction opcode */
  uint8_t  flags;        /**< Instruction flags */
  uint8_t  operand_count; /**< Number of operands */
  coil_type_t result_type; /**< Result type */
  coil_operand_t result;   /**< Result operand */
  coil_operand_t operands[4]; /**< Source operands (variable-sized in file) */
} coil_instruction_t;

/**
 * @brief COIL basic block structure
 */
typedef struct coil_basic_block_s {
  uint32_t                id;          /**< Block ID */
  char                   *name;        /**< Block name */
  uint32_t                instruction_count; /**< Number of instructions */
  coil_instruction_t     *instructions; /**< Array of instructions */
  struct coil_basic_block_s **predecessors; /**< Predecessor blocks */
  uint32_t                predecessor_count; /**< Number of predecessors */
  struct coil_basic_block_s **successors;   /**< Successor blocks */
  uint32_t                successor_count;  /**< Number of successors */
} coil_basic_block_t;

/**
 * @brief COIL function structure
 */
typedef struct {
  uint32_t             id;          /**< Function ID */
  char                *name;        /**< Function name */
  coil_type_t          type;        /**< Function type */
  uint32_t             block_count; /**< Number of basic blocks */
  coil_basic_block_t **blocks;      /**< Array of basic blocks */
  uint32_t             param_count; /**< Number of parameters */
  uint32_t            *param_regs;  /**< Parameter register IDs */
} coil_function_t;

/**
 * @brief Creates a register operand
 * @param reg_id Register ID
 * @param data_type Data type of the register
 * @return The register operand
 */
coil_operand_t coil_operand_create_register(uint32_t reg_id, coil_type_t data_type);

/**
 * @brief Creates an immediate operand
 * @param value Immediate value
 * @param data_type Data type of the immediate
 * @return The immediate operand
 */
coil_operand_t coil_operand_create_immediate(int64_t value, coil_type_t data_type);

/**
 * @brief Creates a memory operand
 * @param base_reg Base register
 * @param index_reg Index register (0 for none)
 * @param offset Memory offset
 * @param scale Scale factor for index (1, 2, 4, or 8)
 * @param data_type Data type of the memory access
 * @return The memory operand
 */
coil_operand_t coil_operand_create_memory(uint32_t base_reg, uint32_t index_reg,
                                          int32_t offset, uint8_t scale,
                                          coil_type_t data_type);

/**
 * @brief Creates a basic block reference operand
 * @param block_id Basic block ID
 * @return The block reference operand
 */
coil_operand_t coil_operand_create_block_ref(uint32_t block_id);

/**
 * @brief Creates a function reference operand
 * @param func_id Function ID
 * @return The function reference operand
 */
coil_operand_t coil_operand_create_func_ref(uint32_t func_id);

/**
 * @brief Creates a type reference operand
 * @param type_id Type ID
 * @return The type reference operand
 */
coil_operand_t coil_operand_create_type_ref(coil_type_t type_id);

/**
 * @brief Creates a new instruction
 * @param opcode Instruction opcode
 * @param flags Instruction flags
 * @param result_type Result type
 * @param result Result operand
 * @param operands Array of source operands
 * @param operand_count Number of source operands
 * @return A new instruction or NULL on failure
 */
coil_instruction_t* coil_instruction_create(uint8_t opcode, uint8_t flags,
                                          coil_type_t result_type,
                                          coil_operand_t result,
                                          coil_operand_t *operands,
                                          uint8_t operand_count);

/**
 * @brief Creates a new basic block
 * @param id Block ID
 * @param name Block name
 * @return A new basic block or NULL on failure
 */
coil_basic_block_t* coil_basic_block_create(uint32_t id, const char *name);

/**
 * @brief Adds an instruction to a basic block
 * @param block Block to add to
 * @param inst Instruction to add
 * @return 0 on success, non-zero on failure
 */
int coil_basic_block_add_instruction(coil_basic_block_t *block, 
                                    coil_instruction_t *inst);

/**
 * @brief Creates a new function
 * @param id Function ID
 * @param name Function name
 * @param type Function type
 * @return A new function or NULL on failure
 */
coil_function_t* coil_function_create(uint32_t id, const char *name, 
                                     coil_type_t type);

/**
 * @brief Adds a basic block to a function
 * @param function Function to add to
 * @param block Block to add
 * @return 0 on success, non-zero on failure
 */
int coil_function_add_block(coil_function_t *function, 
                           coil_basic_block_t *block);

/**
 * @brief Gets a string representation of an instruction
 * @param inst The instruction to convert
 * @param buffer Output buffer
 * @param size Size of the buffer
 * @return The buffer pointer
 */
char* coil_instruction_to_string(const coil_instruction_t *inst, 
                                char *buffer, size_t size);

#endif /* COIL_INSTRUCTIONS_H */