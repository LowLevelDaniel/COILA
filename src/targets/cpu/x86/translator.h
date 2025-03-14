/**
 * @file src/targets/cpu/x86/translator.h
 *
 * @brief x86 instruction translation
 *
 * Maps COIL instructions to x86 native instructions,
 * handles x86-specific requirements, implements feature
 * emulation when needed, and specializes code for
 * available hardware features.
 */

#ifndef X86_TRANSLATOR_H
#define X86_TRANSLATOR_H

#include "core/ir/instruction.h"
#include "core/config.h"

/**
 * @brief x86 translation context
 */
typedef struct {
  /** Target configuration */
  target_config_t* target;
  
  /** Register allocator */
  void* reg_alloc;
  
  /* Additional fields... */
} x86_context_t;

/**
 * @brief Translate a COIL instruction to x86 instructions
 *
 * @param instr The instruction to translate
 * @param ctx The x86 translation context
 * @return int Number of native instructions generated, or error code
 */
int translate_instruction_x86(instruction_t* instr, x86_context_t* ctx);

/* Additional functions... */

#endif /* X86_TRANSLATOR_H */