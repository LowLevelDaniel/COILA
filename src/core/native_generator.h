/**
 * @file native_generator.h
 * @brief Native binary generation interface
 * 
 * This module defines the interface for generating native binary output
 * from native instructions for specific target architectures.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef NATIVE_GENERATOR_H
#define NATIVE_GENERATOR_H

#include <stdint.h>
#include <stddef.h>
#include "error_handling.h"
#include "translator.h"
#include "target_config.h"

/**
 * @brief Native output format types
 */
typedef enum {
  NATIVE_FORMAT_RAW,            /**< Raw binary output */
  NATIVE_FORMAT_ELF,            /**< ELF format output */
  NATIVE_FORMAT_COFF,           /**< COFF format output */
  NATIVE_FORMAT_MACHO           /**< Mach-O format output */
} native_format_t;

/**
 * @brief Native symbol types
 */
typedef enum {
  NATIVE_SYMBOL_FUNCTION,       /**< Function symbol */
  NATIVE_SYMBOL_DATA,           /**< Data symbol */
  NATIVE_SYMBOL_SECTION,        /**< Section symbol */
  NATIVE_SYMBOL_FILE            /**< File symbol */
} native_symbol_type_t;

/**
 * @brief Native symbol binding
 */
typedef enum {
  NATIVE_BINDING_LOCAL,         /**< Local symbol */
  NATIVE_BINDING_GLOBAL,        /**< Global symbol */
  NATIVE_BINDING_WEAK           /**< Weak symbol */
} native_symbol_binding_t;

/**
 * @brief Native relocation types
 */
typedef enum {
  NATIVE_RELOC_ABSOLUTE,        /**< Absolute address relocation */
  NATIVE_RELOC_RELATIVE,        /**< Relative address relocation */
  NATIVE_RELOC_PLT,             /**< Procedure linkage table relocation */
  NATIVE_RELOC_GOT              /**< Global offset table relocation */
} native_reloc_type_t;

/**
 * @brief Native symbol information
 */
typedef struct {
  char* name;                   /**< Symbol name */
  uint64_t value;               /**< Symbol value (address or offset) */
  uint64_t size;                /**< Symbol size */
  native_symbol_type_t type;    /**< Symbol type */
  native_symbol_binding_t binding; /**< Symbol binding */
  uint16_t section_index;       /**< Section index */
} native_symbol_t;

/**
 * @brief Native relocation information
 */
typedef struct {
  uint64_t offset;              /**< Relocation offset */
  uint64_t addend;              /**< Relocation addend */
  native_reloc_type_t type;     /**< Relocation type */
  uint32_t symbol_index;        /**< Symbol index */
} native_relocation_t;

/**
 * @brief Native section types
 */
typedef enum {
  NATIVE_SECTION_TEXT,          /**< Code section */
  NATIVE_SECTION_DATA,          /**< Data section */
  NATIVE_SECTION_BSS,           /**< BSS section */
  NATIVE_SECTION_RODATA,        /**< Read-only data section */
  NATIVE_SECTION_REL,           /**< Relocation section */
  NATIVE_SECTION_SYMTAB,        /**< Symbol table section */
  NATIVE_SECTION_STRTAB,        /**< String table section */
  NATIVE_SECTION_DEBUG          /**< Debug information section */
} native_section_type_t;

/**
 * @brief Native section flags
 */
typedef enum {
  NATIVE_SECTION_FLAG_NONE = 0,
  NATIVE_SECTION_FLAG_WRITE = 1,       /**< Section is writable */
  NATIVE_SECTION_FLAG_ALLOC = 2,       /**< Section occupies memory during execution */
  NATIVE_SECTION_FLAG_EXEC = 4,        /**< Section contains executable code */
  NATIVE_SECTION_FLAG_MERGE = 8,       /**< Section can be merged */
  NATIVE_SECTION_FLAG_STRINGS = 16     /**< Section contains null-terminated strings */
} native_section_flags_t;

/**
 * @brief Native section information
 */
typedef struct {
  char* name;                         /**< Section name */
  native_section_type_t type;         /**< Section type */
  uint32_t flags;                     /**< Section flags (combination of native_section_flags_t) */
  uint8_t* data;                      /**< Section data */
  size_t size;                        /**< Section size */
  size_t alignment;                   /**< Section alignment */
  native_relocation_t* relocations;   /**< Relocations in this section */
  size_t relocation_count;            /**< Number of relocations */
} native_section_t;

/**
 * @brief Native binary output context
 */
typedef struct native_generator_t native_generator_t;

/**
 * @brief Creates a native generator context
 *
 * @param[in] target Target configuration
 * @param[in] format Output format
 * @param[out] generator Pointer to receive the created generator
 * @return Error code indicating success or failure
 */
error_t native_generator_create(
  const target_config_t* target,
  native_format_t format,
  native_generator_t** generator
);

/**
 * @brief Destroys a native generator context and frees associated resources
 *
 * @param[in] generator Pointer to the generator to destroy
 * @return Error code indicating success or failure
 */
error_t native_generator_destroy(native_generator_t* generator);

/**
 * @brief Adds a section to the native binary
 *
 * @param[in] generator Native generator
 * @param[in] name Section name
 * @param[in] type Section type
 * @param[in] flags Section flags
 * @param[out] section_index Pointer to receive the section index
 * @return Error code indicating success or failure
 */
error_t native_generator_add_section(
  native_generator_t* generator,
  const char* name,
  native_section_type_t type,
  uint32_t flags,
  uint16_t* section_index
);

/**
 * @brief Adds data to a section
 *
 * @param[in] generator Native generator
 * @param[in] section_index Section index
 * @param[in] data Data to add
 * @param[in] size Size of data in bytes
 * @param[in] alignment Alignment of data
 * @param[out] offset Pointer to receive the offset of added data
 * @return Error code indicating success or failure
 */
error_t native_generator_add_section_data(
  native_generator_t* generator,
  uint16_t section_index,
  const uint8_t* data,
  size_t size,
  size_t alignment,
  uint64_t* offset
);

/**
 * @brief Adds a symbol to the native binary
 *
 * @param[in] generator Native generator
 * @param[in] name Symbol name
 * @param[in] value Symbol value (address or offset)
 * @param[in] size Symbol size
 * @param[in] type Symbol type
 * @param[in] binding Symbol binding
 * @param[in] section_index Section index
 * @param[out] symbol_index Pointer to receive the symbol index
 * @return Error code indicating success or failure
 */
error_t native_generator_add_symbol(
  native_generator_t* generator,
  const char* name,
  uint64_t value,
  uint64_t size,
  native_symbol_type_t type,
  native_symbol_binding_t binding,
  uint16_t section_index,
  uint32_t* symbol_index
);

/**
 * @brief Adds a relocation to the native binary
 *
 * @param[in] generator Native generator
 * @param[in] section_index Section index
 * @param[in] offset Relocation offset
 * @param[in] addend Relocation addend
 * @param[in] type Relocation type
 * @param[in] symbol_index Symbol index
 * @return Error code indicating success or failure
 */
error_t native_generator_add_relocation(
  native_generator_t* generator,
  uint16_t section_index,
  uint64_t offset,
  uint64_t addend,
  native_reloc_type_t type,
  uint32_t symbol_index
);

/**
 * @brief Sets the entry point of the native binary
 *
 * @param[in] generator Native generator
 * @param[in] symbol_index Symbol index of entry point
 * @return Error code indicating success or failure
 */
error_t native_generator_set_entry_point(
  native_generator_t* generator,
  uint32_t symbol_index
);

/**
 * @brief Adds function instructions to the native binary
 *
 * @param[in] generator Native generator
 * @param[in] function_name Function name
 * @param[in] instructions Instruction list
 * @param[out] symbol_index Pointer to receive the function symbol index
 * @return Error code indicating success or failure
 */
error_t native_generator_add_function(
  native_generator_t* generator,
  const char* function_name,
  const native_instruction_list_t* instructions,
  uint32_t* symbol_index
);

/**
 * @brief Generates the final native binary
 *
 * @param[in] generator Native generator
 * @param[out] binary Pointer to receive the generated binary
 * @param[out] size Pointer to receive the size of the binary
 * @return Error code indicating success or failure
 */
error_t native_generator_generate(
  native_generator_t* generator,
  uint8_t** binary,
  size_t* size
);

/**
 * @brief Writes the native binary to a file
 *
 * @param[in] generator Native generator
 * @param[in] filename Path to output file
 * @return Error code indicating success or failure
 */
error_t native_generator_write_file(
  native_generator_t* generator,
  const char* filename
);

/**
 * @brief Gets a section by name
 *
 * @param[in] generator Native generator
 * @param[in] name Section name
 * @param[out] section_index Pointer to receive the section index
 * @return Error code indicating success or failure
 */
error_t native_generator_get_section_by_name(
  const native_generator_t* generator,
  const char* name,
  uint16_t* section_index
);

/**
 * @brief Gets a symbol by name
 *
 * @param[in] generator Native generator
 * @param[in] name Symbol name
 * @param[out] symbol_index Pointer to receive the symbol index
 * @return Error code indicating success or failure
 */
error_t native_generator_get_symbol_by_name(
  const native_generator_t* generator,
  const char* name,
  uint32_t* symbol_index
);

/**
 * @brief Sets architecture-specific generator data
 *
 * @param[in] generator Native generator
 * @param[in] arch_data Architecture-specific data (implementation-defined)
 * @return Error code indicating success or failure
 */
error_t native_generator_set_arch_data(
  native_generator_t* generator,
  void* arch_data
);

/**
 * @brief Gets architecture-specific generator data
 *
 * @param[in] generator Native generator
 * @return Architecture-specific data (implementation-defined)
 */
void* native_generator_get_arch_data(const native_generator_t* generator);

#endif /* NATIVE_GENERATOR_H */