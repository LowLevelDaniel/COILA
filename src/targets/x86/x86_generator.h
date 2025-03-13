/**
 * @file x86_generator.h
 * @brief x86-specific native code generation
 * 
 * This module provides x86-specific implementations for generating
 * native binaries from translated instructions.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef X86_GENERATOR_H
#define X86_GENERATOR_H

#include <stdint.h>
#include <stddef.h>
#include "../../core/native_generator.h"
#include "../../core/translator.h"
#include "../../utils/error_handling.h"

/**
 * @brief x86 instruction encoding components
 */
typedef struct {
  uint8_t prefixes[4];          /**< Optional instruction prefixes */
  uint8_t prefix_count;         /**< Number of prefixes */
  uint8_t opcode[3];            /**< Opcode bytes */
  uint8_t opcode_size;          /**< Number of opcode bytes */
  uint8_t modrm;                /**< ModR/M byte */
  uint8_t sib;                  /**< SIB byte */
  uint8_t has_modrm;            /**< Whether ModR/M byte is present */
  uint8_t has_sib;              /**< Whether SIB byte is present */
  int32_t displacement;         /**< Instruction displacement */
  uint8_t displacement_size;    /**< Size of displacement in bytes */
  uint64_t immediate;           /**< Immediate value */
  uint8_t immediate_size;       /**< Size of immediate in bytes */
} x86_encoding_t;

/**
 * @brief x86-specific generator data
 */
typedef struct {
  bool is_64bit;                /**< Whether target is 64-bit mode */
  bool is_32bit;                /**< Whether target is 32-bit mode */
  bool is_16bit;                /**< Whether target is 16-bit mode */
  void* target_specific;        /**< Target-specific data */
} x86_generator_data_t;

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
);

/**
 * @brief Cleans up x86-specific generator resources
 *
 * @param[in] generator Native generator
 * @return Error code indicating success or failure
 */
error_t x86_generator_cleanup(native_generator_t* generator);

/**
 * @brief Gets x86-specific generator data
 *
 * @param[in] generator Native generator
 * @return x86-specific generator data, or NULL on error
 */
x86_generator_data_t* x86_generator_get_data(native_generator_t* generator);

/**
 * @brief Encodes an x86 instruction
 *
 * @param[in] instruction Native instruction to encode
 * @param[out] encoding Pointer to receive the encoded instruction
 * @param[in] generator_data x86-specific generator data
 * @return Error code indicating success or failure
 */
error_t x86_encode_instruction(
  const native_instruction_t* instruction,
  x86_encoding_t* encoding,
  const x86_generator_data_t* generator_data
);

/**
 * @brief Calculates the size of an encoded x86 instruction
 *
 * @param[in] encoding Encoded instruction
 * @return Size of the instruction in bytes
 */
size_t x86_calculate_instruction_size(const x86_encoding_t* encoding);

/**
 * @brief Writes an encoded x86 instruction to a buffer
 *
 * @param[out] buffer Buffer to write to
 * @param[in] encoding Encoded instruction
 * @return Number of bytes written
 */
size_t x86_write_instruction(
  uint8_t* buffer,
  const x86_encoding_t* encoding
);

/**
 * @brief Generates an x86 relocation entry
 *
 * @param[in] instruction Native instruction with relocation
 * @param[in] instruction_offset Offset of the instruction in the section
 * @param[in] symbol_index Symbol index for the relocation
 * @param[out] relocation Pointer to receive the generated relocation
 * @param[in] generator_data x86-specific generator data
 * @return Error code indicating success or failure
 */
error_t x86_generate_relocation(
  const native_instruction_t* instruction,
  uint64_t instruction_offset,
  uint32_t symbol_index,
  native_relocation_t* relocation,
  const x86_generator_data_t* generator_data
);

/**
 * @brief Adds x86-specific sections to the binary
 *
 * @param[in] generator Native generator
 * @return Error code indicating success or failure
 */
error_t x86_add_sections(native_generator_t* generator);

/**
 * @brief Generates x86-specific ELF headers
 *
 * @param[in] generator Native generator
 * @return Error code indicating success or failure
 */
error_t x86_generate_elf_headers(native_generator_t* generator);

/**
 * @brief Generates x86-specific ELF program headers
 *
 * @param[in] generator Native generator
 * @return Error code indicating success or failure
 */
error_t x86_generate_elf_program_headers(native_generator_t* generator);

/**
 * @brief Generates x86-specific ELF section headers
 *
 * @param[in] generator Native generator
 * @return Error code indicating success or failure
 */
error_t x86_generate_elf_section_headers(native_generator_t* generator);

/**
 * @brief Generates x86-specific ELF symbol table
 *
 * @param[in] generator Native generator
 * @return Error code indicating success or failure
 */
error_t x86_generate_elf_symbol_table(native_generator_t* generator);

/**
 * @brief Processes a function for x86 binary generation
 *
 * @param[in] generator Native generator
 * @param[in] function_name Function name
 * @param[in] instructions Instruction list
 * @param[out] symbol_index Pointer to receive the function symbol index
 * @return Error code indicating success or failure
 */
error_t x86_process_function(
  native_generator_t* generator,
  const char* function_name,
  const native_instruction_list_t* instructions,
  uint32_t* symbol_index
);

/**
 * @brief Generates x86-specific runtime support code
 *
 * @param[in] generator Native generator
 * @return Error code indicating success or failure
 */
error_t x86_generate_runtime_support(native_generator_t* generator);

/**
 * @brief Generates x86-specific startup code
 *
 * @param[in] generator Native generator
 * @return Error code indicating success or failure
 */
error_t x86_generate_startup_code(native_generator_t* generator);

#endif /* X86_GENERATOR_H */