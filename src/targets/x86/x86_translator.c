/**
 * @file x86_translator.c
 * @brief Implementation of x86-specific instruction translation
 * 
 * This module implements x86-specific translations for COIL instructions,
 * supporting 16-bit, 32-bit, and 64-bit modes.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "x86_translator.h"
#include "../../utils/memory_management.h"
#include "../../utils/logging.h"
#include "../../core/translator.h"
#include "../../core/instruction_decoder.h"

/**
 * @brief Maximum number of registers per class
 */
#define MAX_REGISTERS_PER_CLASS 32

/**
 * @brief COIL to x86 register mapping entry
 */
typedef struct {
  uint32_t coil_reg_id;                   /**< COIL register ID */
  x86_register_info_t* x86_reg;           /**< Corresponding x86 register */
} register_mapping_t;

/**
 * @brief COIL to x86 register mapping table
 */
typedef struct {
  register_mapping_t mappings[256];       /**< Register mappings */
  size_t count;                           /**< Number of mappings */
} register_map_t;

/**
 * @brief Definition of x86 registers
 */
static x86_register_info_t x86_registers[] = {
  /* 8-bit general registers */
  { "al", X86_REG_CLASS_GENERAL, X86_REG_SIZE_8, 0, 0 },
  { "cl", X86_REG_CLASS_GENERAL, X86_REG_SIZE_8, 1, 1 },
  { "dl", X86_REG_CLASS_GENERAL, X86_REG_SIZE_8, 2, 2 },
  { "bl", X86_REG_CLASS_GENERAL, X86_REG_SIZE_8, 3, 3 },
  { "ah", X86_REG_CLASS_GENERAL, X86_REG_SIZE_8, 4, 4 },
  { "ch", X86_REG_CLASS_GENERAL, X86_REG_SIZE_8, 5, 5 },
  { "dh", X86_REG_CLASS_GENERAL, X86_REG_SIZE_8, 6, 6 },
  { "bh", X86_REG_CLASS_GENERAL, X86_REG_SIZE_8, 7, 7 },
  
  /* 16-bit general registers */
  { "ax", X86_REG_CLASS_GENERAL, X86_REG_SIZE_16, 0, 0 },
  { "cx", X86_REG_CLASS_GENERAL, X86_REG_SIZE_16, 1, 1 },
  { "dx", X86_REG_CLASS_GENERAL, X86_REG_SIZE_16, 2, 2 },
  { "bx", X86_REG_CLASS_GENERAL, X86_REG_SIZE_16, 3, 3 },
  { "sp", X86_REG_CLASS_GENERAL, X86_REG_SIZE_16, 4, 4 },
  { "bp", X86_REG_CLASS_GENERAL, X86_REG_SIZE_16, 5, 5 },
  { "si", X86_REG_CLASS_GENERAL, X86_REG_SIZE_16, 6, 6 },
  { "di", X86_REG_CLASS_GENERAL, X86_REG_SIZE_16, 7, 7 },
  
  /* 32-bit general registers */
  { "eax", X86_REG_CLASS_GENERAL, X86_REG_SIZE_32, 0, 0 },
  { "ecx", X86_REG_CLASS_GENERAL, X86_REG_SIZE_32, 1, 1 },
  { "edx", X86_REG_CLASS_GENERAL, X86_REG_SIZE_32, 2, 2 },
  { "ebx", X86_REG_CLASS_GENERAL, X86_REG_SIZE_32, 3, 3 },
  { "esp", X86_REG_CLASS_GENERAL, X86_REG_SIZE_32, 4, 4 },
  { "ebp", X86_REG_CLASS_GENERAL, X86_REG_SIZE_32, 5, 5 },
  { "esi", X86_REG_CLASS_GENERAL, X86_REG_SIZE_32, 6, 6 },
  { "edi", X86_REG_CLASS_GENERAL, X86_REG_SIZE_32, 7, 7 },
  
  /* 64-bit general registers */
  { "rax", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 0, 0 },
  { "rcx", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 1, 1 },
  { "rdx", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 2, 2 },
  { "rbx", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 3, 3 },
  { "rsp", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 4, 4 },
  { "rbp", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 5, 5 },
  { "rsi", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 6, 6 },
  { "rdi", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 7, 7 },
  { "r8", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 8, 8 },
  { "r9", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 9, 9 },
  { "r10", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 10, 10 },
  { "r11", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 11, 11 },
  { "r12", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 12, 12 },
  { "r13", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 13, 13 },
  { "r14", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 14, 14 },
  { "r15", X86_REG_CLASS_GENERAL, X86_REG_SIZE_64, 15, 15 },
  
  /* Segment registers */
  { "cs", X86_REG_CLASS_SEGMENT, X86_REG_SIZE_16, 0, 1 },
  { "ds", X86_REG_CLASS_SEGMENT, X86_REG_SIZE_16, 1, 3 },
  { "es", X86_REG_CLASS_SEGMENT, X86_REG_SIZE_16, 2, 0 },
  { "fs", X86_REG_CLASS_SEGMENT, X86_REG_SIZE_16, 3, 4 },
  { "gs", X86_REG_CLASS_SEGMENT, X86_REG_SIZE_16, 4, 5 },
  { "ss", X86_REG_CLASS_SEGMENT, X86_REG_SIZE_16, 5, 2 },
  
  /* Floating-point registers (x87) */
  { "st0", X86_REG_CLASS_FLOATING, X86_REG_SIZE_64, 0, 0 },
  { "st1", X86_REG_CLASS_FLOATING, X86_REG_SIZE_64, 1, 1 },
  { "st2", X86_REG_CLASS_FLOATING, X86_REG_SIZE_64, 2, 2 },
  { "st3", X86_REG_CLASS_FLOATING, X86_REG_SIZE_64, 3, 3 },
  { "st4", X86_REG_CLASS_FLOATING, X86_REG_SIZE_64, 4, 4 },
  { "st5", X86_REG_CLASS_FLOATING, X86_REG_SIZE_64, 5, 5 },
  { "st6", X86_REG_CLASS_FLOATING, X86_REG_SIZE_64, 6, 6 },
  { "st7", X86_REG_CLASS_FLOATING, X86_REG_SIZE_64, 7, 7 },
  
  /* MMX registers */
  { "mm0", X86_REG_CLASS_MMX, X86_REG_SIZE_64, 0, 0 },
  { "mm1", X86_REG_CLASS_MMX, X86_REG_SIZE_64, 1, 1 },
  { "mm2", X86_REG_CLASS_MMX, X86_REG_SIZE_64, 2, 2 },
  { "mm3", X86_REG_CLASS_MMX, X86_REG_SIZE_64, 3, 3 },
  { "mm4", X86_REG_CLASS_MMX, X86_REG_SIZE_64, 4, 4 },
  { "mm5", X86_REG_CLASS_MMX, X86_REG_SIZE_64, 5, 5 },
  { "mm6", X86_REG_CLASS_MMX, X86_REG_SIZE_64, 6, 6 },
  { "mm7", X86_REG_CLASS_MMX, X86_REG_SIZE_64, 7, 7 },
  
  /* SSE registers (128-bit) */
  { "xmm0", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 0, 0 },
  { "xmm1", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 1, 1 },
  { "xmm2", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 2, 2 },
  { "xmm3", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 3, 3 },
  { "xmm4", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 4, 4 },
  { "xmm5", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 5, 5 },
  { "xmm6", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 6, 6 },
  { "xmm7", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 7, 7 },
  { "xmm8", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 8, 8 },
  { "xmm9", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 9, 9 },
  { "xmm10", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 10, 10 },
  { "xmm11", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 11, 11 },
  { "xmm12", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 12, 12 },
  { "xmm13", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 13, 13 },
  { "xmm14", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 14, 14 },
  { "xmm15", X86_REG_CLASS_SSE, X86_REG_SIZE_128, 15, 15 },
  
  /* AVX registers (256-bit) */
  { "ymm0", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 0, 0 },
  { "ymm1", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 1, 1 },
  { "ymm2", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 2, 2 },
  { "ymm3", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 3, 3 },
  { "ymm4", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 4, 4 },
  { "ymm5", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 5, 5 },
  { "ymm6", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 6, 6 },
  { "ymm7", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 7, 7 },
  { "ymm8", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 8, 8 },
  { "ymm9", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 9, 9 },
  { "ymm10", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 10, 10 },
  { "ymm11", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 11, 11 },
  { "ymm12", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 12, 12 },
  { "ymm13", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 13, 13 },
  { "ymm14", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 14, 14 },
  { "ymm15", X86_REG_CLASS_AVX, X86_REG_SIZE_256, 15, 15 },
  
  /* Sentinel */
  { NULL, 0, 0, 0, 0 }
};

/**
 * @brief Looks up an x86 register by name
 *
 * @param[in] name Register name
 * @return Pointer to register info or NULL if not found
 */
static x86_register_info_t* find_register_by_name(const char* name) {
  if (name == NULL) {
    return NULL;
  }
  
  for (size_t i = 0; x86_registers[i].name != NULL; i++) {
    if (strcmp(x86_registers[i].name, name) == 0) {
      return &x86_registers[i];
    }
  }
  
  return NULL;
}

/**
 * @brief Looks up an x86 register by class and ID
 *
 * @param[in] reg_class Register class
 * @param[in] id Register ID within class
 * @param[in] size Register size in bytes
 * @return Pointer to register info or NULL if not found
 */
static x86_register_info_t* find_register_by_class_id(
  x86_reg_class_t reg_class,
  uint8_t id,
  x86_reg_size_t size
) {
  for (size_t i = 0; x86_registers[i].name != NULL; i++) {
    if (x86_registers[i].reg_class == reg_class &&
        x86_registers[i].id == id &&
        x86_registers[i].size == size) {
      return &x86_registers[i];
    }
  }
  
  return NULL;
}

/**
 * @brief Adds a COIL to x86 register mapping
 *
 * @param[in,out] map Register mapping table
 * @param[in] coil_reg_id COIL register ID
 * @param[in] x86_reg x86 register information
 * @return Error code indicating success or failure
 */
static error_t add_register_mapping(
  register_map_t* map,
  uint32_t coil_reg_id,
  x86_register_info_t* x86_reg
) {
  if (map == NULL || x86_reg == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if mapping already exists */
  for (size_t i = 0; i < map->count; i++) {
    if (map->mappings[i].coil_reg_id == coil_reg_id) {
      map->mappings[i].x86_reg = x86_reg;
      return ERROR_NONE;
    }
  }
  
  /* Add new mapping */
  if (map->count >= 256) {
    log_error("Maximum register mapping count exceeded");
    return ERROR_MEMORY;
  }
  
  map->mappings[map->count].coil_reg_id = coil_reg_id;
  map->mappings[map->count].x86_reg = x86_reg;
  map->count++;
  
  return ERROR_NONE;
}

/**
 * @brief Looks up an x86 register for a COIL register ID
 *
 * @param[in] map Register mapping table
 * @param[in] coil_reg_id COIL register ID
 * @return Pointer to x86 register info or NULL if not found
 */
static x86_register_info_t* find_x86_register_for_coil(
  register_map_t* map,
  uint32_t coil_reg_id
) {
  if (map == NULL) {
    return NULL;
  }
  
  for (size_t i = 0; i < map->count; i++) {
    if (map->mappings[i].coil_reg_id == coil_reg_id) {
      return map->mappings[i].x86_reg;
    }
  }
  
  return NULL;
}

/**
 * @brief Creates initial register mappings based on target features
 *
 * @param[in] data x86 translator data
 * @param[in,out] map Register mapping table
 * @return Error code indicating success or failure
 */
static error_t create_register_mappings(
  x86_translator_data_t* data,
  register_map_t* map
) {
  if (data == NULL || map == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Initialize mapping table */
  map->count = 0;
  
  /* The exact mapping would depend on the COIL specification.
   * This is a simplified mapping for demonstration purposes:
   * - COIL registers 0-7 map to general-purpose registers
   * - COIL registers 8-15 map to floating-point registers
   * - COIL registers 16-31 map to vector registers
   */
  
  x86_reg_size_t gpr_size = X86_REG_SIZE_32;  /* Default to 32-bit */
  
  if (data->is_64bit) {
    gpr_size = X86_REG_SIZE_64;
  } else if (!data->is_64bit && !data->is_32bit) {
    gpr_size = X86_REG_SIZE_16;  /* 16-bit mode */
  }
  
  /* Map general-purpose registers */
  for (uint32_t i = 0; i < 8; i++) {
    x86_register_info_t* reg = find_register_by_class_id(
      X86_REG_CLASS_GENERAL,
      i,
      gpr_size
    );
    
    if (reg != NULL) {
      add_register_mapping(map, i, reg);
    }
  }
  
  /* Additional registers in 64-bit mode */
  if (data->is_64bit) {
    for (uint32_t i = 8; i < 16; i++) {
      x86_register_info_t* reg = find_register_by_class_id(
        X86_REG_CLASS_GENERAL,
        i,
        X86_REG_SIZE_64
      );
      
      if (reg != NULL) {
        add_register_mapping(map, i, reg);
      }
    }
  }
  
  /* Map floating-point registers */
  for (uint32_t i = 0; i < 8; i++) {
    x86_register_info_t* reg = find_register_by_class_id(
      X86_REG_CLASS_FLOATING,
      i,
      X86_REG_SIZE_64
    );
    
    if (reg != NULL) {
      add_register_mapping(map, i + 8, reg);
    }
  }
  
  /* Map vector registers based on available features */
  x86_reg_class_t vec_class = X86_REG_CLASS_SSE;
  x86_reg_size_t vec_size = X86_REG_SIZE_128;
  
  if (data->has_avx) {
    vec_class = X86_REG_CLASS_AVX;
    vec_size = X86_REG_SIZE_256;
  }
  
  for (uint32_t i = 0; i < 16; i++) {
    x86_register_info_t* reg = find_register_by_class_id(
      vec_class,
      i,
      vec_size
    );
    
    if (reg != NULL) {
      add_register_mapping(map, i + 16, reg);
    }
  }
  
  log_debug("Created %zu register mappings", map->count);
  
  return ERROR_NONE;
}

error_t x86_translator_init(translator_context_t* context) {
  if (context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Allocate x86-specific data */
  x86_translator_data_t* data = memory_calloc(1, sizeof(x86_translator_data_t));
  if (data == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Determine architecture features */
  bool has_x86_64 = target_config_has_feature(context->target, "x86_64");
  bool has_i386 = target_config_has_feature(context->target, "i386");
  
  data->is_64bit = has_x86_64;
  data->is_32bit = has_i386 && !has_x86_64;
  
  /* Check for SIMD features */
  data->has_avx = target_config_has_feature(context->target, "avx");
  data->has_avx2 = target_config_has_feature(context->target, "avx2");
  data->has_avx512 = target_config_has_feature(context->target, "avx512f");
  data->has_sse = target_config_has_feature(context->target, "sse");
  data->has_sse2 = target_config_has_feature(context->target, "sse2");
  data->has_sse3 = target_config_has_feature(context->target, "sse3");
  data->has_sse4_1 = target_config_has_feature(context->target, "sse4.1");
  data->has_sse4_2 = target_config_has_feature(context->target, "sse4.2");
  data->has_fma = target_config_has_feature(context->target, "fma");
  
  /* Create register mapping */
  register_map_t* reg_map = memory_calloc(1, sizeof(register_map_t));
  if (reg_map == NULL) {
    memory_free(data);
    return ERROR_MEMORY;
  }
  
  error_t map_result = create_register_mappings(data, reg_map);
  if (map_result != ERROR_NONE) {
    memory_free(reg_map);
    memory_free(data);
    return map_result;
  }
  
  data->register_map = reg_map;
  context->arch_specific = data;
  
  /* Register x86 translators */
  x86_translator_register();
  
  log_info("Initialized x86 translator in %s-bit mode",
          data->is_64bit ? "64" : data->is_32bit ? "32" : "16");
  
  return ERROR_NONE;
}

error_t x86_translator_cleanup(translator_context_t* context) {
  if (context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  x86_translator_data_t* data = (x86_translator_data_t*)context->arch_specific;
  if (data == NULL) {
    return ERROR_NONE;  /* Nothing to clean up */
  }
  
  /* Free register mapping */
  if (data->register_map != NULL) {
    memory_free(data->register_map);
  }
  
  /* Free translator data */
  memory_free(data);
  context->arch_specific = NULL;
  
  return ERROR_NONE;
}

error_t x86_map_register(
  uint32_t coil_reg,
  translator_context_t* context,
  native_operand_t* native_reg
) {
  if (context == NULL || native_reg == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  x86_translator_data_t* data = (x86_translator_data_t*)context->arch_specific;
  if (data == NULL || data->register_map == NULL) {
    return ERROR_TRANSLATION;
  }
  
  register_map_t* map = (register_map_t*)data->register_map;
  x86_register_info_t* reg = find_x86_register_for_coil(map, coil_reg);
  
  if (reg == NULL) {
    /* Handle dynamically allocating a register if not mapped */
    /* For now, just report an error */
    log_error("No x86 register mapping for COIL register %u", coil_reg);
    return ERROR_UNSUPPORTED;
  }
  
  /* Create register operand */
  return x86_create_register_operand(reg, native_reg);
}

error_t x86_create_register_operand(
  const x86_register_info_t* reg_info,
  native_operand_t* operand
) {
  if (reg_info == NULL || operand == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Initialize operand */
  memset(operand, 0, sizeof(native_operand_t));
  
  operand->type = NATIVE_OPERAND_REGISTER;
  operand->value.reg.id = reg_info->id;
  strncpy(operand->value.reg.name, reg_info->name, sizeof(operand->value.reg.name) - 1);
  
  /* Set size based on register class */
  switch (reg_info->size) {
    case X86_REG_SIZE_8:
      operand->size = 1;
      break;
    case X86_REG_SIZE_16:
      operand->size = 2;
      break;
    case X86_REG_SIZE_32:
      operand->size = 4;
      break;
    case X86_REG_SIZE_64:
      operand->size = 8;
      break;
    case X86_REG_SIZE_128:
      operand->size = 16;
      break;
    case X86_REG_SIZE_256:
      operand->size = 32;
      break;
    case X86_REG_SIZE_512:
      operand->size = 64;
      break;
    default:
      operand->size = 4;  /* Default to 32-bit size */
      break;
  }
  
  return ERROR_NONE;
}

error_t x86_create_memory_operand(
  const x86_register_info_t* base_reg,
  const x86_register_info_t* index_reg,
  uint8_t scale,
  int32_t displacement,
  uint8_t size,
  uint8_t segment,
  native_operand_t* operand
) {
  if (operand == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Initialize operand */
  memset(operand, 0, sizeof(native_operand_t));
  
  operand->type = NATIVE_OPERAND_MEMORY;
  operand->size = size;
  
  /* Set base register */
  if (base_reg != NULL) {
    operand->value.mem.base_reg = base_reg->id;
  } else {
    operand->value.mem.base_reg = 0;
  }
  
  /* Set index register */
  if (index_reg != NULL) {
    operand->value.mem.index_reg = index_reg->id;
  } else {
    operand->value.mem.index_reg = 0;
  }
  
  /* Set scale factor */
  operand->value.mem.scale = scale;
  
  /* Set displacement */
  operand->value.mem.displacement = displacement;
  
  /* Set segment override */
  operand->value.mem.segment = segment;
  
  return ERROR_NONE;
}

error_t x86_create_immediate_operand(
  int64_t value,
  uint8_t size,
  native_operand_t* operand
) {
  if (operand == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Initialize operand */
  memset(operand, 0, sizeof(native_operand_t));
  
  operand->type = NATIVE_OPERAND_IMMEDIATE;
  operand->value.imm = value;
  operand->size = size;
  
  return ERROR_NONE;
}

/**
 * @brief Creates a native x86 instruction
 *
 * @param[in] mnemonic Instruction mnemonic
 * @param[in] operands Array of operands
 * @param[in] operand_count Number of operands
 * @param[in] context Translation context
 * @return Error code indicating success or failure
 */
static error_t create_x86_instruction(
  const char* mnemonic,
  native_operand_t* operands,
  uint8_t operand_count,
  translator_context_t* context
) {
  if (mnemonic == NULL || (operand_count > 0 && operands == NULL) || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  native_instruction_t instruction;
  memset(&instruction, 0, sizeof(native_instruction_t));
  
  /* Set mnemonic */
  strncpy(instruction.mnemonic, mnemonic, sizeof(instruction.mnemonic) - 1);
  
  /* Copy operands */
  instruction.operand_count = operand_count;
  for (uint8_t i = 0; i < operand_count && i < MAX_NATIVE_OPERANDS; i++) {
    instruction.operands[i] = operands[i];
  }
  
  /* Calculate total instruction size (simplified) */
  instruction.size = 1;  /* At least 1 byte for opcode */
  for (uint8_t i = 0; i < operand_count && i < MAX_NATIVE_OPERANDS; i++) {
    if (instruction.operands[i].type == NATIVE_OPERAND_MEMORY) {
      instruction.size += 2;  /* ModR/M + SIB */
      if (instruction.operands[i].value.mem.displacement != 0) {
        instruction.size += 4;  /* Displacement */
      }
    } else if (instruction.operands[i].type == NATIVE_OPERAND_IMMEDIATE) {
      instruction.size += instruction.operands[i].size;
    }
  }
  
  /* Add prefix for 16-bit or 64-bit operands */
  x86_translator_data_t* data = (x86_translator_data_t*)context->arch_specific;
  if (data != NULL) {
    if (data->is_64bit) {
      for (uint8_t i = 0; i < operand_count && i < MAX_NATIVE_OPERANDS; i++) {
        if (instruction.operands[i].size == 8) {
          /* REX prefix for 64-bit operands */
          instruction.prefix[instruction.prefix_count++] = 0x48;
          instruction.size++;
          break;
        }
      }
    } else if (data->is_32bit) {
      for (uint8_t i = 0; i < operand_count && i < MAX_NATIVE_OPERANDS; i++) {
        if (instruction.operands[i].size == 2) {
          /* Operand size override prefix for 16-bit operands in 32-bit mode */
          instruction.prefix[instruction.prefix_count++] = 0x66;
          instruction.size++;
          break;
        }
      }
    } else {
      /* 16-bit mode */
      for (uint8_t i = 0; i < operand_count && i < MAX_NATIVE_OPERANDS; i++) {
        if (instruction.operands[i].size == 4) {
          /* Operand size override prefix for 32-bit operands in 16-bit mode */
          instruction.prefix[instruction.prefix_count++] = 0x66;
          instruction.size++;
          break;
        }
      }
    }
  }
  
  /* Add instruction to output */
  return translator_add_instruction(context->output, &instruction);
}

/**
 * @brief Gets the appropriate mnemonic suffix for size
 *
 * @param[in] size Size in bytes
 * @return Suffix string ("b", "w", "l", "q", or empty string)
 */
static const char* get_size_suffix(uint8_t size) {
  switch (size) {
    case 1:
      return "b";  /* byte */
    case 2:
      return "w";  /* word */
    case 4:
      return "l";  /* long */
    case 8:
      return "q";  /* quad */
    default:
      return "";   /* no suffix */
  }
}

error_t x86_translate_arithmetic(
  const coil_instruction_t* instruction,
  translator_context_t* context
) {
  if (instruction == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Map COIL arithmetic instructions to x86 instructions */
  const char* base_mnemonic = NULL;
  
  switch (instruction->opcode) {
    case COIL_OP_ADD:
      base_mnemonic = "add";
      break;
    case COIL_OP_SUB:
      base_mnemonic = "sub";
      break;
    case COIL_OP_MUL:
      base_mnemonic = "imul";
      break;
    case COIL_OP_DIV:
      base_mnemonic = "idiv";
      break;
    case COIL_OP_REM:
      /* x86 doesn't have a direct remainder instruction, need to use div/idiv */
      log_error("Remainder operation needs special handling");
      return ERROR_UNSUPPORTED;
    case COIL_OP_NEG:
      base_mnemonic = "neg";
      break;
    case COIL_OP_ABS:
      /* x86 doesn't have a direct abs instruction, need a sequence */
      log_error("Absolute value operation needs a sequence of instructions");
      return ERROR_UNSUPPORTED;
    case COIL_OP_MIN:
    case COIL_OP_MAX:
      /* x86 doesn't have direct min/max instructions (except in SSE) */
      log_error("Min/Max operations need special handling");
      return ERROR_UNSUPPORTED;
    case COIL_OP_FMA:
      /* Need to use dedicated FMA instructions if available */
      if (((x86_translator_data_t*)context->arch_specific)->has_fma) {
        base_mnemonic = "vfmadd231";  /* Use FMA3 instruction */
      } else {
        log_error("FMA operation requires FMA support");
        return ERROR_UNSUPPORTED;
      }
      break;
    default:
      log_error("Unknown arithmetic operation: 0x%02X", instruction->opcode);
      return ERROR_UNSUPPORTED;
  }
  
  /* Determine operand size */
  uint8_t op_size = 4;  /* Default to 4 bytes (32-bit) */
  
  switch (instruction->data_type) {
    case 1:  /* Assuming 1 is 8-bit integer */
      op_size = 1;
      break;
    case 2:  /* Assuming 2 is 16-bit integer */
      op_size = 2;
      break;
    case 3:  /* Assuming 3 is 32-bit integer */
      op_size = 4;
      break;
    case 4:  /* Assuming 4 is 64-bit integer */
      op_size = 8;
      break;
    default:
      break;
  }
  
  /* Special handling for certain instructions */
  if (instruction->opcode == COIL_OP_DIV) {
    /* x86 div is more complex, need to set up registers properly */
    /* This is a simplified implementation */
    native_operand_t operands[1];
    
    /* Map the divisor operand */
    error_t src_result = translator_map_operand(
      &instruction->operands[1],
      context,
      &operands[0]
    );
    
    if (src_result != ERROR_NONE) {
      return src_result;
    }
    
    /* Create div instruction (assumes eax/rax contains dividend and result goes to eax/rdx) */
    char mnemonic[MAX_MNEMONIC_LENGTH];
    snprintf(mnemonic, sizeof(mnemonic), "%s%s", base_mnemonic, get_size_suffix(op_size));
    
    return create_x86_instruction(mnemonic, operands, 1, context);
  } else if (instruction->opcode == COIL_OP_NEG) {
    /* Unary operation - needs special handling */
    native_operand_t operands[1];
    
    /* Map the single operand */
    error_t src_result = translator_map_operand(
      &instruction->operands[0],
      context,
      &operands[0]
    );
    
    if (src_result != ERROR_NONE) {
      return src_result;
    }
    
    /* Create neg instruction */
    char mnemonic[MAX_MNEMONIC_LENGTH];
    snprintf(mnemonic, sizeof(mnemonic), "%s%s", base_mnemonic, get_size_suffix(op_size));
    
    return create_x86_instruction(mnemonic, operands, 1, context);
  } else {
    /* Binary operation */
    native_operand_t operands[2];
    
    /* Map the destination operand */
    error_t dst_result = translator_map_register(
      instruction->destination,
      context,
      &operands[0]
    );
    
    if (dst_result != ERROR_NONE) {
      return dst_result;
    }
    
    /* Map the source operand */
    error_t src_result = translator_map_operand(
      &instruction->operands[1],
      context,
      &operands[1]
    );
    
    if (src_result != ERROR_NONE) {
      return src_result;
    }
    
    /* Create instruction */
    char mnemonic[MAX_MNEMONIC_LENGTH];
    snprintf(mnemonic, sizeof(mnemonic), "%s%s", base_mnemonic, get_size_suffix(op_size));
    
    return create_x86_instruction(mnemonic, operands, 2, context);
  }
}

error_t x86_translate_logical(
  const coil_instruction_t* instruction,
  translator_context_t* context
) {
  if (instruction == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Map COIL logical instructions to x86 instructions */
  const char* base_mnemonic = NULL;
  
  /* Assuming logical opcodes start at 0x01 */
  switch (instruction->opcode) {
    case 0x01:  /* Assuming NOT */
      base_mnemonic = "not";
      break;
    case 0x02:  /* Assuming AND */
      base_mnemonic = "and";
      break;
    case 0x03:  /* Assuming OR */
      base_mnemonic = "or";
      break;
    case 0x04:  /* Assuming XOR */
      base_mnemonic = "xor";
      break;
    case 0x05:  /* Assuming SHL */
      base_mnemonic = "shl";
      break;
    case 0x06:  /* Assuming SHR */
      base_mnemonic = "shr";
      break;
    case 0x07:  /* Assuming SAR (arithmetic right shift) */
      base_mnemonic = "sar";
      break;
    default:
      log_error("Unknown logical operation: 0x%02X", instruction->opcode);
      return ERROR_UNSUPPORTED;
  }
  
  /* Determine operand size */
  uint8_t op_size = 4;  /* Default to 4 bytes (32-bit) */
  
  switch (instruction->data_type) {
    case 1:  /* Assuming 1 is 8-bit integer */
      op_size = 1;
      break;
    case 2:  /* Assuming 2 is 16-bit integer */
      op_size = 2;
      break;
    case 3:  /* Assuming 3 is 32-bit integer */
      op_size = 4;
      break;
    case 4:  /* Assuming 4 is 64-bit integer */
      op_size = 8;
      break;
    default:
      break;
  }
  
  /* Handle different instruction types */
  if (instruction->opcode == 0x01) {  /* NOT is unary */
    native_operand_t operands[1];
    
    /* Map the destination operand */
    error_t dst_result = translator_map_register(
      instruction->destination,
      context,
      &operands[0]
    );
    
    if (dst_result != ERROR_NONE) {
      return dst_result;
    }
    
    /* Create not instruction */
    char mnemonic[MAX_MNEMONIC_LENGTH];
    snprintf(mnemonic, sizeof(mnemonic), "%s%s", base_mnemonic, get_size_suffix(op_size));
    
    return create_x86_instruction(mnemonic, operands, 1, context);
  } else if (instruction->opcode >= 0x05) {  /* Shift operations */
    native_operand_t operands[2];
    
    /* Map the destination operand */
    error_t dst_result = translator_map_register(
      instruction->destination,
      context,
      &operands[0]
    );
    
    if (dst_result != ERROR_NONE) {
      return dst_result;
    }
    
    /* For shift operations, the shift count can be either in CL or an immediate */
    if (instruction->operands[1].type == COIL_OPERAND_IMMEDIATE) {
      /* Create immediate operand */
      x86_create_immediate_operand(
        instruction->operands[1].value.imm_value & 0xFF,  /* Mask to 8 bits */
        1,  /* Shift count is always 8-bit */
        &operands[1]
      );
    } else {
      /* Map to CL register */
      x86_register_info_t* cl_reg = find_register_by_name("cl");
      if (cl_reg == NULL) {
        log_error("Failed to find CL register for shift count");
        return ERROR_TRANSLATION;
      }
      
      x86_create_register_operand(cl_reg, &operands[1]);
    }
    
    /* Create shift instruction */
    char mnemonic[MAX_MNEMONIC_LENGTH];
    snprintf(mnemonic, sizeof(mnemonic), "%s%s", base_mnemonic, get_size_suffix(op_size));
    
    return create_x86_instruction(mnemonic, operands, 2, context);
  } else {  /* Binary operations (AND, OR, XOR) */
    native_operand_t operands[2];
    
    /* Map the destination operand */
    error_t dst_result = translator_map_register(
      instruction->destination,
      context,
      &operands[0]
    );
    
    if (dst_result != ERROR_NONE) {
      return dst_result;
    }
    
    /* Map the source operand */
    error_t src_result = translator_map_operand(
      &instruction->operands[1],
      context,
      &operands[1]
    );
    
    if (src_result != ERROR_NONE) {
      return src_result;
    }
    
    /* Create instruction */
    char mnemonic[MAX_MNEMONIC_LENGTH];
    snprintf(mnemonic, sizeof(mnemonic), "%s%s", base_mnemonic, get_size_suffix(op_size));
    
    return create_x86_instruction(mnemonic, operands, 2, context);
  }
}

error_t x86_translate_memory(
  const coil_instruction_t* instruction,
  translator_context_t* context
) {
  if (instruction == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Map COIL memory instructions to x86 instructions */
  const char* base_mnemonic = NULL;
  bool is_load = false;
  
  /* Assuming memory opcodes start at 0x01 */
  switch (instruction->opcode) {
    case 0x01:  /* Assuming LOAD */
      base_mnemonic = "mov";
      is_load = true;
      break;
    case 0x02:  /* Assuming STORE */
      base_mnemonic = "mov";
      is_load = false;
      break;
    case 0x03:  /* Assuming LOAD_ATOMIC */
      /* x86 doesn't have a simple atomic load, may need LOCK prefix */
      log_error("Atomic load needs special handling");
      return ERROR_UNSUPPORTED;
    case 0x04:  /* Assuming STORE_ATOMIC */
      /* Use LOCK prefix with XCHG for atomic store */
      log_error("Atomic store needs special handling");
      return ERROR_UNSUPPORTED;
    default:
      log_error("Unknown memory operation: 0x%02X", instruction->opcode);
      return ERROR_UNSUPPORTED;
  }
  
  /* Determine operand size */
  uint8_t op_size = 4;  /* Default to 4 bytes (32-bit) */
  
  switch (instruction->data_type) {
    case 1:  /* Assuming 1 is 8-bit integer */
      op_size = 1;
      break;
    case 2:  /* Assuming 2 is 16-bit integer */
      op_size = 2;
      break;
    case 3:  /* Assuming 3 is 32-bit integer */
      op_size = 4;
      break;
    case 4:  /* Assuming 4 is 64-bit integer */
      op_size = 8;
      break;
    case 5:  /* Assuming 5 is 32-bit float */
      op_size = 4;
      base_mnemonic = "movss";  /* Use SSE instructions for floats */
      break;
    case 6:  /* Assuming 6 is 64-bit float */
      op_size = 8;
      base_mnemonic = "movsd";  /* Use SSE instructions for doubles */
      break;
    default:
      break;
  }
  
  native_operand_t operands[2];
  
  if (is_load) {
    /* Load operation: register = memory */
    
    /* Map the destination register */
    error_t dst_result = translator_map_register(
      instruction->destination,
      context,
      &operands[0]
    );
    
    if (dst_result != ERROR_NONE) {
      return dst_result;
    }
    
    /* Map the source memory operand */
    error_t src_result = translator_map_operand(
      &instruction->operands[0],
      context,
      &operands[1]
    );
    
    if (src_result != ERROR_NONE) {
      return src_result;
    }
    
    /* Ensure the operand is a memory reference */
    if (operands[1].type != NATIVE_OPERAND_MEMORY) {
      log_error("Expected memory operand for load instruction");
      return ERROR_TRANSLATION;
    }
    
    /* Set memory operand size */
    operands[1].size = op_size;
  } else {
    /* Store operation: memory = register */
    
    /* Map the destination memory operand */
    error_t dst_result = translator_map_operand(
      &instruction->operands[0],
      context,
      &operands[0]
    );
    
    if (dst_result != ERROR_NONE) {
      return dst_result;
    }
    
    /* Ensure the operand is a memory reference */
    if (operands[0].type != NATIVE_OPERAND_MEMORY) {
      log_error("Expected memory operand for store instruction");
      return ERROR_TRANSLATION;
    }
    
    /* Set memory operand size */
    operands[0].size = op_size;
    
    /* Map the source register */
    error_t src_result = translator_map_register(
      instruction->operands[1].value.reg_id,
      context,
      &operands[1]
    );
    
    if (src_result != ERROR_NONE) {
      return src_result;
    }
  }
  
  /* Create instruction */
  char mnemonic[MAX_MNEMONIC_LENGTH];
  
  /* Only add suffix for basic mov */
  if (strcmp(base_mnemonic, "mov") == 0) {
    snprintf(mnemonic, sizeof(mnemonic), "%s%s", base_mnemonic, get_size_suffix(op_size));
  } else {
    strncpy(mnemonic, base_mnemonic, sizeof(mnemonic) - 1);
  }
  
  return create_x86_instruction(mnemonic, operands, 2, context);
}

error_t x86_translate_control(
  const coil_instruction_t* instruction,
  translator_context_t* context
) {
  if (instruction == NULL || context == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Map COIL control flow instructions to x86 instructions */
  const char* mnemonic = NULL;
  uint8_t operand_count = 0;
  
  /* Assuming control flow opcodes start at 0x01 */
  switch (instruction->opcode) {
    case 0x01:  /* Assuming JUMP */
      mnemonic = "jmp";
      operand_count = 1;
      break;
    case 0x02:  /* Assuming BRANCH */
      /* There are many branch types in x86, we'd need to determine which one */
      /* For simplicity, assume this is a jump-if-equal (je) */
      mnemonic = "je";
      operand_count = 1;
      break;
    case 0x03:  /* Assuming CALL */
      mnemonic = "call";
      operand_count = 1;
      break;
    case 0x04:  /* Assuming RETURN */
      mnemonic = "ret";
      operand_count = 0;
      break;
    default:
      log_error("Unknown control flow operation: 0x%02X", instruction->opcode);
      return ERROR_UNSUPPORTED;
  }
  
  /* Check if we need to handle operands */
  if (operand_count > 0) {
    native_operand_t operands[1];
    
    /* Map operand */
    error_t operand_result = translator_map_operand(
      &instruction->operands[0],
      context,
      &operands[0]
    );
    
    if (operand_result != ERROR_NONE) {
      return operand_result;
    }
    
    /* Create instruction */
    return create_x86_instruction(mnemonic, operands, operand_count, context);
  } else {
    /* No operands (e.g., ret) */
    return create_x86_instruction(mnemonic, NULL, 0, context);
  }
}

/**
 * @brief Registers all x86 instruction translators
 *
 * @return Error code indicating success or failure
 */
error_t x86_translator_register(void) {
  /* Register translators with the main translator module */
  /* In a full implementation, we would need to define and register
   * translation functions for each COIL instruction category. For now,
   * we'll just log the function call.
   */
  
  log_debug("x86_translator_register called");
  
  /* The following registrations would be done through appropriate functions
   * in the translator module. For now, they're just placeholders.
   */
  
  /* Example registrations:
  translator_register_category_translator(COIL_CAT_ARITHMETIC, x86_translate_arithmetic);
  translator_register_category_translator(COIL_CAT_LOGICAL, x86_translate_logical);
  translator_register_category_translator(COIL_CAT_MEMORY, x86_translate_memory);
  translator_register_category_translator(COIL_CAT_CONTROL, x86_translate_control);
  */
  
  return ERROR_NONE;
}

const x86_register_info_t* x86_translator_get_register_by_name(const char* reg_name) {
  return find_register_by_name(reg_name);
}

const x86_register_info_t* x86_translator_get_register_by_id(
  uint8_t reg_id, 
  x86_reg_class_t reg_class,
  x86_reg_size_t reg_size
) {
  return find_register_by_class_id(reg_class, reg_id, reg_size);
}