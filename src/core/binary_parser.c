/**
 * @file binary_parser.c
 * @brief COIL binary parser implementation
 * @details Implementation of the binary parser component for the COIL assembler.
 *          Provides functionality for loading, parsing, and validating COIL binary modules.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "coil/binary.h"
#include "../utils/memory.c"

/**
 * @brief Creates an empty COIL module
 * @return Pointer to a new COIL module or NULL on failure
 */
coil_module_t* coil_module_create(void) {
  // Allocate memory for the module
  coil_module_t* module = (coil_module_t*)coil_calloc(1, sizeof(coil_module_t));
  if (!module) {
    return NULL;
  }
  
  // Initialize header with default values
  module->header.magic = COIL_MAGIC;
  module->header.version_major = COIL_VERSION_MAJOR;
  module->header.version_minor = COIL_VERSION_MINOR;
  module->header.version_patch = COIL_VERSION_PATCH;
  module->header.section_count = 0;
  module->header.flags = 0;
  
  // Initialize section tables to NULL (will be allocated as needed)
  module->sections = NULL;
  module->section_data = NULL;
  
  return module;
}

/**
 * @brief Destroys a COIL module and frees all associated memory
 * @param module Pointer to the module to destroy
 */
void coil_module_destroy(coil_module_t *module) {
  if (!module) {
    return;
  }
  
  // Free section data if allocated
  if (module->section_data) {
    for (uint32_t i = 0; i < module->header.section_count; i++) {
      if (module->section_data[i]) {
        coil_free(module->section_data[i], module->sections[i].size);
      }
    }
    coil_free(module->section_data, module->header.section_count * sizeof(uint8_t*));
  }
  
  // Free section table if allocated
  if (module->sections) {
    coil_free(module->sections, module->header.section_count * sizeof(coil_section_entry_t));
  }
  
  // Free the module itself
  coil_free(module, sizeof(coil_module_t));
}

/**
 * @brief Reads a file into memory
 * @param filename Path to the file to read
 * @param size Pointer to store the size of the file
 * @return Pointer to the file contents or NULL on failure
 */
static uint8_t* read_file(const char *filename, size_t *size) {
  FILE *file;
  uint8_t *buffer;
  long file_size;
  
  // Open the file
  file = fopen(filename, "rb");
  if (!file) {
    return NULL;
  }
  
  // Get file size
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  if (file_size <= 0) {
    fclose(file);
    return NULL;
  }
  
  // Allocate buffer
  buffer = (uint8_t*)coil_malloc(file_size);
  if (!buffer) {
    fclose(file);
    return NULL;
  }
  
  // Read the file
  if (fread(buffer, 1, file_size, file) != (size_t)file_size) {
    coil_free(buffer, file_size);
    fclose(file);
    return NULL;
  }
  
  fclose(file);
  *size = (size_t)file_size;
  return buffer;
}

/**
 * @brief Loads a COIL module from a file
 * @param filename Path to the COIL binary file
 * @return Pointer to the loaded module or NULL on failure
 */
coil_module_t* coil_module_load_from_file(const char *filename) {
  uint8_t *data;
  size_t size;
  coil_module_t *module;
  
  // Read the file into memory
  data = read_file(filename, &size);
  if (!data) {
    return NULL;
  }
  
  // Load the module from memory
  module = coil_module_load_from_memory(data, size);
  
  // Free the file data
  coil_free(data, size);
  
  return module;
}

/**
 * @brief Loads a COIL module from memory
 * @param data Pointer to the COIL binary data
 * @param size Size of the data in bytes
 * @return Pointer to the loaded module or NULL on failure
 */
coil_module_t* coil_module_load_from_memory(const uint8_t *data, size_t size) {
  coil_module_t *module;
  const coil_module_header_t *header;
  const coil_section_entry_t *section_table;
  size_t min_size;
  
  // Validate input
  if (!data || size < sizeof(coil_module_header_t)) {
    return NULL;
  }
  
  // Check minimum size needed for header
  header = (const coil_module_header_t*)data;
  min_size = sizeof(coil_module_header_t) + 
             header->section_count * sizeof(coil_section_entry_t);
  
  if (size < min_size) {
    return NULL;
  }
  
  // Validate magic number
  if (header->magic != COIL_MAGIC) {
    return NULL;
  }
  
  // Create a new module
  module = coil_module_create();
  if (!module) {
    return NULL;
  }
  
  // Copy header
  memcpy(&module->header, header, sizeof(coil_module_header_t));
  
  // Allocate section table and data pointers
  if (header->section_count > 0) {
    module->sections = (coil_section_entry_t*)coil_calloc(
        header->section_count, sizeof(coil_section_entry_t));
    
    module->section_data = (uint8_t**)coil_calloc(
        header->section_count, sizeof(uint8_t*));
    
    if (!module->sections || !module->section_data) {
      coil_module_destroy(module);
      return NULL;
    }
    
    // Copy section table
    section_table = (const coil_section_entry_t*)(data + sizeof(coil_module_header_t));
    memcpy(module->sections, section_table, 
           header->section_count * sizeof(coil_section_entry_t));
    
    // Copy section data
    for (uint32_t i = 0; i < header->section_count; i++) {
      const coil_section_entry_t *entry = &module->sections[i];
      
      // Validate section offset and size
      if (entry->offset + entry->size > size) {
        coil_module_destroy(module);
        return NULL;
      }
      
      // Allocate and copy section data
      if (entry->size > 0) {
        module->section_data[i] = (uint8_t*)coil_malloc(entry->size);
        if (!module->section_data[i]) {
          coil_module_destroy(module);
          return NULL;
        }
        
        memcpy(module->section_data[i], data + entry->offset, entry->size);
      }
    }
  }
  
  return module;
}

/**
 * @brief Writes a COIL module to a file
 * @param module Pointer to the module to write
 * @param filename Path to the output file
 * @return 0 on success, non-zero on failure
 */
int coil_module_write_to_file(const coil_module_t *module, const char *filename) {
  FILE *file;
  uint32_t current_offset;
  
  // Validate input
  if (!module || !filename) {
    return -1;
  }
  
  // Open the file
  file = fopen(filename, "wb");
  if (!file) {
    return -1;
  }
  
  // Write header
  if (fwrite(&module->header, sizeof(coil_module_header_t), 1, file) != 1) {
    fclose(file);
    return -1;
  }
  
  // Write section table if sections exist
  if (module->header.section_count > 0 && module->sections) {
    if (fwrite(module->sections, sizeof(coil_section_entry_t), 
              module->header.section_count, file) != module->header.section_count) {
      fclose(file);
      return -1;
    }
  }
  
  // Calculate initial offset (header + section table)
  current_offset = sizeof(coil_module_header_t) + 
                  module->header.section_count * sizeof(coil_section_entry_t);
  
  // Write section data if sections exist
  if (module->header.section_count > 0 && module->sections && module->section_data) {
    for (uint32_t i = 0; i < module->header.section_count; i++) {
      const coil_section_entry_t *entry = &module->sections[i];
      
      // Pad to section offset if necessary
      while (current_offset < entry->offset) {
        if (fputc(0, file) == EOF) {
          fclose(file);
          return -1;
        }
        current_offset++;
      }
      
      // Write section data if it exists
      if (entry->size > 0 && module->section_data[i]) {
        if (fwrite(module->section_data[i], 1, entry->size, file) != entry->size) {
          fclose(file);
          return -1;
        }
        
        current_offset += entry->size;
      }
    }
  }
  
  fclose(file);
  return 0;
}

/**
 * @brief Gets a section from a COIL module
 * @param module Pointer to the module
 * @param type Type of section to get
 * @return Pointer to the section data or NULL if not found
 */
const uint8_t* coil_module_get_section(const coil_module_t *module, coil_section_type_t type) {
  if (!module || !module->sections || !module->section_data) {
    return NULL;
  }
  
  // Search for section with matching type
  for (uint32_t i = 0; i < module->header.section_count; i++) {
    if (module->sections[i].section_type == type) {
      return module->section_data[i];
    }
  }
  
  // Section not found
  return NULL;
}

/**
 * @brief Validates a COIL module
 * @param module Pointer to the module to validate
 * @return 0 if valid, non-zero if invalid
 */
int coil_module_validate(const coil_module_t *module) {
  if (!module) {
    return -1;
  }
  
  // Check magic number
  if (module->header.magic != COIL_MAGIC) {
    return -1;
  }
  
  // Validate header fields
  if (module->header.section_count > 0 && 
      (!module->sections || !module->section_data)) {
    return -1;
  }
  
  // Validate sections
  for (uint32_t i = 0; i < module->header.section_count; i++) {
    const coil_section_entry_t *entry = &module->sections[i];
    
    // Check section type (must not be unknown)
    if (entry->section_type == COIL_SECTION_TYPE_UNKNOWN) {
      return -1;
    }
    
    // Check section data exists if size > 0
    if (entry->size > 0 && !module->section_data[i]) {
      return -1;
    }
    
    // Check for overlapping sections
    for (uint32_t j = i + 1; j < module->header.section_count; j++) {
      const coil_section_entry_t *other = &module->sections[j];
      
      // Check if sections overlap
      if (!(entry->offset + entry->size <= other->offset || 
            other->offset + other->size <= entry->offset)) {
        return -1;
      }
    }
  }
  
  return 0;
}

/**
 * @brief Add a section to a COIL module
 * @param module Pointer to the module
 * @param type Type of section to add
 * @param data Section data
 * @param size Size of section data in bytes
 * @return 0 on success, non-zero on failure
 */
int coil_module_add_section(coil_module_t *module, coil_section_type_t type, 
                           const uint8_t *data, uint32_t size) {
  uint32_t section_index;
  coil_section_entry_t *new_sections;
  uint8_t **new_section_data;
  uint8_t *section_data_copy = NULL;
  uint32_t offset;
  
  // Validate input
  if (!module || (size > 0 && !data)) {
    return -1;
  }
  
  // Check if section already exists
  for (uint32_t i = 0; i < module->header.section_count; i++) {
    if (module->sections[i].section_type == type) {
      // Section already exists
      return -1;
    }
  }
  
  // Calculate new section index
  section_index = module->header.section_count;
  
  // Resize section table
  if (section_index == 0) {
    new_sections = (coil_section_entry_t*)coil_malloc(sizeof(coil_section_entry_t));
  } else {
    new_sections = (coil_section_entry_t*)coil_realloc(
        module->sections, 
        section_index * sizeof(coil_section_entry_t),
        (section_index + 1) * sizeof(coil_section_entry_t));
  }
  
  if (!new_sections) {
    return -1;
  }
  
  module->sections = new_sections;
  
  // Resize section data array
  if (section_index == 0) {
    new_section_data = (uint8_t**)coil_malloc(sizeof(uint8_t*));
  } else {
    new_section_data = (uint8_t**)coil_realloc(
        module->section_data,
        section_index * sizeof(uint8_t*),
        (section_index + 1) * sizeof(uint8_t*));
  }
  
  if (!new_section_data) {
    // Revert section table resize
    if (section_index == 0) {
      coil_free(module->sections, sizeof(coil_section_entry_t));
      module->sections = NULL;
    } else {
      module->sections = (coil_section_entry_t*)coil_realloc(
          module->sections,
          (section_index + 1) * sizeof(coil_section_entry_t),
          section_index * sizeof(coil_section_entry_t));
    }
    return -1;
  }
  
  module->section_data = new_section_data;
  
  // Copy section data
  if (size > 0) {
    section_data_copy = (uint8_t*)coil_malloc(size);
    if (!section_data_copy) {
      // Revert resizing
      if (section_index == 0) {
        coil_free(module->sections, sizeof(coil_section_entry_t));
        coil_free(module->section_data, sizeof(uint8_t*));
        module->sections = NULL;
        module->section_data = NULL;
      } else {
        module->sections = (coil_section_entry_t*)coil_realloc(
            module->sections,
            (section_index + 1) * sizeof(coil_section_entry_t),
            section_index * sizeof(coil_section_entry_t));
        module->section_data = (uint8_t**)coil_realloc(
            module->section_data,
            (section_index + 1) * sizeof(uint8_t*),
            section_index * sizeof(uint8_t*));
      }
      return -1;
    }
    
    memcpy(section_data_copy, data, size);
  }
  
  module->section_data[section_index] = section_data_copy;
  
  // Calculate section offset (after header and section table)
  offset = sizeof(coil_module_header_t) + 
          (section_index + 1) * sizeof(coil_section_entry_t);
  
  // Align offset to 4-byte boundary
  offset = (offset + 3) & ~3;
  
  // Add section to table
  module->sections[section_index].section_type = type;
  module->sections[section_index].offset = offset;
  module->sections[section_index].size = size;
  
  // Increment section count
  module->header.section_count++;
  
  return 0;
}