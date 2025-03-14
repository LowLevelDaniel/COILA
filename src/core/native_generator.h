/**
 * @file src/core/native_generator.h
 *
 * @brief Native binary generation
 *
 * Generates native binary format, handles relocation entries,
 * creates symbol tables, and produces debug information.
 */

#ifndef NATIVE_GENERATOR_H
#define NATIVE_GENERATOR_H

#include "core/ir/module.h"
#include "core/config.h"

/**
 * @brief Output format type
 */
typedef enum {
  OUTPUT_ELF,
  OUTPUT_PE,
  OUTPUT_MACHO,
  OUTPUT_RAW
} output_format_t;

/**
 * @brief Generate native binary
 *
 * @param module The processed module
 * @param target Target configuration
 * @param format Output format
 * @param output_path Output file path
 * @return int 0 on success, error code otherwise
 */
int generate_native_binary(module_t* module, 
                         target_config_t* target,
                         output_format_t format,
                         const char* output_path);

/* Additional functions... */

#endif /* NATIVE_GENERATOR_H */