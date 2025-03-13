/**
 * @file x86_generator.c
 * @brief Implementation of x86-specific native code generation
 * 
 * This module implements x86-specific code for generating native binaries
 * from translated instructions, supporting 16-bit, 32-bit, and 64-bit modes.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "x86_generator.h"
#include "x86_translator.h"
#include "../../utils/memory_management.h"
#include "../../utils/logging.h"

/**
 * @brief x86 ELF file format header sizes
 */
#define X86_ELF_HEADER_SIZE32 52
#define X86_ELF_HEADER_SIZE64 64
#define X86_ELF_PROGRAM_HEADER_SIZE32 32
#define X86_ELF_PROGRAM_HEADER_SIZE64 56
#define X86_ELF_SECTION_HEADER_SIZE32 40
#define X86_ELF_SECTION_HEADER_SIZE64 64

/**
 * @brief x86 ELF machine types
 */
#define X86_ELF_MACHINE_386 3
#define X86_ELF_MACHINE_X86_64 62

/**
 * @brief ELF header magic bytes
 */
static const uint8_t ELF_MAGIC[4] = { 0x7F, 'E', 'L', 'F' };

/**
 * @brief ModR/M byte components
 */
typedef struct {
  uint8_t rm : 3;   /**< R/M field */
  uint8_t reg : 3;  /**< REG field */
  uint8_t mod : 2;  /**< MOD field */
} modrm_byte_t;

/**
 * @brief SIB byte components
 */
typedef struct {
  uint8_t base : 3;  /**< Base field */
  uint8_t index : 3; /**< Index field */
  uint8_t scale : 2; /**< Scale field */
} sib_byte_t;

/**
 * @brief Initializes x86-specific generator
 *
 * @param[in] generator Native generator
 * @param[in] target Target configuration
 * @return Error code indicating success or failure
 */
error_t x86_generator_init(
  native_generator_t* generator,
  const target_config_t* target
) {
  if (generator == NULL || target == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Allocate x86-specific data */
  x86_generator_data_t* data = memory_calloc(1, sizeof(x86_generator_data_t));
  if (data == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Determine architecture features */
  bool has_x86_64 = target_config_has_feature(target, "x86_64");
  bool has_i386 = target_config_has_feature(target, "i386");
  
  data->is_64bit = has_x86_64;
  data->is_32bit = has_i386 && !has_x86_64;
  data->is_16bit = !has_x86_64 && !has_i386;
  data->target_specific = NULL;
  
  /* Store in generator */
  error_t set_result = native_generator_set_arch_data(generator, data);
  if (set_result != ERROR_NONE) {
    memory_free(data);
    return set_result;
  }
  
  /* Add default sections */
  error_t sections_result = x86_add_sections(generator);
  if (sections_result != ERROR_NONE) {
    memory_free(data);
    return sections_result;
  }
  
  log_info("Initialized x86 generator in %s-bit mode",
          data->is_64bit ? "64" : data->is_32bit ? "32" : "16");
  
  return ERROR_NONE;
}

/**
 * @brief Cleans up x86-specific generator resources
 *
 * @param[in] generator Native generator
 * @return Error code indicating success or failure
 */
error_t x86_generator_cleanup(native_generator_t* generator) {
  if (generator == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  x86_generator_data_t* data = x86_generator_get_data(generator);
  if (data == NULL) {
    return ERROR_NONE;  /* Nothing to clean up */
  }
  
  /* Free any target-specific data */
  if (data->target_specific != NULL) {
    memory_free(data->target_specific);
  }
  
  /* Free generator data */
  memory_free(data);
  
  return ERROR_NONE;
}

/**
 * @brief Gets x86-specific generator data
 *
 * @param[in] generator Native generator
 * @return x86-specific generator data, or NULL on error
 */
x86_generator_data_t* x86_generator_get_data(native_generator_t* generator) {
  if (generator == NULL) {
    return NULL;
  }
  
  return (x86_generator_data_t*)native_generator_get_arch_data(generator);
}

/**
 * @brief Creates a ModR/M byte
 *
 * @param[in] mod MOD field value
 * @param[in] reg REG field value
 * @param[in] rm R/M field value
 * @return Packed ModR/M byte
 */
static uint8_t create_modrm(uint8_t mod, uint8_t reg, uint8_t rm) {
  return ((mod & 0x3) << 6) | ((reg & 0x7) << 3) | (rm & 0x7);
}

/**
 * @brief Creates a SIB byte
 *
 * @param[in] scale Scale field value
 * @param[in] index Index field value
 * @param[in] base Base field value
 * @return Packed SIB byte
 */
static uint8_t create_sib(uint8_t scale, uint8_t index, uint8_t base) {
  /* Convert scale to 2-bit value (0=1, 1=2, 2=4, 3=8) */
  uint8_t scale_bits = 0;
  
  switch (scale) {
    case 1:
      scale_bits = 0;
      break;
    case 2:
      scale_bits = 1;
      break;
    case 4:
      scale_bits = 2;
      break;
    case 8:
      scale_bits = 3;
      break;
    default:
      scale_bits = 0;
      break;
  }
  
  return ((scale_bits & 0x3) << 6) | ((index & 0x7) << 3) | (base & 0x7);
}

/**
 * @brief Encodes register operand in ModR/M format
 *
 * @param[in] operand Register operand
 * @param[out] encoding Encoding information
 * @param[in] is_rm Whether the register is in the R/M field
 * @return Error code indicating success or failure
 */
static error_t encode_register(
  const native_operand_t* operand,
  x86_encoding_t* encoding,
  bool is_rm
) {
  if (operand == NULL || encoding == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  if (operand->type != NATIVE_OPERAND_REGISTER) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Get register encoding value */
  uint8_t reg_code = operand->value.reg.id & 0x7;
  
  /* Handle extended registers (R8-R15) */
  if (operand->value.reg.id >= 8) {
    /* Need REX prefix with B or R bit */
    uint8_t rex = 0x40 | (is_rm ? 0x01 : 0x04);
    
    /* Add to prefixes if not already present */
    bool found = false;
    for (uint8_t i = 0; i < encoding->prefix_count; i++) {
      if ((encoding->prefixes[i] & 0xF0) == 0x40) {
        encoding->prefixes[i] |= (is_rm ? 0x01 : 0x04);
        found = true;
        break;
      }
    }
    
    if (!found && encoding->prefix_count < 4) {
      encoding->prefixes[encoding->prefix_count++] = rex;
    }
  }
  
  /* Update ModR/M byte */
  if (is_rm) {
    encoding->modrm = create_modrm(3, encoding->modrm >> 3, reg_code);
  } else {
    encoding->modrm = create_modrm(encoding->modrm >> 6, reg_code, encoding->modrm & 0x7);
  }
  
  encoding->has_modrm = 1;
  
  return ERROR_NONE;
}

/**
 * @brief Encodes memory operand in ModR/M format
 *
 * @param[in] operand Memory operand
 * @param[out] encoding Encoding information
 * @param[in] gen_data Generator data
 * @return Error code indicating success or failure
 */
static error_t encode_memory(
  const native_operand_t* operand,
  x86_encoding_t* encoding,
  const x86_generator_data_t* gen_data
) {
  if (operand == NULL || encoding == NULL || gen_data == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  if (operand->type != NATIVE_OPERAND_MEMORY) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  uint8_t base_reg = operand->value.mem.base_reg & 0x7;
  uint8_t index_reg = operand->value.mem.index_reg & 0x7;
  bool has_base = operand->value.mem.base_reg != 0;
  bool has_index = operand->value.mem.index_reg != 0;
  bool has_disp = operand->value.mem.displacement != 0;
  
  /* Handle extended registers */
  if (operand->value.mem.base_reg >= 8) {
    /* Need REX prefix with B bit */
    uint8_t rex = 0x41;
    
    /* Add to prefixes if not already present */
    bool found = false;
    for (uint8_t i = 0; i < encoding->prefix_count; i++) {
      if ((encoding->prefixes[i] & 0xF0) == 0x40) {
        encoding->prefixes[i] |= 0x01;
        found = true;
        break;
      }
    }
    
    if (!found && encoding->prefix_count < 4) {
      encoding->prefixes[encoding->prefix_count++] = rex;
    }
  }
  
  if (operand->value.mem.index_reg >= 8) {
    /* Need REX prefix with X bit */
    uint8_t rex = 0x42;
    
    /* Add to prefixes if not already present */
    bool found = false;
    for (uint8_t i = 0; i < encoding->prefix_count; i++) {
      if ((encoding->prefixes[i] & 0xF0) == 0x40) {
        encoding->prefixes[i] |= 0x02;
        found = true;
        break;
      }
    }
    
    if (!found && encoding->prefix_count < 4) {
      encoding->prefixes[encoding->prefix_count++] = rex;
    }
  }
  
  /* Check for segment override */
  if (operand->value.mem.segment != 0) {
    uint8_t segment_prefix = 0;
    
    /* Map segment to prefix */
    switch (operand->value.mem.segment) {
      case 0:  /* Default segment */
        break;
      case 1:  /* CS */
        segment_prefix = 0x2E;
        break;
      case 2:  /* SS */
        segment_prefix = 0x36;
        break;
      case 3:  /* DS */
        segment_prefix = 0x3E;
        break;
      case 4:  /* FS */
        segment_prefix = 0x64;
        break;
      case 5:  /* GS */
        segment_prefix = 0x65;
        break;
      default:
        log_warning("Unknown segment identifier: %u", operand->value.mem.segment);
        break;
    }
    
    /* Add segment override prefix if needed */
    if (segment_prefix != 0 && encoding->prefix_count < 4) {
      encoding->prefixes[encoding->prefix_count++] = segment_prefix;
    }
  }
  
  /* Address size override for 16-bit addressing in 32-bit mode or vice versa */
  if ((gen_data->is_32bit && encoding->modrm == 0) ||
      (gen_data->is_16bit && encoding->modrm != 0)) {
    if (encoding->prefix_count < 4) {
      encoding->prefixes[encoding->prefix_count++] = 0x67;
    }
  }
  
  /* Determine ModR/M and SIB encoding */
  uint8_t mod;
  
  if (!has_disp) {
    mod = 0;  /* No displacement */
  } else if (operand->value.mem.displacement >= -128 && operand->value.mem.displacement <= 127) {
    mod = 1;  /* 8-bit displacement */
    encoding->displacement = operand->value.mem.displacement;
    encoding->displacement_size = 1;
  } else {
    mod = 2;  /* 32-bit displacement */
    encoding->displacement = operand->value.mem.displacement;
    encoding->displacement_size = 4;
  }
  
  /* Special case: displacement only */
  if (!has_base && !has_index) {
    encoding->modrm = create_modrm(mod, encoding->modrm >> 3, 5);
    encoding->has_modrm = 1;
    
    /* Always use 32-bit displacement for absolute addresses */
    encoding->displacement = operand->value.mem.displacement;
    encoding->displacement_size = 4;
    
    return ERROR_NONE;
  }
  
  /* Special case: [BP] or [EBP] needs displacement */
  if (has_base && base_reg == 5 && !has_disp) {
    encoding->displacement = 0;
    encoding->displacement_size = 1;
    mod = 1;  /* 8-bit displacement */
  }
  
  /* Check if SIB byte is needed */
  bool need_sib = false;
  
  /* SIB is needed for scaled index or when the base is ESP/RSP */
  if (has_index || (has_base && base_reg == 4)) {
    need_sib = true;
  }
  
  if (need_sib) {
    /* Use SIB encoding */
    encoding->modrm = create_modrm(mod, encoding->modrm >> 3, 4);
    encoding->has_modrm = 1;
    
    /* Create SIB byte */
    uint8_t scale = 0;
    
    switch (operand->value.mem.scale) {
      case 1:
        scale = 0;
        break;
      case 2:
        scale = 1;
        break;
      case 4:
        scale = 2;
        break;
      case 8:
        scale = 3;
        break;
      default:
        scale = 0;
        break;
    }
    
    /* If no index, use special "none" encoding */
    if (!has_index) {
      index_reg = 4;  /* Special "none" value */
    }
    
    /* If no base, use special "none" encoding */
    if (!has_base) {
      base_reg = 5;  /* Special "none" value */
      mod = 0;       /* Update MOD field */
      
      /* Need displacement */
      encoding->displacement = operand->value.mem.displacement;
      encoding->displacement_size = 4;
      
      /* Update ModR/M byte */
      encoding->modrm = create_modrm(mod, encoding->modrm >> 3, 4);
    }
    
    encoding->sib = create_sib(scale, index_reg, base_reg);
    encoding->has_sib = 1;
  } else {
    /* Direct ModR/M encoding */
    encoding->modrm = create_modrm(mod, encoding->modrm >> 3, base_reg);
    encoding->has_modrm = 1;
  }
  
  return ERROR_NONE;
}

/**
 * @brief Determines the opcode for a specific instruction
 *
 * @param[in] instruction Native instruction
 * @param[out] encoding Encoding information
 * @param[in] gen_data Generator data
 * @return Error code indicating success or failure
 */
static error_t determine_opcode(
  const native_instruction_t* instruction,
  x86_encoding_t* encoding,
  const x86_generator_data_t* gen_data
) {
  if (instruction == NULL || encoding == NULL || gen_data == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Extract the base mnemonic without size suffix */
  char base_mnemonic[MAX_MNEMONIC_LENGTH];
  strncpy(base_mnemonic, instruction->mnemonic, sizeof(base_mnemonic) - 1);
  base_mnemonic[sizeof(base_mnemonic) - 1] = '\0';
  
  /* Remove size suffix if present */
  char* suffix = strpbrk(base_mnemonic, "bwlq");
  if (suffix != NULL) {
    *suffix = '\0';
  }
  
  /* Look up the opcode based on mnemonic */
  if (strcmp(base_mnemonic, "mov") == 0) {
    /* MOV has many variants depending on operands */
    if (instruction->operand_count != 2) {
      log_error("MOV requires 2 operands");
      return ERROR_GENERATION;
    }
    
    if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER &&
        instruction->operands[1].type == NATIVE_OPERAND_REGISTER) {
      /* register to register */
      encoding->opcode[0] = 0x89;
      encoding->opcode_size = 1;
      
      /* REG field is source, MOD:RM field is destination */
      encode_register(&instruction->operands[1], encoding, false);
      encode_register(&instruction->operands[0], encoding, true);
    } else if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER &&
               instruction->operands[1].type == NATIVE_OPERAND_MEMORY) {
      /* memory to register */
      encoding->opcode[0] = 0x8B;
      encoding->opcode_size = 1;
      
      /* REG field is destination, MOD:RM field is source */
      encode_register(&instruction->operands[0], encoding, false);
      encode_memory(&instruction->operands[1], encoding, gen_data);
    } else if (instruction->operands[0].type == NATIVE_OPERAND_MEMORY &&
               instruction->operands[1].type == NATIVE_OPERAND_REGISTER) {
      /* register to memory */
      encoding->opcode[0] = 0x89;
      encoding->opcode_size = 1;
      
      /* REG field is source, MOD:RM field is destination */
      encode_register(&instruction->operands[1], encoding, false);
      encode_memory(&instruction->operands[0], encoding, gen_data);
    } else if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER &&
               instruction->operands[1].type == NATIVE_OPERAND_IMMEDIATE) {
      /* immediate to register */
      if (instruction->operands[0].value.reg.id < 8) {
        /* Optimized encoding for common registers */
        encoding->opcode[0] = 0xB8 + (instruction->operands[0].value.reg.id & 0x7);
        encoding->opcode_size = 1;
      } else {
        /* Use general form with ModR/M for extended registers */
        encoding->opcode[0] = 0xC7;
        encoding->opcode_size = 1;
        
        /* REG field is 0 (opcode extension), MOD:RM field is destination */
        encoding->modrm = create_modrm(3, 0, 0);  /* Placeholder */
        encode_register(&instruction->operands[0], encoding, true);
      }
      
      /* Store immediate value */
      encoding->immediate = instruction->operands[1].value.imm;
      encoding->immediate_size = instruction->operands[1].size;
    } else {
      log_error("Unsupported MOV operand combination");
      return ERROR_GENERATION;
    }
  } else if (strcmp(base_mnemonic, "add") == 0) {
    /* ADD has several variants */
    if (instruction->operand_count != 2) {
      log_error("ADD requires 2 operands");
      return ERROR_GENERATION;
    }
    
    if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER &&
        instruction->operands[1].type == NATIVE_OPERAND_REGISTER) {
      /* register to register */
      encoding->opcode[0] = 0x01;
      encoding->opcode_size = 1;
      
      /* REG field is source, MOD:RM field is destination */
      encode_register(&instruction->operands[1], encoding, false);
      encode_register(&instruction->operands[0], encoding, true);
    } else if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER &&
               instruction->operands[1].type == NATIVE_OPERAND_IMMEDIATE) {
      /* immediate to register */
      if (instruction->operands[0].value.reg.id == 0) {
        /* Special case for AL/AX/EAX/RAX */
        encoding->opcode[0] = 0x05;
        encoding->opcode_size = 1;
      } else {
        /* Use general form with ModR/M */
        encoding->opcode[0] = 0x81;
        encoding->opcode_size = 1;
        
        /* REG field is 0 (opcode extension), MOD:RM field is destination */
        encoding->modrm = create_modrm(3, 0, 0);  /* Placeholder */
        encode_register(&instruction->operands[0], encoding, true);
      }
      
      /* Store immediate value */
      encoding->immediate = instruction->operands[1].value.imm;
      encoding->immediate_size = instruction->operands[1].size;
    } else {
      log_error("Unsupported ADD operand combination");
      return ERROR_GENERATION;
    }
  } else if (strcmp(base_mnemonic, "sub") == 0) {
    /* SUB has several variants */
    if (instruction->operand_count != 2) {
      log_error("SUB requires 2 operands");
      return ERROR_GENERATION;
    }
    
    if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER &&
        instruction->operands[1].type == NATIVE_OPERAND_REGISTER) {
      /* register to register */
      encoding->opcode[0] = 0x29;
      encoding->opcode_size = 1;
      
      /* REG field is source, MOD:RM field is destination */
      encode_register(&instruction->operands[1], encoding, false);
      encode_register(&instruction->operands[0], encoding, true);
    } else if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER &&
               instruction->operands[1].type == NATIVE_OPERAND_IMMEDIATE) {
      /* immediate to register */
      if (instruction->operands[0].value.reg.id == 0) {
        /* Special case for AL/AX/EAX/RAX */
        encoding->opcode[0] = 0x2D;
        encoding->opcode_size = 1;
      } else {
        /* Use general form with ModR/M */
        encoding->opcode[0] = 0x81;
        encoding->opcode_size = 1;
        
        /* REG field is 5 (opcode extension for SUB), MOD:RM field is destination */
        encoding->modrm = create_modrm(3, 5, 0);  /* Placeholder */
        encode_register(&instruction->operands[0], encoding, true);
      }
      
      /* Store immediate value */
      encoding->immediate = instruction->operands[1].value.imm;
      encoding->immediate_size = instruction->operands[1].size;
    } else {
      log_error("Unsupported SUB operand combination");
      return ERROR_GENERATION;
    }
  } else if (strcmp(base_mnemonic, "jmp") == 0) {
    /* JMP has several variants */
    if (instruction->operand_count != 1) {
      log_error("JMP requires 1 operand");
      return ERROR_GENERATION;
    }
    
    if (instruction->operands[0].type == NATIVE_OPERAND_LABEL) {
      /* Jump to label - use relative jump for now */
      /* This would be resolved during relocation */
      encoding->opcode[0] = 0xE9;  /* Near relative jump */
      encoding->opcode_size = 1;
      
      /* Placeholder for 32-bit relative offset */
      encoding->immediate = 0;
      encoding->immediate_size = 4;
    } else if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER) {
      /* Jump to register */
      encoding->opcode[0] = 0xFF;
      encoding->opcode_size = 1;
      
      /* REG field is 4 (opcode extension for JMP), MOD:RM field is target */
      encoding->modrm = create_modrm(3, 4, 0);  /* Placeholder */
      encode_register(&instruction->operands[0], encoding, true);
    } else {
      log_error("Unsupported JMP operand type");
      return ERROR_GENERATION;
    }
  } else if (strcmp(base_mnemonic, "call") == 0) {
    /* CALL has several variants */
    if (instruction->operand_count != 1) {
      log_error("CALL requires 1 operand");
      return ERROR_GENERATION;
    }
    
    if (instruction->operands[0].type == NATIVE_OPERAND_LABEL) {
      /* Call to label - use relative call for now */
      /* This would be resolved during relocation */
      encoding->opcode[0] = 0xE8;  /* Near relative call */
      encoding->opcode_size = 1;
      
      /* Placeholder for 32-bit relative offset */
      encoding->immediate = 0;
      encoding->immediate_size = 4;
    } else if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER) {
      /* Call to register */
      encoding->opcode[0] = 0xFF;
      encoding->opcode_size = 1;
      
      /* REG field is 2 (opcode extension for CALL), MOD:RM field is target */
      encoding->modrm = create_modrm(3, 2, 0);  /* Placeholder */
      encode_register(&instruction->operands[0], encoding, true);
    } else {
      log_error("Unsupported CALL operand type");
      return ERROR_GENERATION;
    }
  } else if (strcmp(base_mnemonic, "ret") == 0) {
    /* RET is simple */
    encoding->opcode[0] = 0xC3;  /* Near return */
    encoding->opcode_size = 1;
  } else {
    log_error("Unknown or unimplemented instruction: %s", instruction->mnemonic);
    return ERROR_UNSUPPORTED;
  }
  
  /* Add operand size prefix if needed */
  uint8_t operand_size = 0;
  
  if (instruction->operand_count > 0) {
    operand_size = instruction->operands[0].size;
  }
  
  if (gen_data->is_16bit) {
    /* 16-bit mode: 32-bit operands need prefix */
    if (operand_size == 4) {
      if (encoding->prefix_count < 4) {
        encoding->prefixes[encoding->prefix_count++] = 0x66;
      }
    }
  } else {
    /* 32/64-bit mode: 16-bit operands need prefix */
    if (operand_size == 2) {
      if (encoding->prefix_count < 4) {
        encoding->prefixes[encoding->prefix_count++] = 0x66;
      }
    }
  }
  
  /* Add REX.W prefix for 64-bit operands in 64-bit mode */
  if (gen_data->is_64bit && operand_size == 8) {
    /* Check if we already have a REX prefix */
    bool has_rex = false;
    
    for (uint8_t i = 0; i < encoding->prefix_count; i++) {
      if ((encoding->prefixes[i] & 0xF0) == 0x40) {
        encoding->prefixes[i] |= 0x08;  /* Set W bit */
        has_rex = true;
        break;
      }
    }
    
    /* Add REX.W prefix if not already present */
    if (!has_rex && encoding->prefix_count < 4) {
      encoding->prefixes[encoding->prefix_count++] = 0x48;  /* REX.W */
    }
  }
  
  return ERROR_NONE;
}

error_t x86_encode_instruction(
  const native_instruction_t* instruction,
  x86_encoding_t* encoding,
  const x86_generator_data_t* generator_data
) {
  if (instruction == NULL || encoding == NULL || generator_data == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Initialize encoding */
  memset(encoding, 0, sizeof(x86_encoding_t));
  
  /* Determine opcode and encode operands */
  return determine_opcode(instruction, encoding, generator_data);
}

size_t x86_calculate_instruction_size(const x86_encoding_t* encoding) {
  if (encoding == NULL) {
    return 0;
  }
  
  size_t size = 0;
  
  /* Add prefix size */
  size += encoding->prefix_count;
  
  /* Add opcode size */
  size += encoding->opcode_size;
  
  /* Add ModR/M byte if present */
  if (encoding->has_modrm) {
    size += 1;
  }
  
  /* Add SIB byte if present */
  if (encoding->has_sib) {
    size += 1;
  }
  
  /* Add displacement if present */
  size += encoding->displacement_size;
  
  /* Add immediate if present */
  size += encoding->immediate_size;
  
  return size;
}

size_t x86_write_instruction(
  uint8_t* buffer,
  const x86_encoding_t* encoding
) {
  if (buffer == NULL || encoding == NULL) {
    return 0;
  }
  
  size_t offset = 0;
  
  /* Write prefixes */
  for (uint8_t i = 0; i < encoding->prefix_count; i++) {
    buffer[offset++] = encoding->prefixes[i];
  }
  
  /* Write opcode */
  for (uint8_t i = 0; i < encoding->opcode_size; i++) {
    buffer[offset++] = encoding->opcode[i];
  }
  
  /* Write ModR/M byte if present */
  if (encoding->has_modrm) {
    buffer[offset++] = encoding->modrm;
  }
  
  /* Write SIB byte if present */
  if (encoding->has_sib) {
    buffer[offset++] = encoding->sib;
  }
  
  /* Write displacement if present */
  if (encoding->displacement_size == 1) {
    buffer[offset++] = (uint8_t)encoding->displacement;
  } else if (encoding->displacement_size == 4) {
    buffer[offset++] = (uint8_t)encoding->displacement;
    buffer[offset++] = (uint8_t)(encoding->displacement >> 8);
    buffer[offset++] = (uint8_t)(encoding->displacement >> 16);
    buffer[offset++] = (uint8_t)(encoding->displacement >> 24);
  }
  
  /* Write immediate if present */
  if (encoding->immediate_size == 1) {
    buffer[offset++] = (uint8_t)encoding->immediate;
  } else if (encoding->immediate_size == 2) {
    buffer[offset++] = (uint8_t)encoding->immediate;
    buffer[offset++] = (uint8_t)(encoding->immediate >> 8);
  } else if (encoding->immediate_size == 4) {
    buffer[offset++] = (uint8_t)encoding->immediate;
    buffer[offset++] = (uint8_t)(encoding->immediate >> 8);
    buffer[offset++] = (uint8_t)(encoding->immediate >> 16);
    buffer[offset++] = (uint8_t)(encoding->immediate >> 24);
  } else if (encoding->immediate_size == 8) {
    buffer[offset++] = (uint8_t)encoding->immediate;
    buffer[offset++] = (uint8_t)(encoding->immediate >> 8);
    buffer[offset++] = (uint8_t)(encoding->immediate >> 16);
    buffer[offset++] = (uint8_t)(encoding->immediate >> 24);
    buffer[offset++] = (uint8_t)(encoding->immediate >> 32);
    buffer[offset++] = (uint8_t)(encoding->immediate >> 40);
    buffer[offset++] = (uint8_t)(encoding->immediate >> 48);
    buffer[offset++] = (uint8_t)(encoding->immediate >> 56);
  }
  
  return offset;
}

error_t x86_generate_relocation(
  const native_instruction_t* instruction,
  uint64_t instruction_offset,
  uint32_t symbol_index,
  native_relocation_t* relocation,
  const x86_generator_data_t* generator_data
) {
  if (instruction == NULL || relocation == NULL || generator_data == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Initialize relocation */
  memset(relocation, 0, sizeof(native_relocation_t));
  
  relocation->symbol_index = symbol_index;
  
  /* Default values */
  relocation->type = NATIVE_RELOC_ABSOLUTE;
  relocation->offset = instruction_offset;
  relocation->addend = 0;
  
  /* For simplified x86 implementation, we only handle a few cases */
  if (strcmp(instruction->mnemonic, "jmp") == 0 ||
      strcmp(instruction->mnemonic, "call") == 0) {
    /* Relative relocation for control flow instructions */
    relocation->type = NATIVE_RELOC_RELATIVE;
    
    /* Offset to the immediate value (after opcode) */
    relocation->offset = instruction_offset + 1;
    
    /* No addend for relative jumps/calls */
    relocation->addend = 0;
  } else if (strncmp(instruction->mnemonic, "mov", 3) == 0) {
    /* Absolute relocation for memory references */
    relocation->type = NATIVE_RELOC_ABSOLUTE;
    
    /* Offset depends on instruction format */
    if (instruction->operands[0].type == NATIVE_OPERAND_REGISTER &&
        instruction->operands[1].type == NATIVE_OPERAND_IMMEDIATE) {
      /* mov reg, imm - relocation at immediate offset */
      relocation->offset = instruction_offset + 1;  /* After opcode */
      
      /* If ModR/M present, adjust offset */
      if (instruction->opcode[0] == 0xC7) {
        relocation->offset += 1;  /* After ModR/M */
      }
    } else {
      /* Other mov variants would be more complex */
      log_error("Unsupported relocation for MOV instruction");
      return ERROR_UNSUPPORTED;
    }
  } else {
    log_error("Unsupported relocation for instruction: %s", instruction->mnemonic);
    return ERROR_UNSUPPORTED;
  }
  
  return ERROR_NONE;
}

error_t x86_add_sections(native_generator_t* generator) {
  if (generator == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  uint16_t text_section_index;
  uint16_t data_section_index;
  uint16_t bss_section_index;
  uint16_t rodata_section_index;
  uint16_t symtab_section_index;
  uint16_t strtab_section_index;
  
  /* Add .text section for code */
  error_t text_result = native_generator_add_section(
    generator,
    ".text",
    NATIVE_SECTION_TEXT,
    NATIVE_SECTION_FLAG_EXEC | NATIVE_SECTION_FLAG_ALLOC,
    &text_section_index
  );
  
  if (text_result != ERROR_NONE) {
    return text_result;
  }
  
  /* Add .data section for writeable data */
  error_t data_result = native_generator_add_section(
    generator,
    ".data",
    NATIVE_SECTION_DATA,
    NATIVE_SECTION_FLAG_WRITE | NATIVE_SECTION_FLAG_ALLOC,
    &data_section_index
  );
  
  if (data_result != ERROR_NONE) {
    return data_result;
  }
  
  /* Add .bss section for uninitialized data */
  error_t bss_result = native_generator_add_section(
    generator,
    ".bss",
    NATIVE_SECTION_BSS,
    NATIVE_SECTION_FLAG_WRITE | NATIVE_SECTION_FLAG_ALLOC,
    &bss_section_index
  );
  
  if (bss_result != ERROR_NONE) {
    return bss_result;
  }
  
  /* Add .rodata section for read-only data */
  error_t rodata_result = native_generator_add_section(
    generator,
    ".rodata",
    NATIVE_SECTION_RODATA,
    NATIVE_SECTION_FLAG_ALLOC,
    &rodata_section_index
  );
  
  if (rodata_result != ERROR_NONE) {
    return rodata_result;
  }
  
  /* Add .symtab section for symbol table */
  error_t symtab_result = native_generator_add_section(
    generator,
    ".symtab",
    NATIVE_SECTION_SYMTAB,
    NATIVE_SECTION_FLAG_NONE,
    &symtab_section_index
  );
  
  if (symtab_result != ERROR_NONE) {
    return symtab_result;
  }
  
  /* Add .strtab section for string table */
  error_t strtab_result = native_generator_add_section(
    generator,
    ".strtab",
    NATIVE_SECTION_STRTAB,
    NATIVE_SECTION_FLAG_NONE | NATIVE_SECTION_FLAG_STRINGS,
    &strtab_section_index
  );
  
  if (strtab_result != ERROR_NONE) {
    return strtab_result;
  }
  
  return ERROR_NONE;
}

error_t x86_process_function(
  native_generator_t* generator,
  const char* function_name,
  const native_instruction_list_t* instructions,
  uint32_t* symbol_index
) {
  if (generator == NULL || function_name == NULL || 
      instructions == NULL || symbol_index == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  x86_generator_data_t* data = x86_generator_get_data(generator);
  if (data == NULL) {
    return ERROR_GENERATION;
  }
  
  /* Find code section */
  uint16_t text_section;
  error_t section_result = native_generator_get_section_by_name(
    generator,
    ".text",
    &text_section
  );
  
  if (section_result != ERROR_NONE) {
    return section_result;
  }
  
  /* Calculate total code size */
  size_t total_size = 0;
  uint8_t* code_buffer = NULL;
  
  /* First pass: calculate size */
  for (size_t i = 0; i < instructions->count; i++) {
    x86_encoding_t encoding;
    
    error_t encode_result = x86_encode_instruction(
      &instructions->instructions[i],
      &encoding,
      data
    );
    
    if (encode_result != ERROR_NONE) {
      log_error("Failed to encode instruction %zu", i);
      return encode_result;
    }
    
    total_size += x86_calculate_instruction_size(&encoding);
  }
  
  /* Allocate code buffer */
  code_buffer = memory_alloc(total_size);
  if (code_buffer == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Second pass: encode instructions */
  size_t offset = 0;
  
  for (size_t i = 0; i < instructions->count; i++) {
    x86_encoding_t encoding;
    
    error_t encode_result = x86_encode_instruction(
      &instructions->instructions[i],
      &encoding,
      data
    );
    
    if (encode_result != ERROR_NONE) {
      memory_free(code_buffer);
      return encode_result;
    }
    
    size_t instr_size = x86_write_instruction(code_buffer + offset, &encoding);
    offset += instr_size;
  }
  
  /* Add code to text section */
  uint64_t function_offset = 0;
  error_t add_result = native_generator_add_section_data(
    generator,
    text_section,
    code_buffer,
    total_size,
    data->is_64bit ? 16 : 4,  /* Alignment: 16 for 64-bit, 4 for 32/16-bit */
    &function_offset
  );
  
  if (add_result != ERROR_NONE) {
    memory_free(code_buffer);
    return add_result;
  }
  
  /* Add function symbol */
  error_t symbol_result = native_generator_add_symbol(
    generator,
    function_name,
    function_offset,
    total_size,
    NATIVE_SYMBOL_FUNCTION,
    NATIVE_BINDING_GLOBAL,
    text_section,
    symbol_index
  );
  
  if (symbol_result != ERROR_NONE) {
    memory_free(code_buffer);
    return symbol_result;
  }
  
  /* Free code buffer */
  memory_free(code_buffer);
  
  return ERROR_NONE;
}

error_t x86_generate_elf_binary(
  native_generator_t* generator,
  uint8_t** binary,
  size_t* size
) {
  if (generator == NULL || binary == NULL || size == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  x86_generator_data_t* data = x86_generator_get_data(generator);
  if (data == NULL) {
    return ERROR_GENERATION;
  }
  
  /* This is a simplified ELF generation for demonstration purposes.
   * A real implementation would be much more complex and handle
   * all the details of the ELF format, sections, symbols, etc.
   */
  
  /* For now, log what we would do */
  log_info("Generating %s-bit ELF binary",
          data->is_64bit ? "64" : data->is_32bit ? "32" : "16");
  
  /* Check for missing implementation */
  log_error("Full ELF generation not implemented");
  
  /* Return a small dummy ELF header for now */
  size_t header_size = data->is_64bit ? X86_ELF_HEADER_SIZE64 : X86_ELF_HEADER_SIZE32;
  uint8_t* elf_header = memory_calloc(1, header_size);
  
  if (elf_header == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Fill in ELF magic */
  memcpy(elf_header, ELF_MAGIC, 4);
  
  /* EI_CLASS */
  elf_header[4] = data->is_64bit ? 2 : 1;  /* 1=32-bit, 2=64-bit */
  
  /* EI_DATA */
  elf_header[5] = 1;  /* 1=little-endian */
  
  /* EI_VERSION */
  elf_header[6] = 1;  /* 1=current version */
  
  /* e_machine */
  if (data->is_64bit) {
    elf_header[18] = X86_ELF_MACHINE_X86_64 & 0xFF;
    elf_header[19] = (X86_ELF_MACHINE_X86_64 >> 8) & 0xFF;
  } else {
    elf_header[18] = X86_ELF_MACHINE_386 & 0xFF;
    elf_header[19] = (X86_ELF_MACHINE_386 >> 8) & 0xFF;
  }
  
  *binary = elf_header;
  *size = header_size;
  
  return ERROR_NONE;
}