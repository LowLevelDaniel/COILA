/**
 * @file native_generator.c
 * @brief Implementation of native binary generation interface
 * 
 * This module implements the interface for generating native binary output
 * from native instructions for specific target architectures.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "native_generator.h"
#include "../utils/memory_management.h"
#include "../utils/logging.h"

/**
 * @brief Maximum number of sections in a native binary
 */
#define MAX_SECTIONS 32

/**
 * @brief Maximum number of symbols in a native binary
 */
#define MAX_SYMBOLS 1024

/**
 * @brief Maximum number of relocations in a native binary
 */
#define MAX_RELOCATIONS 1024

/**
 * @brief Native binary section
 */
typedef struct {
  native_section_t info;        /**< Section information */
  size_t index;                 /**< Section index */
} native_section_entry_t;

/**
 * @brief Native binary symbol
 */
typedef struct {
  native_symbol_t info;         /**< Symbol information */
  size_t index;                 /**< Symbol index */
} native_symbol_entry_t;

/**
 * @brief Native generator implementation
 */
struct native_generator_t {
  const target_config_t* target;        /**< Target configuration */
  native_format_t format;               /**< Output format */
  native_section_entry_t* sections;     /**< Sections array */
  size_t section_count;                 /**< Number of sections */
  native_symbol_entry_t* symbols;       /**< Symbols array */
  size_t symbol_count;                  /**< Number of symbols */
  native_relocation_t* relocations;     /**< Relocations array */
  size_t relocation_count;              /**< Number of relocations */
  uint32_t entry_point_symbol;          /**< Entry point symbol index */
  void* arch_data;                      /**< Architecture-specific data */
};

error_t native_generator_create(
  const target_config_t* target,
  native_format_t format,
  native_generator_t** generator
) {
  if (target == NULL || generator == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *generator = NULL;
  
  /* Allocate generator structure */
  native_generator_t* new_generator = memory_calloc(1, sizeof(native_generator_t));
  if (new_generator == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Initialize generator fields */
  new_generator->target = target;
  new_generator->format = format;
  
  /* Allocate sections array */
  new_generator->sections = memory_calloc(MAX_SECTIONS, sizeof(native_section_entry_t));
  if (new_generator->sections == NULL) {
    memory_free(new_generator);
    return ERROR_MEMORY;
  }
  new_generator->section_count = 0;
  
  /* Allocate symbols array */
  new_generator->symbols = memory_calloc(MAX_SYMBOLS, sizeof(native_symbol_entry_t));
  if (new_generator->symbols == NULL) {
    memory_free(new_generator->sections);
    memory_free(new_generator);
    return ERROR_MEMORY;
  }
  new_generator->symbol_count = 0;
  
  /* Allocate relocations array */
  new_generator->relocations = memory_calloc(MAX_RELOCATIONS, sizeof(native_relocation_t));
  if (new_generator->relocations == NULL) {
    memory_free(new_generator->symbols);
    memory_free(new_generator->sections);
    memory_free(new_generator);
    return ERROR_MEMORY;
  }
  new_generator->relocation_count = 0;
  
  /* Initialize architecture-specific generator */
  const char* arch = target->architecture;
  
  if (strncmp(arch, "x86", 3) == 0) {
    /* Initialize x86-specific generator */
    /* This would typically be handled by a function in x86_generator.c */
    extern error_t x86_generator_init(native_generator_t* generator, const target_config_t* target);
    error_t init_result = x86_generator_init(new_generator, target);
    if (init_result != ERROR_NONE) {
      memory_free(new_generator->relocations);
      memory_free(new_generator->symbols);
      memory_free(new_generator->sections);
      memory_free(new_generator);
      return init_result;
    }
  } else if (strncmp(arch, "arm", 3) == 0 || strncmp(arch, "aarch", 5) == 0) {
    /* Initialize ARM-specific generator */
    /* This would typically be handled by a function in arm_generator.c */
    log_error("ARM generator not yet implemented");
    memory_free(new_generator->relocations);
    memory_free(new_generator->symbols);
    memory_free(new_generator->sections);
    memory_free(new_generator);
    return ERROR_UNSUPPORTED;
  } else {
    /* Unsupported architecture */
    log_error("Unsupported architecture: %s", arch);
    memory_free(new_generator->relocations);
    memory_free(new_generator->symbols);
    memory_free(new_generator->sections);
    memory_free(new_generator);
    return ERROR_UNSUPPORTED;
  }
  
  *generator = new_generator;
  return ERROR_NONE;
}

error_t native_generator_destroy(native_generator_t* generator) {
  if (generator == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Cleanup architecture-specific generator */
  const char* arch = generator->target->architecture;
  
  if (strncmp(arch, "x86", 3) == 0) {
    /* Cleanup x86-specific generator */
    /* This would typically be handled by a function in x86_generator.c */
    extern error_t x86_generator_cleanup(native_generator_t* generator);
    error_t cleanup_result = x86_generator_cleanup(generator);
    if (cleanup_result != ERROR_NONE) {
      log_warning("Failed to cleanup x86 generator: %s", error_message(cleanup_result));
    }
  } else if (strncmp(arch, "arm", 3) == 0 || strncmp(arch, "aarch", 5) == 0) {
    /* Cleanup ARM-specific generator */
    /* Not implemented yet */
  }
  
  /* Free section data */
  for (size_t i = 0; i < generator->section_count; i++) {
    memory_free(generator->sections[i].info.name);
    memory_free(generator->sections[i].info.data);
    memory_free(generator->sections[i].info.relocations);
  }
  
  /* Free symbol names */
  for (size_t i = 0; i < generator->symbol_count; i++) {
    memory_free(generator->symbols[i].info.name);
  }
  
  /* Free arrays */
  memory_free(generator->relocations);
  memory_free(generator->symbols);
  memory_free(generator->sections);
  
  /* Free generator structure */
  memory_free(generator);
  
  return ERROR_NONE;
}

error_t native_generator_add_section(
  native_generator_t* generator,
  const char* name,
  native_section_type_t type,
  uint32_t flags,
  uint16_t* section_index
) {
  if (generator == NULL || name == NULL || section_index == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if we've reached the maximum number of sections */
  if (generator->section_count >= MAX_SECTIONS) {
    log_error("Maximum number of sections (%u) exceeded", MAX_SECTIONS);
    return ERROR_GENERATION;
  }
  
  /* Check if section already exists */
  for (size_t i = 0; i < generator->section_count; i++) {
    if (strcmp(generator->sections[i].info.name, name) == 0) {
      *section_index = i;
      return ERROR_NONE;
    }
  }
  
  /* Create new section */
  size_t index = generator->section_count;
  native_section_entry_t* section = &generator->sections[index];
  
  /* Initialize section information */
  section->info.name = memory_alloc(strlen(name) + 1);
  if (section->info.name == NULL) {
    return ERROR_MEMORY;
  }
  strcpy(section->info.name, name);
  
  section->info.type = type;
  section->info.flags = flags;
  section->info.data = NULL;
  section->info.size = 0;
  section->info.alignment = 1;
  section->info.relocations = NULL;
  section->info.relocation_count = 0;
  section->index = index;
  
  /* Update section count */
  generator->section_count++;
  
  *section_index = index;
  return ERROR_NONE;
}

error_t native_generator_add_section_data(
  native_generator_t* generator,
  uint16_t section_index,
  const uint8_t* data,
  size_t size,
  size_t alignment,
  uint64_t* offset
) {
  if (generator == NULL || data == NULL || offset == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if section index is valid */
  if (section_index >= generator->section_count) {
    log_error("Invalid section index: %u", section_index);
    return ERROR_INVALID_ARGUMENT;
  }
  
  native_section_entry_t* section = &generator->sections[section_index];
  
  /* Calculate aligned size of current data */
  size_t aligned_size = 0;
  if (section->info.size > 0) {
    aligned_size = (section->info.size + alignment - 1) & ~(alignment - 1);
  }
  
  /* Allocate or reallocate section data */
  uint8_t* new_data = NULL;
  if (section->info.data == NULL) {
    new_data = memory_alloc(aligned_size + size);
    if (new_data == NULL) {
      return ERROR_MEMORY;
    }
  } else {
    new_data = memory_realloc(section->info.data, aligned_size + size);
    if (new_data == NULL) {
      return ERROR_MEMORY;
    }
    
    /* Zero padding bytes */
    if (aligned_size > section->info.size) {
      memset(new_data + section->info.size, 0, aligned_size - section->info.size);
    }
  }
  
  /* Copy new data */
  memcpy(new_data + aligned_size, data, size);
  
  /* Update section information */
  section->info.data = new_data;
  *offset = aligned_size;
  section->info.size = aligned_size + size;
  
  if (alignment > section->info.alignment) {
    section->info.alignment = alignment;
  }
  
  return ERROR_NONE;
}

error_t native_generator_add_symbol(
  native_generator_t* generator,
  const char* name,
  uint64_t value,
  uint64_t size,
  native_symbol_type_t type,
  native_symbol_binding_t binding,
  uint16_t section_index,
  uint32_t* symbol_index
) {
  if (generator == NULL || name == NULL || symbol_index == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if we've reached the maximum number of symbols */
  if (generator->symbol_count >= MAX_SYMBOLS) {
    log_error("Maximum number of symbols (%u) exceeded", MAX_SYMBOLS);
    return ERROR_GENERATION;
  }
  
  /* Check if symbol already exists */
  for (size_t i = 0; i < generator->symbol_count; i++) {
    if (strcmp(generator->symbols[i].info.name, name) == 0) {
      *symbol_index = i;
      return ERROR_NONE;
    }
  }
  
  /* Check if section index is valid */
  if (section_index >= generator->section_count) {
    log_error("Invalid section index: %u", section_index);
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Create new symbol */
  size_t index = generator->symbol_count;
  native_symbol_entry_t* symbol = &generator->symbols[index];
  
  /* Initialize symbol information */
  symbol->info.name = memory_alloc(strlen(name) + 1);
  if (symbol->info.name == NULL) {
    return ERROR_MEMORY;
  }
  strcpy(symbol->info.name, name);
  
  symbol->info.value = value;
  symbol->info.size = size;
  symbol->info.type = type;
  symbol->info.binding = binding;
  symbol->info.section_index = section_index;
  symbol->index = index;
  
  /* Update symbol count */
  generator->symbol_count++;
  
  *symbol_index = index;
  return ERROR_NONE;
}

error_t native_generator_add_relocation(
  native_generator_t* generator,
  uint16_t section_index,
  uint64_t offset,
  uint64_t addend,
  native_reloc_type_t type,
  uint32_t symbol_index
) {
  if (generator == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if section index is valid */
  if (section_index >= generator->section_count) {
    log_error("Invalid section index: %u", section_index);
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if symbol index is valid */
  if (symbol_index >= generator->symbol_count) {
    log_error("Invalid symbol index: %u", symbol_index);
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if we've reached the maximum number of relocations */
  if (generator->relocation_count >= MAX_RELOCATIONS) {
    log_error("Maximum number of relocations (%u) exceeded", MAX_RELOCATIONS);
    return ERROR_GENERATION;
  }
  
  /* Add relocation to global array */
  native_relocation_t* reloc = &generator->relocations[generator->relocation_count++];
  reloc->offset = offset;
  reloc->addend = addend;
  reloc->type = type;
  reloc->symbol_index = symbol_index;
  
  /* Add relocation to section */
  native_section_entry_t* section = &generator->sections[section_index];
  
  if (section->info.relocations == NULL) {
    section->info.relocations = memory_calloc(1, sizeof(native_relocation_t));
    if (section->info.relocations == NULL) {
      generator->relocation_count--;
      return ERROR_MEMORY;
    }
  } else {
    native_relocation_t* new_relocs = memory_realloc(
      section->info.relocations,
      (section->info.relocation_count + 1) * sizeof(native_relocation_t)
    );
    if (new_relocs == NULL) {
      generator->relocation_count--;
      return ERROR_MEMORY;
    }
    section->info.relocations = new_relocs;
  }
  
  /* Copy relocation to section */
  memcpy(
    &section->info.relocations[section->info.relocation_count++],
    reloc,
    sizeof(native_relocation_t)
  );
  
  return ERROR_NONE;
}

error_t native_generator_set_entry_point(
  native_generator_t* generator,
  uint32_t symbol_index
) {
  if (generator == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if symbol index is valid */
  if (symbol_index >= generator->symbol_count) {
    log_error("Invalid symbol index: %u", symbol_index);
    return ERROR_INVALID_ARGUMENT;
  }
  
  generator->entry_point_symbol = symbol_index;
  return ERROR_NONE;
}

error_t native_generator_add_function(
  native_generator_t* generator,
  const char* function_name,
  const native_instruction_list_t* instructions,
  uint32_t* symbol_index
) {
  if (generator == NULL || function_name == NULL || instructions == NULL || symbol_index == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Architecture-specific function processing */
  const char* arch = generator->target->architecture;
  
  if (strncmp(arch, "x86", 3) == 0) {
    /* Use x86-specific function processing */
    extern error_t x86_process_function(
      native_generator_t* generator,
      const char* function_name,
      const native_instruction_list_t* instructions,
      uint32_t* symbol_index
    );
    return x86_process_function(generator, function_name, instructions, symbol_index);
  } else if (strncmp(arch, "arm", 3) == 0 || strncmp(arch, "aarch", 5) == 0) {
    /* Use ARM-specific function processing */
    /* Not implemented yet */
    log_error("ARM function processing not yet implemented");
    return ERROR_UNSUPPORTED;
  } else {
    /* Unsupported architecture */
    log_error("Unsupported architecture for function processing: %s", arch);
    return ERROR_UNSUPPORTED;
  }
}

error_t native_generator_generate(
  native_generator_t* generator,
  uint8_t** binary,
  size_t* size
) {
  if (generator == NULL || binary == NULL || size == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *binary = NULL;
  *size = 0;
  
  /* Architecture-specific binary generation */
  const char* arch = generator->target->architecture;
  
  if (strncmp(arch, "x86", 3) == 0) {
    /* Use x86-specific binary generation */
    switch (generator->format) {
      case NATIVE_FORMAT_ELF:
        /* Generate ELF binary */
        extern error_t x86_generate_elf_binary(
          native_generator_t* generator,
          uint8_t** binary,
          size_t* size
        );
        return x86_generate_elf_binary(generator, binary, size);
      
      case NATIVE_FORMAT_COFF:
        /* Generate COFF binary */
        log_error("COFF format not yet implemented for x86");
        return ERROR_UNSUPPORTED;
      
      case NATIVE_FORMAT_MACHO:
        /* Generate Mach-O binary */
        log_error("Mach-O format not yet implemented for x86");
        return ERROR_UNSUPPORTED;
      
      case NATIVE_FORMAT_RAW:
        /* Generate raw binary */
        log_error("Raw format not yet implemented for x86");
        return ERROR_UNSUPPORTED;
      
      default:
        log_error("Unknown output format: %d", generator->format);
        return ERROR_INVALID_ARGUMENT;
    }
  } else if (strncmp(arch, "arm", 3) == 0 || strncmp(arch, "aarch", 5) == 0) {
    /* Use ARM-specific binary generation */
    /* Not implemented yet */
    log_error("ARM binary generation not yet implemented");
    return ERROR_UNSUPPORTED;
  } else {
    /* Unsupported architecture */
    log_error("Unsupported architecture for binary generation: %s", arch);
    return ERROR_UNSUPPORTED;
  }
}

error_t native_generator_write_file(
  native_generator_t* generator,
  const char* filename
) {
  if (generator == NULL || filename == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Generate binary */
  uint8_t* binary = NULL;
  size_t size = 0;
  
  error_t generate_result = native_generator_generate(generator, &binary, &size);
  if (generate_result != ERROR_NONE) {
    return generate_result;
  }
  
  /* Write binary to file */
  FILE* file = fopen(filename, "wb");
  if (file == NULL) {
    log_error("Failed to open output file: %s", filename);
    memory_free(binary);
    return ERROR_FILE_IO;
  }
  
  size_t written = fwrite(binary, 1, size, file);
  if (written != size) {
    log_error("Failed to write output file: %s", filename);
    fclose(file);
    memory_free(binary);
    return ERROR_FILE_IO;
  }
  
  fclose(file);
  memory_free(binary);
  
  log_info("Successfully wrote %zu bytes to %s", size, filename);
  
  return ERROR_NONE;
}

error_t native_generator_get_section_by_name(
  const native_generator_t* generator,
  const char* name,
  uint16_t* section_index
) {
  if (generator == NULL || name == NULL || section_index == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Search for section by name */
  for (size_t i = 0; i < generator->section_count; i++) {
    if (strcmp(generator->sections[i].info.name, name) == 0) {
      *section_index = i;
      return ERROR_NONE;
    }
  }
  
  return ERROR_NOT_FOUND;
}

error_t native_generator_get_symbol_by_name(
  const native_generator_t* generator,
  const char* name,
  uint32_t* symbol_index
) {
  if (generator == NULL || name == NULL || symbol_index == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Search for symbol by name */
  for (size_t i = 0; i < generator->symbol_count; i++) {
    if (strcmp(generator->symbols[i].info.name, name) == 0) {
      *symbol_index = i;
      return ERROR_NONE;
    }
  }
  
  return ERROR_NOT_FOUND;
}

error_t native_generator_set_arch_data(
  native_generator_t* generator,
  void* arch_data
) {
  if (generator == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  generator->arch_data = arch_data;
  return ERROR_NONE;
}

void* native_generator_get_arch_data(const native_generator_t* generator) {
  if (generator == NULL) {
    return NULL;
  }
  
  return generator->arch_data;
}