/**
 * @file binary_parser.c
 * @brief Implementation of the COIL binary format parser
 * 
 * This module implements parsing of COIL binary format, extracting sections
 * and instructions for further processing.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "binary_parser.h"
#include "../utils/memory_management.h"
#include "../utils/logging.h"

/**
 * @brief Validates that a buffer has enough bytes for a given size
 *
 * @param[in] buffer Pointer to buffer
 * @param[in] buffer_size Size of buffer in bytes
 * @param[in] offset Current offset in buffer
 * @param[in] required_size Required size in bytes
 * @return true if buffer has enough bytes, false otherwise
 */
static bool validate_buffer_size(
  const uint8_t* buffer,
  size_t buffer_size,
  size_t offset,
  size_t required_size
) {
  if (buffer == NULL) {
    return false;
  }
  
  if (offset > buffer_size) {
    return false;
  }
  
  if (required_size > buffer_size - offset) {
    return false;
  }
  
  return true;
}

/**
 * @brief Reads a 32-bit unsigned integer from a buffer with bounds checking
 *
 * @param[in] buffer Pointer to buffer
 * @param[in] buffer_size Size of buffer in bytes
 * @param[in] offset Offset in buffer
 * @param[out] value Pointer to receive the value
 * @return true if read was successful, false otherwise
 */
static bool read_uint32(
  const uint8_t* buffer,
  size_t buffer_size,
  size_t offset,
  uint32_t* value
) {
  if (!validate_buffer_size(buffer, buffer_size, offset, sizeof(uint32_t))) {
    return false;
  }
  
  /* Read little-endian uint32 */
  *value = (uint32_t)buffer[offset] |
           ((uint32_t)buffer[offset + 1] << 8) |
           ((uint32_t)buffer[offset + 2] << 16) |
           ((uint32_t)buffer[offset + 3] << 24);
  
  return true;
}

error_t binary_parser_parse(
  const uint8_t* binary,
  size_t size,
  coil_module_t** module
) {
  if (binary == NULL || size == 0 || module == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *module = NULL;
  
  /* Validate minimum size for header */
  if (size < sizeof(coil_header_t)) {
    log_error("Binary is too small to contain a valid header (size: %zu)", size);
    return ERROR_INVALID_FORMAT;
  }
  
  /* Allocate module structure */
  coil_module_t* new_module = memory_calloc(1, sizeof(coil_module_t));
  if (new_module == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Read header fields */
  size_t offset = 0;
  
  if (!read_uint32(binary, size, offset, &new_module->header.magic)) {
    log_error("Failed to read magic number from binary");
    memory_free(new_module);
    return ERROR_INVALID_FORMAT;
  }
  offset += sizeof(uint32_t);
  
  if (!read_uint32(binary, size, offset, &new_module->header.version)) {
    log_error("Failed to read version from binary");
    memory_free(new_module);
    return ERROR_INVALID_FORMAT;
  }
  offset += sizeof(uint32_t);
  
  if (!read_uint32(binary, size, offset, &new_module->header.section_count)) {
    log_error("Failed to read section count from binary");
    memory_free(new_module);
    return ERROR_INVALID_FORMAT;
  }
  offset += sizeof(uint32_t);
  
  if (!read_uint32(binary, size, offset, &new_module->header.flags)) {
    log_error("Failed to read flags from binary");
    memory_free(new_module);
    return ERROR_INVALID_FORMAT;
  }
  offset += sizeof(uint32_t);
  
  /* Validate header */
  error_t header_validation = binary_parser_validate_header(&new_module->header);
  if (header_validation != ERROR_NONE) {
    log_error("Invalid COIL binary header");
    memory_free(new_module);
    return header_validation;
  }
  
  /* Validate size for section table */
  size_t section_table_size = new_module->header.section_count * sizeof(coil_section_entry_t);
  if (!validate_buffer_size(binary, size, offset, section_table_size)) {
    log_error("Binary is too small to contain section table (needed: %zu, available: %zu)",
             section_table_size, size - offset);
    memory_free(new_module);
    return ERROR_INVALID_FORMAT;
  }
  
  /* Allocate section table */
  new_module->section_table = memory_calloc(
    new_module->header.section_count,
    sizeof(coil_section_entry_t)
  );
  if (new_module->section_table == NULL) {
    memory_free(new_module);
    return ERROR_MEMORY;
  }
  
  /* Read section table */
  for (uint32_t i = 0; i < new_module->header.section_count; i++) {
    if (!read_uint32(binary, size, offset, &new_module->section_table[i].section_type)) {
      log_error("Failed to read section type for section %u", i);
      memory_free(new_module->section_table);
      memory_free(new_module);
      return ERROR_INVALID_FORMAT;
    }
    offset += sizeof(uint32_t);
    
    if (!read_uint32(binary, size, offset, &new_module->section_table[i].offset)) {
      log_error("Failed to read section offset for section %u", i);
      memory_free(new_module->section_table);
      memory_free(new_module);
      return ERROR_INVALID_FORMAT;
    }
    offset += sizeof(uint32_t);
    
    if (!read_uint32(binary, size, offset, &new_module->section_table[i].size)) {
      log_error("Failed to read section size for section %u", i);
      memory_free(new_module->section_table);
      memory_free(new_module);
      return ERROR_INVALID_FORMAT;
    }
    offset += sizeof(uint32_t);
    
    /* Validate section offset and size */
    if (new_module->section_table[i].offset + new_module->section_table[i].size > size) {
      log_error("Section %u extends beyond end of binary (offset: %u, size: %u, binary size: %zu)",
               i, new_module->section_table[i].offset, new_module->section_table[i].size, size);
      memory_free(new_module->section_table);
      memory_free(new_module);
      return ERROR_INVALID_FORMAT;
    }
  }
  
  /* Allocate section data pointers */
  new_module->section_data = memory_calloc(
    new_module->header.section_count,
    sizeof(uint8_t*)
  );
  if (new_module->section_data == NULL) {
    memory_free(new_module->section_table);
    memory_free(new_module);
    return ERROR_MEMORY;
  }
  
  /* Extract section data */
  for (uint32_t i = 0; i < new_module->header.section_count; i++) {
    new_module->section_data[i] = memory_alloc(new_module->section_table[i].size);
    if (new_module->section_data[i] == NULL) {
      /* Free previously allocated section data */
      for (uint32_t j = 0; j < i; j++) {
        memory_free(new_module->section_data[j]);
      }
      memory_free(new_module->section_data);
      memory_free(new_module->section_table);
      memory_free(new_module);
      return ERROR_MEMORY;
    }
    
    memcpy(
      new_module->section_data[i],
      binary + new_module->section_table[i].offset,
      new_module->section_table[i].size
    );
    
    /* Assign sections to typed pointers based on section type */
    switch (new_module->section_table[i].section_type) {
      case COIL_SECTION_TYPE:
        new_module->type_section = new_module->section_data[i];
        break;
      
      case COIL_SECTION_FUNCTION:
        new_module->function_section = new_module->section_data[i];
        break;
      
      case COIL_SECTION_GLOBAL:
        new_module->global_section = new_module->section_data[i];
        break;
      
      case COIL_SECTION_CONSTANT:
        new_module->constant_section = new_module->section_data[i];
        break;
      
      case COIL_SECTION_CODE:
        new_module->code_section = new_module->section_data[i];
        break;
      
      case COIL_SECTION_RELOCATION:
        new_module->relocation_section = new_module->section_data[i];
        break;
      
      case COIL_SECTION_METADATA:
        new_module->metadata_section = new_module->section_data[i];
        break;
      
      default:
        log_warning("Unknown section type: %u", new_module->section_table[i].section_type);
        break;
    }
  }
  
  log_info("Successfully parsed COIL binary with %u sections",
          new_module->header.section_count);
  
  *module = new_module;
  return ERROR_NONE;
}

error_t binary_parser_free_module(coil_module_t* module) {
  if (module == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Free section data */
  if (module->section_data != NULL) {
    for (uint32_t i = 0; i < module->header.section_count; i++) {
      memory_free(module->section_data[i]);
    }
    memory_free(module->section_data);
  }
  
  /* Free section table */
  memory_free(module->section_table);
  
  /* Free module */
  memory_free(module);
  
  return ERROR_NONE;
}

error_t binary_parser_get_section(
  const coil_module_t* module,
  coil_section_type_t section_type,
  const uint8_t** section_data,
  size_t* section_size
) {
  if (module == NULL || section_data == NULL || section_size == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *section_data = NULL;
  *section_size = 0;
  
  /* Find the section in the section table */
  for (uint32_t i = 0; i < module->header.section_count; i++) {
    if (module->section_table[i].section_type == section_type) {
      *section_data = module->section_data[i];
      *section_size = module->section_table[i].size;
      return ERROR_NONE;
    }
  }
  
  return ERROR_NOT_FOUND;
}

error_t binary_parser_validate_header(const coil_header_t* header) {
  if (header == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check magic number */
  if (header->magic != COIL_MAGIC) {
    log_error("Invalid magic number: 0x%08X (expected: 0x%08X)",
             header->magic, COIL_MAGIC);
    return ERROR_INVALID_FORMAT;
  }
  
  /* Check version compatibility 
   * For now, we only check that the major version is compatible
   * Future versions may have more specific compatibility rules
   */
  uint8_t major_version = (header->version >> 24) & 0xFF;
  if (major_version > 1) {
    log_error("Unsupported COIL binary version: %u.%u.%u",
             major_version,
             (header->version >> 16) & 0xFF,
             header->version & 0xFFFF);
    return ERROR_UNSUPPORTED;
  }
  
  /* Check section count */
  if (header->section_count == 0) {
    log_error("Invalid section count: 0");
    return ERROR_INVALID_FORMAT;
  }
  
  if (header->section_count > 255) {
    log_warning("Unusually high section count: %u", header->section_count);
  }
  
  return ERROR_NONE;
}