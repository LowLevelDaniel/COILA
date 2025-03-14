/**
 * @file binary.h
 * @brief COIL binary format definitions
 * @details This file contains definitions for the COIL binary format,
 *          including module structure, sections, and encoding details.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_BINARY_H
#define COIL_BINARY_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief COIL binary format magic number ("COIL" in ASCII)
 */
#define COIL_MAGIC 0x434F494C

/**
 * @brief Current COIL binary format version
 */
#define COIL_VERSION_MAJOR 0
#define COIL_VERSION_MINOR 1
#define COIL_VERSION_PATCH 0

/**
 * @brief COIL section types
 */
typedef enum {
  COIL_SECTION_TYPE_UNKNOWN = 0,
  COIL_SECTION_TYPE_TYPE = 1,       /**< Type definitions */
  COIL_SECTION_TYPE_FUNCTION = 2,   /**< Function declarations */
  COIL_SECTION_TYPE_GLOBAL = 3,     /**< Global variables */
  COIL_SECTION_TYPE_CONSTANT = 4,   /**< Constant data */
  COIL_SECTION_TYPE_CODE = 5,       /**< Function implementations */
  COIL_SECTION_TYPE_RELOCATION = 6, /**< Relocation information */
  COIL_SECTION_TYPE_METADATA = 7,   /**< Optional metadata */
  COIL_SECTION_TYPE_DEBUG = 8,      /**< Debug information */
  COIL_SECTION_TYPE_CUSTOM = 9      /**< Custom sections */
} coil_section_type_t;

/**
 * @brief COIL module header
 * @details Every COIL binary module begins with this header
 */
typedef struct {
  uint32_t magic;         /**< Magic number (COIL_MAGIC) */
  uint8_t  version_major; /**< Major version */
  uint8_t  version_minor; /**< Minor version */
  uint16_t version_patch; /**< Patch version */
  uint32_t section_count; /**< Number of sections */
  uint32_t flags;         /**< Module flags */
} coil_module_header_t;

/**
 * @brief COIL section entry
 * @details Each section in the module is described by this entry
 */
typedef struct {
  uint32_t section_type; /**< Type of section (coil_section_type_t) */
  uint32_t offset;       /**< Byte offset from start of file */
  uint32_t size;         /**< Size of section in bytes */
} coil_section_entry_t;

/**
 * @brief COIL module structure
 * @details Represents a complete COIL binary module
 */
typedef struct {
  coil_module_header_t header;        /**< Module header */
  coil_section_entry_t *sections;     /**< Section table */
  uint8_t             **section_data; /**< Section data pointers */
} coil_module_t;

/**
 * @brief Creates an empty COIL module
 * @return Pointer to a new COIL module or NULL on failure
 */
coil_module_t* coil_module_create(void);

/**
 * @brief Destroys a COIL module and frees all associated memory
 * @param module Pointer to the module to destroy
 */
void coil_module_destroy(coil_module_t *module);

/**
 * @brief Loads a COIL module from a file
 * @param filename Path to the COIL binary file
 * @return Pointer to the loaded module or NULL on failure
 */
coil_module_t* coil_module_load_from_file(const char *filename);

/**
 * @brief Loads a COIL module from memory
 * @param data Pointer to the COIL binary data
 * @param size Size of the data in bytes
 * @return Pointer to the loaded module or NULL on failure
 */
coil_module_t* coil_module_load_from_memory(const uint8_t *data, size_t size);

/**
 * @brief Writes a COIL module to a file
 * @param module Pointer to the module to write
 * @param filename Path to the output file
 * @return 0 on success, non-zero on failure
 */
int coil_module_write_to_file(const coil_module_t *module, const char *filename);

/**
 * @brief Gets a section from a COIL module
 * @param module Pointer to the module
 * @param type Type of section to get
 * @return Pointer to the section data or NULL if not found
 */
const uint8_t* coil_module_get_section(const coil_module_t *module, coil_section_type_t type);

/**
 * @brief Validates a COIL module
 * @param module Pointer to the module to validate
 * @return 0 if valid, non-zero if invalid
 */
int coil_module_validate(const coil_module_t *module);

#endif /* COIL_BINARY_H */