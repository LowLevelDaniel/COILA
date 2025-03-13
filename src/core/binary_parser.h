/**
 * @file binary_parser.h
 * @brief Parser for COIL binary format
 * 
 * This module parses COIL binary format, extracting sections and instructions
 * for further processing.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef BINARY_PARSER_H
#define BINARY_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include "../utils/error_handling.h"

/**
 * @brief COIL binary magic number ("COIL" in ASCII)
 */
#define COIL_MAGIC 0x434F494C

/**
 * @brief COIL binary section types
 */
typedef enum {
  COIL_SECTION_TYPE = 1,       /**< Type definitions section */
  COIL_SECTION_FUNCTION = 2,    /**< Function declarations section */
  COIL_SECTION_GLOBAL = 3,      /**< Global variable declarations section */
  COIL_SECTION_CONSTANT = 4,    /**< Constant data section */
  COIL_SECTION_CODE = 5,        /**< Function implementations section */
  COIL_SECTION_RELOCATION = 6,  /**< Relocation information section */
  COIL_SECTION_METADATA = 7     /**< Optional metadata section */
} coil_section_type_t;

/**
 * @brief COIL binary file header
 */
typedef struct {
  uint32_t magic;              /**< Magic number ("COIL" in ASCII) */
  uint32_t version;            /**< Version information (major.minor.patch) */
  uint32_t section_count;      /**< Number of sections in the module */
  uint32_t flags;              /**< Module-specific flags */
} coil_header_t;

/**
 * @brief COIL binary section entry
 */
typedef struct {
  uint32_t section_type;       /**< Type of section */
  uint32_t offset;             /**< Byte offset from start of file */
  uint32_t size;               /**< Size of section in bytes */
} coil_section_entry_t;

/**
 * @brief COIL module structure holding parsed binary data
 */
typedef struct {
  coil_header_t header;                       /**< Binary header */
  coil_section_entry_t* section_table;        /**< Section table */
  uint8_t** section_data;                     /**< Array of pointers to section data */
  
  /* Parsed sections */
  void* type_section;                         /**< Parsed type definitions */
  void* function_section;                     /**< Parsed function declarations */
  void* global_section;                       /**< Parsed global declarations */
  void* constant_section;                     /**< Parsed constant data */
  void* code_section;                         /**< Parsed code */
  void* relocation_section;                   /**< Parsed relocation information */
  void* metadata_section;                     /**< Parsed metadata */
} coil_module_t;

/**
 * @brief Parses COIL binary data into a module structure
 *
 * @param[in] binary Pointer to COIL binary data
 * @param[in] size Size of the binary data in bytes
 * @param[out] module Pointer to receive the parsed module
 * @return Error code indicating success or failure
 */
error_t binary_parser_parse(
  const uint8_t* binary, 
  size_t size,
  coil_module_t** module
);

/**
 * @brief Frees resources associated with a parsed COIL module
 *
 * @param[in] module Pointer to the module to free
 * @return Error code indicating success or failure
 */
error_t binary_parser_free_module(coil_module_t* module);

/**
 * @brief Gets a pointer to a specific section in a COIL module
 *
 * @param[in] module Pointer to the COIL module
 * @param[in] section_type Type of section to retrieve
 * @param[out] section_data Pointer to receive the section data
 * @param[out] section_size Pointer to receive the section size
 * @return Error code indicating success or failure
 */
error_t binary_parser_get_section(
  const coil_module_t* module,
  coil_section_type_t section_type,
  const uint8_t** section_data,
  size_t* section_size
);

/**
 * @brief Validates a COIL binary header
 *
 * @param[in] header Pointer to the COIL header to validate
 * @return Error code indicating success or failure
 */
error_t binary_parser_validate_header(const coil_header_t* header);

#endif /* BINARY_PARSER_H */