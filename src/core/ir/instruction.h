/**
 * @file src/core/instruction_decoder.h
 *
 * @brief COIL instruction decoder
 *
 * Decodes COIL instructions, validates instruction semantics,
 * builds internal instruction representation, and analyzes
 * instruction dependencies.
 */

#ifndef INSTRUCTION_DECODER_H
#define INSTRUCTION_DECODER_H

#include "core/ir/instruction.h"

/**
 * @brief Decode a COIL instruction from binary data
 *
 * @param binary_data Pointer to binary instruction data
 * @param size Size of binary data
 * @param[out] error Error code if decoding fails
 * @return instruction_t* Decoded instruction or NULL on error
 */
instruction_t* decode_instruction(const uint8_t* binary_data, 
                                size_t size, 
                                int* error);

/* Additional functions... */

#endif /* INSTRUCTION_DECODER_H */