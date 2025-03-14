/**
 * @file x86_64.h
 * @brief x86_64 target backend definitions
 * @details This file contains target-specific definitions for the x86_64 backend.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_TARGET_X86_64_H
#define COIL_TARGET_X86_64_H

#include "coil-assembler/target.h"

/**
 * @brief Initialize the x86_64 target backend
 * @param context Target context to initialize
 * @return 0 on success, non-zero on failure
 */
int x86_64_target_initialize(coil_target_context_t *context);

/**
 * @brief Finalize the x86_64 target backend
 * @param context Target context to finalize
 * @return 0 on success, non-zero on failure
 */
int x86_64_target_finalize(coil_target_context_t *context);

/**
 * @brief Map a COIL instruction to x86_64 instructions
 * @param context Target context
 * @param instruction COIL instruction to map
 * @return 0 on success, non-zero on failure
 */
int x86_64_target_map_instruction(coil_target_context_t *context,
                               coil_instruction_t *instruction);

/**
 * @brief Generate native code for a function
 * @param context Target context
 * @param function COIL function to generate
 * @return 0 on success, non-zero on failure
 */
int x86_64_target_generate_function(coil_target_context_t *context,
                                 coil_function_t *function);

/**
 * @brief Register the x86_64 target with the framework
 */
void register_x86_64_target(void);

/**
 * @brief Apply x86_64 ABI calling convention to a function
 * @param context Target context
 * @param function COIL function to process
 * @return 0 on success, non-zero on failure
 */
int x86_64_target_apply_calling_convention(coil_target_context_t *context,
                                        coil_function_t *function);

/**
 * @brief Register x86_64-specific optimization passes
 * @param context Target context
 * @return 0 on success, non-zero on failure
 */
int x86_64_target_register_optimizations(coil_target_context_t *context);

/**
 * @brief Perform x86_64-specific peephole optimizations
 * @param context Target context
 * @param instructions Instructions to optimize
 * @param count Number of instructions
 * @return 0 on success, non-zero on failure
 */
int x86_64_target_optimize_instruction_sequence(coil_target_context_t *context,
                                             coil_instruction_t *instructions,
                                             uint32_t count);

/* Target-specific constants and defines below */

/**
 * @brief x86_64 register classes
 */
typedef enum {
  X86_64_REG_CLASS_GPR = 0,    /**< General-purpose registers */
  X86_64_REG_CLASS_XMM = 1,    /**< SSE/AVX registers */
  X86_64_REG_CLASS_YMM = 2,    /**< AVX registers */
  X86_64_REG_CLASS_ZMM = 3,    /**< AVX-512 registers */
  X86_64_REG_CLASS_MASK = 4,   /**< AVX-512 mask registers */
  X86_64_REG_CLASS_SEGMENT = 5, /**< Segment registers */
  X86_64_REG_CLASS_CONTROL = 6, /**< Control registers */
  X86_64_REG_CLASS_DEBUG = 7    /**< Debug registers */
} x86_64_register_class_t;

/**
 * @brief x86_64 general-purpose registers
 */
typedef enum {
  /* 64-bit registers */
  X86_64_REG_RAX = 0,
  X86_64_REG_RCX = 1,
  X86_64_REG_RDX = 2,
  X86_64_REG_RBX = 3,
  X86_64_REG_RSP = 4,
  X86_64_REG_RBP = 5,
  X86_64_REG_RSI = 6,
  X86_64_REG_RDI = 7,
  X86_64_REG_R8 = 8,
  X86_64_REG_R9 = 9,
  X86_64_REG_R10 = 10,
  X86_64_REG_R11 = 11,
  X86_64_REG_R12 = 12,
  X86_64_REG_R13 = 13,
  X86_64_REG_R14 = 14,
  X86_64_REG_R15 = 15,

  /* 32-bit registers */
  X86_64_REG_EAX = 16,
  X86_64_REG_ECX = 17,
  X86_64_REG_EDX = 18,
  X86_64_REG_EBX = 19,
  X86_64_REG_ESP = 20,
  X86_64_REG_EBP = 21,
  X86_64_REG_ESI = 22,
  X86_64_REG_EDI = 23,
  X86_64_REG_R8D = 24,
  X86_64_REG_R9D = 25,
  X86_64_REG_R10D = 26,
  X86_64_REG_R11D = 27,
  X86_64_REG_R12D = 28,
  X86_64_REG_R13D = 29,
  X86_64_REG_R14D = 30,
  X86_64_REG_R15D = 31,

  /* 16-bit registers */
  X86_64_REG_AX = 32,
  X86_64_REG_CX = 33,
  X86_64_REG_DX = 34,
  X86_64_REG_BX = 35,
  X86_64_REG_SP = 36,
  X86_64_REG_BP = 37,
  X86_64_REG_SI = 38,
  X86_64_REG_DI = 39,
  X86_64_REG_R8W = 40,
  X86_64_REG_R9W = 41,
  X86_64_REG_R10W = 42,
  X86_64_REG_R11W = 43,
  X86_64_REG_R12W = 44,
  X86_64_REG_R13W = 45,
  X86_64_REG_R14W = 46,
  X86_64_REG_R15W = 47,

  /* 8-bit registers */
  X86_64_REG_AL = 48,
  X86_64_REG_CL = 49,
  X86_64_REG_DL = 50,
  X86_64_REG_BL = 51,
  X86_64_REG_SPL = 52,
  X86_64_REG_BPL = 53,
  X86_64_REG_SIL = 54,
  X86_64_REG_DIL = 55,
  X86_64_REG_R8B = 56,
  X86_64_REG_R9B = 57,
  X86_64_REG_R10B = 58,
  X86_64_REG_R11B = 59,
  X86_64_REG_R12B = 60,
  X86_64_REG_R13B = 61,
  X86_64_REG_R14B = 62,
  X86_64_REG_R15B = 63,

  /* Legacy 8-bit high registers */
  X86_64_REG_AH = 64,
  X86_64_REG_CH = 65,
  X86_64_REG_DH = 66,
  X86_64_REG_BH = 67
} x86_64_gpr_register_t;

/**
 * @brief x86_64 SSE/AVX registers
 */
typedef enum {
  X86_64_REG_XMM0 = 0,
  X86_64_REG_XMM1 = 1,
  X86_64_REG_XMM2 = 2,
  X86_64_REG_XMM3 = 3,
  X86_64_REG_XMM4 = 4,
  X86_64_REG_XMM5 = 5,
  X86_64_REG_XMM6 = 6,
  X86_64_REG_XMM7 = 7,
  X86_64_REG_XMM8 = 8,
  X86_64_REG_XMM9 = 9,
  X86_64_REG_XMM10 = 10,
  X86_64_REG_XMM11 = 11,
  X86_64_REG_XMM12 = 12,
  X86_64_REG_XMM13 = 13,
  X86_64_REG_XMM14 = 14,
  X86_64_REG_XMM15 = 15,
  X86_64_REG_XMM16 = 16,
  X86_64_REG_XMM17 = 17,
  X86_64_REG_XMM18 = 18,
  X86_64_REG_XMM19 = 19,
  X86_64_REG_XMM20 = 20,
  X86_64_REG_XMM21 = 21,
  X86_64_REG_XMM22 = 22,
  X86_64_REG_XMM23 = 23,
  X86_64_REG_XMM24 = 24,
  X86_64_REG_XMM25 = 25,
  X86_64_REG_XMM26 = 26,
  X86_64_REG_XMM27 = 27,
  X86_64_REG_XMM28 = 28,
  X86_64_REG_XMM29 = 29,
  X86_64_REG_XMM30 = 30,
  X86_64_REG_XMM31 = 31
} x86_64_xmm_register_t;

/**
 * @brief x86_64 instruction set features
 */
typedef enum {
  X86_64_FEATURE_BASIC = 0,    /**< Basic x86_64 instruction set */
  X86_64_FEATURE_SSE = 1,      /**< SSE instructions */
  X86_64_FEATURE_SSE2 = 2,     /**< SSE2 instructions */
  X86_64_FEATURE_SSE3 = 3,     /**< SSE3 instructions */
  X86_64_FEATURE_SSSE3 = 4,    /**< SSSE3 instructions */
  X86_64_FEATURE_SSE4_1 = 5,   /**< SSE4.1 instructions */
  X86_64_FEATURE_SSE4_2 = 6,   /**< SSE4.2 instructions */
  X86_64_FEATURE_AVX = 7,      /**< AVX instructions */
  X86_64_FEATURE_AVX2 = 8,     /**< AVX2 instructions */
  X86_64_FEATURE_AVX512F = 9,  /**< AVX-512 Foundation */
  X86_64_FEATURE_AVX512BW = 10, /**< AVX-512 Byte and Word */
  X86_64_FEATURE_AVX512DQ = 11, /**< AVX-512 Doubleword and Quadword */
  X86_64_FEATURE_AVX512VL = 12, /**< AVX-512 Vector Length Extensions */
  X86_64_FEATURE_FMA = 13,     /**< Fused Multiply-Add */
  X86_64_FEATURE_BMI1 = 14,    /**< Bit Manipulation Instruction Set 1 */
  X86_64_FEATURE_BMI2 = 15,    /**< Bit Manipulation Instruction Set 2 */
  X86_64_FEATURE_POPCNT = 16,  /**< POPCNT instruction */
  X86_64_FEATURE_LZCNT = 17,   /**< LZCNT instruction */
  X86_64_FEATURE_ADX = 18      /**< Multi-Precision Add-Carry Instructions */
} x86_64_feature_t;

/**
 * @brief x86_64 addressing modes
 */
typedef enum {
  X86_64_ADDR_MODE_DIRECT = 0,        /**< Direct addressing */
  X86_64_ADDR_MODE_INDIRECT = 1,      /**< Indirect addressing */
  X86_64_ADDR_MODE_INDEXED = 2,       /**< Indexed addressing */
  X86_64_ADDR_MODE_BASED_INDEXED = 3, /**< Based indexed addressing */
  X86_64_ADDR_MODE_RIP_RELATIVE = 4   /**< RIP-relative addressing */
} x86_64_addressing_mode_t;

/**
 * @brief x86_64 operand types
 */
typedef enum {
  X86_64_OPERAND_NONE = 0,      /**< No operand */
  X86_64_OPERAND_REG = 1,       /**< Register operand */
  X86_64_OPERAND_IMM = 2,       /**< Immediate operand */
  X86_64_OPERAND_MEM = 3,       /**< Memory operand */
  X86_64_OPERAND_REL = 4        /**< Relative offset */
} x86_64_operand_type_t;

/**
 * @brief x86_64 operand structure
 */
typedef struct {
  uint8_t type;                 /**< Operand type */
  uint8_t size;                 /**< Operand size in bytes */
  union {
    struct {
      uint8_t reg_class;        /**< Register class */
      uint8_t reg_id;           /**< Register ID */
    } reg;                      /**< Register operand */
    int64_t imm;                /**< Immediate value */
    struct {
      uint8_t base_reg;         /**< Base register */
      uint8_t index_reg;        /**< Index register */
      uint8_t scale;            /**< Scale factor (1, 2, 4, or 8) */
      int32_t disp;             /**< Displacement */
      uint8_t addr_mode;        /**< Addressing mode */
    } mem;                      /**< Memory operand */
    int32_t rel;                /**< Relative offset */
  } value;                      /**< Operand value */
} x86_64_operand_t;

/**
 * @brief x86_64 instruction structure
 */
typedef struct {
  uint16_t opcode;              /**< Instruction opcode */
  uint8_t prefix;               /**< Instruction prefix */
  uint8_t flags;                /**< Instruction flags */
  uint8_t operand_count;        /**< Number of operands */
  x86_64_operand_t operands[4]; /**< Instruction operands */
} x86_64_instruction_t;

/**
 * @brief Map a COIL register to an x86_64 register
 * @param context Target context
 * @param reg_id COIL register ID
 * @param reg_type COIL register type
 * @return x86_64 register ID or 0 if not mappable
 */
uint32_t x86_64_target_map_register(coil_target_context_t *context,
                                  uint32_t reg_id,
                                  coil_type_t reg_type);

/**
 * @brief Get the string representation of an x86_64 register
 * @param reg_class Register class
 * @param reg_id Register ID
 * @return Register name or NULL if invalid
 */
const char* x86_64_target_get_register_name(uint8_t reg_class, uint8_t reg_id);

/**
 * @brief Get the appropriate register size for a type
 * @param type COIL type
 * @return Register size in bytes
 */
uint8_t x86_64_target_get_register_size(coil_type_t type);

/**
 * @brief Check if a feature is supported by the target
 * @param context Target context
 * @param feature Feature to check
 * @return 1 if supported, 0 if not
 */
int x86_64_target_has_feature(coil_target_context_t *context,
                            x86_64_feature_t feature);

#endif /* COIL_TARGET_X86_64_H */