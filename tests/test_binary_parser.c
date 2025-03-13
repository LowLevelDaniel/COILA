/**
 * @file test_binary_parser.c
 * @brief Tests for the COIL binary parser
 * 
 * This file contains tests for the binary parser module
 * of the COIL assembler.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "../src/core/binary_parser.h"
#include "../src/utils/memory_management.h"

/**
 * @brief Creates a mock COIL binary for testing
 *
 * @param[out] size Pointer to receive the size of the binary
 * @return Pointer to mock binary data, or NULL on failure
 */
static uint8_t* create_mock_binary(size_t* size) {
  if (size == NULL) {
    return NULL;
  }
  
  /* Calculate the size of the binary */
  size_t header_size = sizeof(uint32_t) * 4;  /* magic, version, section_count, flags */
  size_t section_table_size = sizeof(uint32_t) * 3 * 2;  /* 2 sections, each with 3 fields */
  size_t section1_size = 16;  /* Small section 1 */
  size_t section2_size = 32;  /* Small section 2 */
  
  *size = header_size + section_table_size + section1_size + section2_size;
  
  /* Allocate buffer */
  uint8_t* binary = memory_calloc(1, *size);
  if (binary == NULL) {
    return NULL;
  }
  
  /* Fill in header */
  uint32_t offset = 0;
  
  /* Magic number "COIL" */
  binary[offset++] = 'C';
  binary[offset++] = 'O';
  binary[offset++] = 'I';
  binary[offset++] = 'L';
  
  /* Version 1.0.0 */
  binary[offset++] = 0;
  binary[offset++] = 0;
  binary[offset++] = 0;
  binary[offset++] = 1;
  
  /* Section count: 2 */
  binary[offset++] = 2;
  binary[offset++] = 0;
  binary[offset++] = 0;
  binary[offset++] = 0;
  
  /* Flags: 0 */
  binary[offset++] = 0;
  binary[offset++] = 0;
  binary[offset++] = 0;
  binary[offset++] = 0;
  
  /* Section table */
  
  /* Section 1: Type */
  binary[offset++] = COIL_SECTION_TYPE;
  binary[offset++] = 0;
  binary[offset++] = 0;
  binary[offset++] = 0;
  
  /* Section 1: Offset */
  uint32_t section1_offset = header_size + section_table_size;
  binary[offset++] = section1_offset & 0xFF;
  binary[offset++] = (section1_offset >> 8) & 0xFF;
  binary[offset++] = (section1_offset >> 16) & 0xFF;
  binary[offset++] = (section1_offset >> 24) & 0xFF;
  
  /* Section 1: Size */
  binary[offset++] = section1_size & 0xFF;
  binary[offset++] = (section1_size >> 8) & 0xFF;
  binary[offset++] = (section1_size >> 16) & 0xFF;
  binary[offset++] = (section1_size >> 24) & 0xFF;
  
  /* Section 2: Function */
  binary[offset++] = COIL_SECTION_FUNCTION;
  binary[offset++] = 0;
  binary[offset++] = 0;
  binary[offset++] = 0;
  
  /* Section 2: Offset */
  uint32_t section2_offset = section1_offset + section1_size;
  binary[offset++] = section2_offset & 0xFF;
  binary[offset++] = (section2_offset >> 8) & 0xFF;
  binary[offset++] = (section2_offset >> 16) & 0xFF;
  binary[offset++] = (section2_offset >> 24) & 0xFF;
  
  /* Section 2: Size */
  binary[offset++] = section2_size & 0xFF;
  binary[offset++] = (section2_size >> 8) & 0xFF;
  binary[offset++] = (section2_size >> 16) & 0xFF;
  binary[offset++] = (section2_size >> 24) & 0xFF;
  
  /* Section 1 data (dummy data) */
  for (size_t i = 0; i < section1_size; i++) {
    binary[offset++] = i & 0xFF;
  }
  
  /* Section 2 data (dummy data) */
  for (size_t i = 0; i < section2_size; i++) {
    binary[offset++] = (i + 0x10) & 0xFF;
  }
  
  return binary;
}

/**
 * @brief Tests binary parser initialization and cleanup
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
static bool test_binary_parser_init(test_results_t* results) {
  bool success = true;
  
  /* Create mock binary */
  size_t binary_size = 0;
  uint8_t* binary = create_mock_binary(&binary_size);
  
  if (!test_assert(results, binary != NULL, "Create mock binary")) {
    success = false;
    goto cleanup;
  }
  
  /* Test parsing */
  coil_module_t* module = NULL;
  error_t parse_result = binary_parser_parse(binary, binary_size, &module);
  
  if (!test_assert_error(results, parse_result, ERROR_NONE, "Parse binary")) {
    success = false;
    goto cleanup;
  }
  
  if (!test_assert(results, module != NULL, "Module not NULL")) {
    success = false;
    goto cleanup;
  }
  
  /* Test header contents */
  if (!test_assert_int(results, module->header.magic, COIL_MAGIC, "Magic number")) {
    success = false;
  }
  
  if (!test_assert_int(results, module->header.section_count, 2, "Section count")) {
    success = false;
  }
  
  /* Test section table */
  if (!test_assert_int(results, module->section_table[0].section_type, COIL_SECTION_TYPE, "Section 1 type")) {
    success = false;
  }
  
  if (!test_assert_int(results, module->section_table[1].section_type, COIL_SECTION_FUNCTION, "Section 2 type")) {
    success = false;
  }
  
  /* Test section access */
  const uint8_t* section_data = NULL;
  size_t section_size = 0;
  
  error_t section_result = binary_parser_get_section(module, COIL_SECTION_TYPE, &section_data, &section_size);
  
  if (!test_assert_error(results, section_result, ERROR_NONE, "Get section")) {
    success = false;
  }
  
  if (!test_assert_int(results, section_size, 16, "Section size")) {
    success = false;
  }
  
  if (!test_assert(results, section_data != NULL, "Section data not NULL")) {
    success = false;
  }
  
  /* Test header validation */
  error_t validate_result = binary_parser_validate_header(&module->header);
  
  if (!test_assert_error(results, validate_result, ERROR_NONE, "Validate header")) {
    success = false;
  }
  
  /* Test cleanup */
  error_t free_result = binary_parser_free_module(module);
  
  if (!test_assert_error(results, free_result, ERROR_NONE, "Free module")) {
    success = false;
  }
  
cleanup:
  memory_free(binary);
  return success;
}

/**
 * @brief Tests binary parser error handling
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
static bool test_binary_parser_errors(test_results_t* results) {
  bool success = true;
  
  /* Test NULL arguments */
  coil_module_t* module = NULL;
  error_t parse_result = binary_parser_parse(NULL, 100, &module);
  
  if (!test_assert_error(results, parse_result, ERROR_INVALID_ARGUMENT, "NULL binary")) {
    success = false;
  }
  
  parse_result = binary_parser_parse((uint8_t*)"test", 4, NULL);
  
  if (!test_assert_error(results, parse_result, ERROR_INVALID_ARGUMENT, "NULL module")) {
    success = false;
  }
  
  /* Test invalid formats */
  uint8_t invalid_binary[] = { 0x00, 0x01, 0x02, 0x03 };
  parse_result = binary_parser_parse(invalid_binary, sizeof(invalid_binary), &module);
  
  if (!test_assert_error(results, parse_result, ERROR_INVALID_FORMAT, "Invalid magic")) {
    success = false;
  }
  
  /* Test free with NULL */
  error_t free_result = binary_parser_free_module(NULL);
  
  if (!test_assert_error(results, free_result, ERROR_INVALID_ARGUMENT, "Free NULL module")) {
    success = false;
  }
  
  return success;
}

/**
 * @brief Main test function for binary parser
 *
 * @param[in,out] results Test results to update
 * @return true if all tests passed, false otherwise
 */
bool test_binary_parser(test_results_t* results) {
  bool success = true;
  
  /* Initialize memory management */
  memory_init(true);
  
  /* Run sub-tests */
  if (!test_binary_parser_init(results)) {
    success = false;
  }
  
  if (!test_binary_parser_errors(results)) {
    success = false;
  }
  
  /* Cleanup memory management */
  memory_shutdown();
  
  return success;
}