/**
 * @file src/core/binary_parser.h
 *
 * @brief COIL binary format parser
 *
 * Parses COIL binary format, validates format correctness,
 * extracts sections and modules, and handles version compatibility.
 */

#ifndef BINARY_PARSER_H
#define BINARY_PARSER_H

#include "utils/sstream.h"
#include "core/ir/module.h"

/**
 * @brief Parse a COIL binary module from a stream
 *
 * @param stream The input stream containing COIL binary data
 * @param[out] error Error code if parsing fails
 * @return module_t* Parsed module or NULL on error
 */
module_t* parse_coil_binary(stream_t* stream, int* error);

/* Additional functions... */

#endif /* BINARY_PARSER_H */