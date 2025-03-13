/**
 * @file x86_map.c
 * @brief X86-specific instruction mapping for COIL assembler.
 */

#include "coil_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Core instruction opcodes
 */
#define COIL_OP_ADD      0x01
#define COIL_OP_SUB      0x02
#define COIL_OP_MUL      0x03
#define COIL_OP_DIV      0x04
#define COIL_OP_REM      0x05
#define COIL_OP_NEG      0x06
#define COIL_OP_ABS      0x07
#define COIL_OP_MIN      0x08
#define COIL_OP_MAX      0x09
#define COIL_OP_FMA      0x0A

#define COIL_OP_AND      0x10
#define COIL_OP_OR       0x11
#define COIL_OP_XOR      0x12
#define COIL_OP_NOT      0x13
#define COIL_OP_SHL      0x14
#define COIL_OP_SHR      0x15

#define COIL_OP_CMP_EQ   0x20
#define COIL_OP_CMP_NE   0x21
#define COIL_OP_CMP_LT   0x22
#define COIL_OP_CMP_LE   0x23
#define COIL_OP_CMP_GT   0x24
#define COIL_OP_CMP_GE   0x25

#define COIL_OP_LOAD     0x30
#define COIL_OP_STORE    0x31
#define COIL_OP_LEA      0x32
#define COIL_OP_FENCE    0x33

#define COIL_OP_BR       0x40
#define COIL_OP_BR_COND  0x41
#define COIL_OP_SWITCH   0x42
#define COIL_OP_CALL     0x43
#define COIL_OP_RET      0x44

#define COIL_OP_CONVERT  0x50
#define COIL_OP_TRUNC    0x51
#define COIL_OP_EXTEND   0x52

#define COIL_OP_VADD     0x60
#define COIL_OP_VSUB     0x61
#define COIL_OP_VMUL     0x62
#define COIL_OP_VDIV     0x63
#define COIL_OP_VDOT     0x64
#define COIL_OP_VSPLAT   0x65
#define COIL_OP_VLOAD    0x66
#define COIL_OP_VSTORE   0x67

#define COIL_OP_ATOMIC_ADD  0x70
#define COIL_OP_ATOMIC_SUB  0x71
#define COIL_OP_ATOMIC_AND  0x72
#define COIL_OP_ATOMIC_OR   0x73
#define COIL_OP_ATOMIC_XOR  0x74
#define COIL_OP_ATOMIC_CAS  0x75

/**
 * @brief Simple buffer for generating x86 machine code
 */
typedef struct {
  uint8_t* data;
  size_t size;
  size_t capacity;
} code_buffer_t;

/**
 * @brief Initialize a code buffer
 */
static void init_code_buffer(code_buffer_t* buffer, uint8_t* data, size_t capacity) {
  buffer->data = data;
  buffer->size = 0;
  buffer->capacity = capacity;
}

/**
 * @brief Append a byte to the code buffer
 */
static void emit_byte(code_buffer_t* buffer, uint8_t byte) {
  if (buffer->size < buffer->capacity) {
    buffer->data[buffer->size++] = byte;
  }
}

/**
 * @brief Append a 32-bit value to the code buffer
 */
static void emit_dword(code_buffer_t* buffer, uint32_t value) {
  if (buffer->size + 4 <= buffer->capacity) {
    buffer->data[buffer->size++] = value & 0xFF;
    buffer->data[buffer->size++] = (value >> 8) & 0xFF;
    buffer->data[buffer->size++] = (value >> 16) & 0xFF;
    buffer->data[buffer->size++] = (value >> 24) & 0xFF;
  }
}

/**
 * @brief Append a 64-bit value to the code buffer
 */
static void emit_qword(code_buffer_t* buffer, uint64_t value) {
  if (buffer->size + 8 <= buffer->capacity) {
    buffer->data[buffer->size++] = value & 0xFF;
    buffer->data[buffer->size++] = (value >> 8) & 0xFF;
    buffer->data[buffer->size++] = (value >> 16) & 0xFF;
    buffer->data[buffer->size++] = (value >> 24) & 0xFF;
    buffer->data[buffer->size++] = (value >> 32) & 0xFF;
    buffer->data[buffer->size++] = (value >> 40) & 0xFF;
    buffer->data[buffer->size++] = (value >> 48) & 0xFF;
    buffer->data[buffer->size++] = (value >> 56) & 0xFF;
  }
}

/**
 * @brief X86-64 register encoding
 * 
 * For encoding in ModR/M and REX prefixes:
 * RAX: 0, RCX: 1, RDX: 2, RBX: 3, RSP: 4, RBP: 5, RSI: 6, RDI: 7
 * R8-R15: 8-15 (requires REX prefix)
 */
#define X86_RAX 0
#define X86_RCX 1
#define X86_RDX 2
#define X86_RBX 3
#define X86_RSP 4
#define X86_RBP 5
#define X86_RSI 6
#define X86_RDI 7
#define X86_R8  8
#define X86_R9  9
#define X86_R10 10
#define X86_R11 11
#define X86_R12 12
#define X86_R13 13
#define X86_R14 14
#define X86_R15 15

/**
 * @brief Emit a REX prefix for x86-64
 * 
 * @param buffer Code buffer
 * @param w 64-bit operand flag
 * @param r Extension of ModR/M reg field
 * @param x Extension of SIB index field
 * @param b Extension of ModR/M r/m field or SIB base field
 */
static void emit_rex(code_buffer_t* buffer, bool w, uint8_t r, uint8_t x, uint8_t b) {
  uint8_t rex = 0x40;
  
  if (w) rex |= 0x08;
  if (r > 7) rex |= 0x04;
  if (x > 7) rex |= 0x02;
  if (b > 7) rex |= 0x01;
  
  // Only emit REX if needed
  if (rex != 0x40 || w) {
    emit_byte(buffer, rex);
  }
}

/**
 * @brief Emit a ModR/M byte
 * 
 * @param buffer Code buffer
 * @param mod Mode (0-3)
 * @param reg Register (0-7)
 * @param rm Register/Memory (0-7)
 */
static void emit_modrm(code_buffer_t* buffer, uint8_t mod, uint8_t reg, uint8_t rm) {
  emit_byte(buffer, ((mod & 0x3) << 6) | ((reg & 0x7) << 3) | (rm & 0x7));
}

/**
 * @brief Emit a SIB byte
 * 
 * @param buffer Code buffer
 * @param scale Scale factor (0-3, representing 1,2,4,8)
 * @param index Index register (0-7)
 * @param base Base register (0-7)
 */
static void emit_sib(code_buffer_t* buffer, uint8_t scale, uint8_t index, uint8_t base) {
  emit_byte(buffer, ((scale & 0x3) << 6) | ((index & 0x7) << 3) | (base & 0x7));
}

/**
 * @brief Map from virtual register to physical register
 */
static uint8_t map_register(translation_context_t* ctx, uint32_t virtual_reg) {
  // Simple direct mapping for now - will be replaced with proper register allocation
  return (uint8_t)(virtual_reg % ctx->target->resources.registers);
}

/**
 * @brief Get operand size for a type
 */
static uint8_t get_operand_size(coil_type_encoding_t encoding) {
  uint8_t category = (encoding >> 28) & 0xF;
  uint8_t width = (encoding >> 20) & 0xFF;
  
  if (category == TYPE_CATEGORY_VOID) {
    return 0;
  } else if (category == TYPE_CATEGORY_BOOLEAN) {
    return 1;
  } else if (category == TYPE_CATEGORY_INTEGER || category == TYPE_CATEGORY_FLOAT) {
    return width / 8;
  } else if (category == TYPE_CATEGORY_POINTER) {
    return 8;  // Assume 64-bit pointers for x86-64
  } else {
    return 8;  // Default to 64-bit for unknown types
  }
}

/**
 * @brief Get integer size suffix for x86 mnemonic
 */
static char get_size_suffix(uint8_t size) {
  switch (size) {
    case 1: return 'b';  // byte
    case 2: return 'w';  // word
    case 4: return 'l';  // long
    case 8: return 'q';  // quad
    default: return 'q';  // default to quad
  }
}

/**
 * @brief Emit x86 opcode(s)
 */
static void emit_opcode(code_buffer_t* buffer, const uint8_t* opcodes, size_t opcode_count) {
  for (size_t i = 0; i < opcode_count; i++) {
    emit_byte(buffer, opcodes[i]);
  }
}

/**
 * @brief Generate code for X86 ADD instruction
 */
static void translate_add(coil_instruction_t* instr, void* context) {
  translation_context_t* ctx = (translation_context_t*)context;
  
  code_buffer_t buffer;
  init_code_buffer(&buffer, 
                   ((uint8_t*)ctx->native_code) + ctx->code_size, 
                   ctx->code_capacity - ctx->code_size);
  
  // Get operand size
  uint8_t size = get_operand_size(instr->type.encoding);
  
  // Get destination register
  uint8_t dst_reg = map_register(ctx, instr->dest);
  
  // Check operands
  if (instr->num_operands < 2) {
    // Invalid instruction format - should have at least 2 operands
    return;
  }
  
  // Handle different operand types
  if (instr->operands[0].type == OPERAND_REGISTER && 
      instr->operands[1].type == OPERAND_REGISTER) {
    // Register-register ADD
    uint8_t src1_reg = map_register(ctx, instr->operands[0].value.reg);
    uint8_t src2_reg = map_register(ctx, instr->operands[1].value.reg);
    
    // Different encodings based on operand size
    if (size == 8) {
      // 64-bit add: REX.W + 03 /r
      emit_rex(&buffer, true, dst_reg, 0, src2_reg);
      emit_byte(&buffer, 0x03);
      emit_modrm(&buffer, 3, dst_reg & 0x7, src2_reg & 0x7);
    } else if (size == 4) {
      // 32-bit add: 03 /r
      // May need REX prefix for extended registers
      emit_rex(&buffer, false, dst_reg, 0, src2_reg);
      emit_byte(&buffer, 0x03);
      emit_modrm(&buffer, 3, dst_reg & 0x7, src2_reg & 0x7);
    } else {
      // Smaller sizes have different encodings
      // For simplicity, not implemented in this example
    }
  } else if (instr->operands[0].type == OPERAND_REGISTER && 
             instr->operands[1].type == OPERAND_IMMEDIATE) {
    // Register-immediate ADD
    uint8_t dst_reg = map_register(ctx, instr->dest);
    uint8_t src_reg = map_register(ctx, instr->operands[0].value.reg);
    int64_t imm = instr->operands[1].value.imm;
    
    // Move source to destination first if different registers
    if (dst_reg != src_reg) {
      if (size == 8) {
        // 64-bit mov: REX.W + 89 /r
        emit_rex(&buffer, true, src_reg, 0, dst_reg);
        emit_byte(&buffer, 0x89);
        emit_modrm(&buffer, 3, src_reg & 0x7, dst_reg & 0x7);
      } else if (size == 4) {
        emit_rex(&buffer, false, src_reg, 0, dst_reg);
        emit_byte(&buffer, 0x89);
        emit_modrm(&buffer, 3, src_reg & 0x7, dst_reg & 0x7);
      }
    }
    
    // Add immediate to destination
    if (size == 8) {
      if (imm >= -128 && imm <= 127) {
        // 64-bit add with 8-bit immediate: REX.W + 83 /0 ib
        emit_rex(&buffer, true, 0, 0, dst_reg);
        emit_byte(&buffer, 0x83);
        emit_modrm(&buffer, 3, 0, dst_reg & 0x7);
        emit_byte(&buffer, imm & 0xFF);
      } else if (imm >= -2147483648LL && imm <= 2147483647LL) {
        // 64-bit add with 32-bit immediate: REX.W + 81 /0 id
        emit_rex(&buffer, true, 0, 0, dst_reg);
        emit_byte(&buffer, 0x81);
        emit_modrm(&buffer, 3, 0, dst_reg & 0x7);
        emit_dword(&buffer, imm);
      }
    } else if (size == 4) {
      if (imm >= -128 && imm <= 127) {
        // 32-bit add with 8-bit immediate: 83 /0 ib
        emit_rex(&buffer, false, 0, 0, dst_reg);
        emit_byte(&buffer, 0x83);
        emit_modrm(&buffer, 3, 0, dst_reg & 0x7);
        emit_byte(&buffer, imm & 0xFF);
      } else {
        // 32-bit add with 32-bit immediate: 81 /0 id
        emit_rex(&buffer, false, 0, 0, dst_reg);
        emit_byte(&buffer, 0x81);
        emit_modrm(&buffer, 3, 0, dst_reg & 0x7);
        emit_dword(&buffer, imm);
      }
    }
  }
  
  // Update code size
  ctx->code_size += buffer.size;
}

/**
 * @brief Generate code for X86 SUB instruction
 */
static void translate_sub(coil_instruction_t* instr, void* context) {
  translation_context_t* ctx = (translation_context_t*)context;
  
  code_buffer_t buffer;
  init_code_buffer(&buffer, 
                   ((uint8_t*)ctx->native_code) + ctx->code_size, 
                   ctx->code_capacity - ctx->code_size);
  
  // Get operand size
  uint8_t size = get_operand_size(instr->type.encoding);
  
  // Get destination register
  uint8_t dst_reg = map_register(ctx, instr->dest);
  
  // Check operands
  if (instr->num_operands < 2) {
    // Invalid instruction format - should have at least 2 operands
    return;
  }
  
  // Handle different operand types
  if (instr->operands[0].type == OPERAND_REGISTER && 
      instr->operands[1].type == OPERAND_REGISTER) {
    // Register-register SUB
    uint8_t src1_reg = map_register(ctx, instr->operands[0].value.reg);
    uint8_t src2_reg = map_register(ctx, instr->operands[1].value.reg);
    
    // Move source1 to destination first if different
    if (dst_reg != src1_reg) {
      if (size == 8) {
        emit_rex(&buffer, true, src1_reg, 0, dst_reg);
        emit_byte(&buffer, 0x89);
        emit_modrm(&buffer, 3, src1_reg & 0x7, dst_reg & 0x7);
      } else if (size == 4) {
        emit_rex(&buffer, false, src1_reg, 0, dst_reg);
        emit_byte(&buffer, 0x89);
        emit_modrm(&buffer, 3, src1_reg & 0x7, dst_reg & 0x7);
      }
    }
    
    // Subtract source2 from destination
    if (size == 8) {
      // 64-bit sub: REX.W + 2B /r
      emit_rex(&buffer, true, dst_reg, 0, src2_reg);
      emit_byte(&buffer, 0x2B);
      emit_modrm(&buffer, 3, dst_reg & 0x7, src2_reg & 0x7);
    } else if (size == 4) {
      // 32-bit sub: 2B /r
      emit_rex(&buffer, false, dst_reg, 0, src2_reg);
      emit_byte(&buffer, 0x2B);
      emit_modrm(&buffer, 3, dst_reg & 0x7, src2_reg & 0x7);
    }
  } else if (instr->operands[0].type == OPERAND_REGISTER && 
             instr->operands[1].type == OPERAND_IMMEDIATE) {
    // Register-immediate SUB
    uint8_t src_reg = map_register(ctx, instr->operands[0].value.reg);
    int64_t imm = instr->operands[1].value.imm;
    
    // Move source to destination first if different
    if (dst_reg != src_reg) {
      if (size == 8) {
        emit_rex(&buffer, true, src_reg, 0, dst_reg);
        emit_byte(&buffer, 0x89);
        emit_modrm(&buffer, 3, src_reg & 0x7, dst_reg & 0x7);
      } else if (size == 4) {
        emit_rex(&buffer, false, src_reg, 0, dst_reg);
        emit_byte(&buffer, 0x89);
        emit_modrm(&buffer, 3, src_reg & 0x7, dst_reg & 0x7);
      }
    }
    
    // Subtract immediate from destination
    if (size == 8) {
      if (imm >= -128 && imm <= 127) {
        // 64-bit sub with 8-bit immediate: REX.W + 83 /5 ib
        emit_rex(&buffer, true, 0, 0, dst_reg);
        emit_byte(&buffer, 0x83);
        emit_modrm(&buffer, 3, 5, dst_reg & 0x7);
        emit_byte(&buffer, imm & 0xFF);
      } else if (imm >= -2147483648LL && imm <= 2147483647LL) {
        // 64-bit sub with 32-bit immediate: REX.W + 81 /5 id
        emit_rex(&buffer, true, 0, 0, dst_reg);
        emit_byte(&buffer, 0x81);
        emit_modrm(&buffer, 3, 5, dst_reg & 0x7);
        emit_dword(&buffer, imm);
      }
    } else if (size == 4) {
      if (imm >= -128 && imm <= 127) {
        // 32-bit sub with 8-bit immediate: 83 /5 ib
        emit_rex(&buffer, false, 0, 0, dst_reg);
        emit_byte(&buffer, 0x83);
        emit_modrm(&buffer, 3, 5, dst_reg & 0x7);
        emit_byte(&buffer, imm & 0xFF);
      } else {
        // 32-bit sub with 32-bit immediate: 81 /5 id
        emit_rex(&buffer, false, 0, 0, dst_reg);
        emit_byte(&buffer, 0x81);
        emit_modrm(&buffer, 3, 5, dst_reg & 0x7);
        emit_dword(&buffer, imm);
      }
    }
  }
  
  // Update code size
  ctx->code_size += buffer.size;
}

/**
 * @brief Generate code for X86 MUL instruction
 */
static void translate_mul(coil_instruction_t* instr, void* context) {
  translation_context_t* ctx = (translation_context_t*)context;
  
  code_buffer_t buffer;
  init_code_buffer(&buffer, 
                   ((uint8_t*)ctx->native_code) + ctx->code_size, 
                   ctx->code_capacity - ctx->code_size);
  
  // Get operand size
  uint8_t size = get_operand_size(instr->type.encoding);
  
  // Get destination register
  uint8_t dst_reg = map_register(ctx, instr->dest);
  
  // Check operands
  if (instr->num_operands < 2) {
    // Invalid instruction format - should have at least 2 operands
    return;
  }
  
  // Handle different operand types
  if (instr->operands[0].type == OPERAND_REGISTER && 
      instr->operands[1].type == OPERAND_REGISTER) {
    // Register-register MUL
    uint8_t src1_reg = map_register(ctx, instr->operands[0].value.reg);
    uint8_t src2_reg = map_register(ctx, instr->operands[1].value.reg);
    
    // Move source1 to destination first if different
    if (dst_reg != src1_reg) {
      if (size == 8) {
        emit_rex(&buffer, true, src1_reg, 0, dst_reg);
        emit_byte(&buffer, 0x89);
        emit_modrm(&buffer, 3, src1_reg & 0x7, dst_reg & 0x7);
      } else if (size == 4) {
        emit_rex(&buffer, false, src1_reg, 0, dst_reg);
        emit_byte(&buffer, 0x89);
        emit_modrm(&buffer, 3, src1_reg & 0x7, dst_reg & 0x7);
      }
    }
    
    // Multiply destination by source2
    // IMUL r64, r/m64: REX.W + 0F AF /r
    if (size == 8) {
      emit_rex(&buffer, true, dst_reg, 0, src2_reg);
      emit_byte(&buffer, 0x0F);
      emit_byte(&buffer, 0xAF);
      emit_modrm(&buffer, 3, dst_reg & 0x7, src2_reg & 0x7);
    } else if (size == 4) {
      emit_rex(&buffer, false, dst_reg, 0, src2_reg);
      emit_byte(&buffer, 0x0F);
      emit_byte(&buffer, 0xAF);
      emit_modrm(&buffer, 3, dst_reg & 0x7, src2_reg & 0x7);
    }
  } else if (instr->operands[0].type == OPERAND_REGISTER && 
             instr->operands[1].type == OPERAND_IMMEDIATE) {
    // Register-immediate MUL
    uint8_t src_reg = map_register(ctx, instr->operands[0].value.reg);
    int64_t imm = instr->operands[1].value.imm;
    
    // IMUL r64, r/m64, imm32: REX.W + 69 /r id
    if (size == 8) {
      emit_rex(&buffer, true, dst_reg, 0, src_reg);
      emit_byte(&buffer, 0x69);
      emit_modrm(&buffer, 3, dst_reg & 0x7, src_reg & 0x7);
      emit_dword(&buffer, imm);
    } else if (size == 4) {
      emit_rex(&buffer, false, dst_reg, 0, src_reg);
      emit_byte(&buffer, 0x69);
      emit_modrm(&buffer, 3, dst_reg & 0x7, src_reg & 0x7);
      emit_dword(&buffer, imm);
    }
  }
  
  // Update code size
  ctx->code_size += buffer.size;
}

/**
 * @brief Generate code for X86 LOAD instruction
 */
static void translate_load(coil_instruction_t* instr, void* context) {
  translation_context_t* ctx = (translation_context_t*)context;
  
  code_buffer_t buffer;
  init_code_buffer(&buffer, 
                   ((uint8_t*)ctx->native_code) + ctx->code_size, 
                   ctx->code_capacity - ctx->code_size);
  
  // Get operand size
  uint8_t size = get_operand_size(instr->type.encoding);
  
  // Get destination register
  uint8_t dst_reg = map_register(ctx, instr->dest);
  
  // Check operands
  if (instr->num_operands < 1) {
    // Invalid instruction format - should have at least 1 operand
    return;
  }
  
  // Handle different address operand types
  if (instr->operands[0].type == OPERAND_REGISTER) {
    // Simple load from memory pointed to by register
    uint8_t addr_reg = map_register(ctx, instr->operands[0].value.reg);
    
    if (size == 8) {
      // MOV r64, r/m64: REX.W + 8B /r
      emit_rex(&buffer, true, dst_reg, 0, addr_reg);
      emit_byte(&buffer, 0x8B);
      emit_modrm(&buffer, 0, dst_reg & 0x7, addr_reg & 0x7);
    } else if (size == 4) {
      // MOV r32, r/m32: 8B /r
      emit_rex(&buffer, false, dst_reg, 0, addr_reg);
      emit_byte(&buffer, 0x8B);
      emit_modrm(&buffer, 0, dst_reg & 0x7, addr_reg & 0x7);
    } else if (size == 2) {
      // MOV r16, r/m16: 66 8B /r
      emit_byte(&buffer, 0x66); // 16-bit operand size prefix
      emit_rex(&buffer, false, dst_reg, 0, addr_reg);
      emit_byte(&buffer, 0x8B);
      emit_modrm(&buffer, 0, dst_reg & 0x7, addr_reg & 0x7);
    } else if (size == 1) {
      // MOV r8, r/m8: 8A /r
      emit_rex(&buffer, false, dst_reg, 0, addr_reg);
      emit_byte(&buffer, 0x8A);
      emit_modrm(&buffer, 0, dst_reg & 0x7, addr_reg & 0x7);
    }
  } else if (instr->operands[0].type == OPERAND_MEMORY) {
    // Load from complex memory address (base + index*scale + disp)
    uint8_t base_reg = map_register(ctx, instr->operands[0].value.mem.base);
    uint8_t index_reg = map_register(ctx, instr->operands[0].value.mem.index);
    uint32_t scale = instr->operands[0].value.mem.scale;
    int64_t disp = instr->operands[0].value.mem.disp;
    
    // Convert scale to SIB encoding
    uint8_t scale_encoding = 0;
    switch (scale) {
      case 1: scale_encoding = 0; break;
      case 2: scale_encoding = 1; break;
      case 4: scale_encoding = 2; break;
      case 8: scale_encoding = 3; break;
      default: scale_encoding = 0; break;
    }
    
    if (size == 8) {
      // MOV r64, r/m64: REX.W + 8B /r
      emit_rex(&buffer, true, dst_reg, index_reg, base_reg);
      emit_byte(&buffer, 0x8B);
      
      if (disp == 0) {
        // No displacement, just base + index*scale
        emit_modrm(&buffer, 0, dst_reg & 0x7, 4); // 4 indicates SIB follows
        emit_sib(&buffer, scale_encoding, index_reg & 0x7, base_reg & 0x7);
      } else if (disp >= -128 && disp <= 127) {
        // 8-bit displacement
        emit_modrm(&buffer, 1, dst_reg & 0x7, 4); // Mod 01, SIB follows
        emit_sib(&buffer, scale_encoding, index_reg & 0x7, base_reg & 0x7);
        emit_byte(&buffer, disp & 0xFF);
      } else {
        // 32-bit displacement
        emit_modrm(&buffer, 2, dst_reg & 0x7, 4); // Mod 10, SIB follows
        emit_sib(&buffer, scale_encoding, index_reg & 0x7, base_reg & 0x7);
        emit_dword(&buffer, disp);
      }
    } else {
      // Similar patterns for other sizes
      // Simplified for now
    }
  }
  
  // Update code size
  ctx->code_size += buffer.size;
}

/**
 * @brief Generate code for X86 STORE instruction
 */
static void translate_store(coil_instruction_t* instr, void* context) {
  translation_context_t* ctx = (translation_context_t*)context;
  
  code_buffer_t buffer;
  init_code_buffer(&buffer, 
                   ((uint8_t*)ctx->native_code) + ctx->code_size, 
                   ctx->code_capacity - ctx->code_size);
  
  // Get operand size
  uint8_t size = get_operand_size(instr->type.encoding);
  
  // Check operands
  if (instr->num_operands < 2) {
    // Invalid instruction format - should have at least 2 operands
    return;
  }
  
  // Get source register
  uint8_t src_reg = map_register(ctx, instr->operands[1].value.reg);
  
  // Handle different address operand types
  if (instr->operands[0].type == OPERAND_REGISTER) {
    // Simple store to memory pointed to by register
    uint8_t addr_reg = map_register(ctx, instr->operands[0].value.reg);
    
    if (size == 8) {
      // MOV r/m64, r64: REX.W + 89 /r
      emit_rex(&buffer, true, src_reg, 0, addr_reg);
      emit_byte(&buffer, 0x89);
      emit_modrm(&buffer, 0, src_reg & 0x7, addr_reg & 0x7);
    } else if (size == 4) {
      // MOV r/m32, r32: 89 /r
      emit_rex(&buffer, false, src_reg, 0, addr_reg);
      emit_byte(&buffer, 0x89);
      emit_modrm(&buffer, 0, src_reg & 0x7, addr_reg & 0x7);
    } else if (size == 2) {
      // MOV r/m16, r16: 66 89 /r
      emit_byte(&buffer, 0x66); // 16-bit operand size prefix
      emit_rex(&buffer, false, src_reg, 0, addr_reg);
      emit_byte(&buffer, 0x89);
      emit_modrm(&buffer, 0, src_reg & 0x7, addr_reg & 0x7);
    } else if (size == 1) {
      // MOV r/m8, r8: 88 /r
      emit_rex(&buffer, false, src_reg, 0, addr_reg);
      emit_byte(&buffer, 0x88);
      emit_modrm(&buffer, 0, src_reg & 0x7, addr_reg & 0x7);
    }
  } else if (instr->operands[0].type == OPERAND_MEMORY) {
    // Store to complex memory address (base + index*scale + disp)
    uint8_t base_reg = map_register(ctx, instr->operands[0].value.mem.base);
    uint8_t index_reg = map_register(ctx, instr->operands[0].value.mem.index);
    uint32_t scale = instr->operands[0].value.mem.scale;
    int64_t disp = instr->operands[0].value.mem.disp;
    
    // Convert scale to SIB encoding
    uint8_t scale_encoding = 0;
    switch (scale) {
      case 1: scale_encoding = 0; break;
      case 2: scale_encoding = 1; break;
      case 4: scale_encoding = 2; break;
      case 8: scale_encoding = 3; break;
      default: scale_encoding = 0; break;
    }
    
    if (size == 8) {
      // MOV r/m64, r64: REX.W + 89 /r
      emit_rex(&buffer, true, src_reg, index_reg, base_reg);
      emit_byte(&buffer, 0x89);
      
      if (disp == 0) {
        // No displacement, just base + index*scale
        emit_modrm(&buffer, 0, src_reg & 0x7, 4); // 4 indicates SIB follows
        emit_sib(&buffer, scale_encoding, index_reg & 0x7, base_reg & 0x7);
      } else if (disp >= -128 && disp <= 127) {
        // 8-bit displacement
        emit_modrm(&buffer, 1, src_reg & 0x7, 4); // Mod 01, SIB follows
        emit_sib(&buffer, scale_encoding, index_reg & 0x7, base_reg & 0x7);
        emit_byte(&buffer, disp & 0xFF);
      } else {
        // 32-bit displacement
        emit_modrm(&buffer, 2, src_reg & 0x7, 4); // Mod 10, SIB follows
        emit_sib(&buffer, scale_encoding, index_reg & 0x7, base_reg & 0x7);
        emit_dword(&buffer, disp);
      }
    } else {
      // Similar patterns for other sizes
      // Simplified for now
    }
  }
  
  // Update code size
  ctx->code_size += buffer.size;
}

/**
 * @brief Initialize X86 instruction mappings for virtual map
 */
virtual_map_t* initialize_x86_map(coil_device_class_t device_class) {
  // Only support CPU device class for x86
  if (device_class != COIL_DEVICE_CPU) {
    return NULL;
  }
  
  // Create x86 virtual map
  virtual_map_t* map = create_virtual_map("x86_64", device_class);
  if (!map) {
    return NULL;
  }
  
  // Add instruction mappings
  add_instruction_mapping(map, COIL_OP_ADD, "add", translate_add);
  add_instruction_mapping(map, COIL_OP_SUB, "sub", translate_sub);
  add_instruction_mapping(map, COIL_OP_MUL, "imul", translate_mul);
  add_instruction_mapping(map, COIL_OP_LOAD, "mov", translate_load);
  add_instruction_mapping(map, COIL_OP_STORE, "mov", translate_store);
  
  // Additional mappings would be added here
  
  return map;
}