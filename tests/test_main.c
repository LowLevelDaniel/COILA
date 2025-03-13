/**
 * @file test_main.c
 * @brief Main entry point for COIL assembler tests
 * 
 * This file contains the main function for running all
 * COIL assembler test suites.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include "test_framework.h"

/* Forward declarations of test functions */
bool test_binary_parser(test_results_t* results);
bool test_x86_translator(test_results_t* results);
bool test_optimization(test_results_t* results);

/**
 * @brief Main function
 *
 * @param[in] argc Argument count
 * @param[in] argv Argument values
 * @return 0 on success, non-zero on failure
 */
int main(int argc, char* argv[]) {
  printf("=== COIL Assembler Tests ===\n\n");
  
  /* Initialize test results */
  test_results_t results;
  test_init(&results);
  
  /* Run test suites */
  RUN_TEST(test_binary_parser, "Binary Parser", &results);
  RUN_TEST(test_x86_translator, "x86 Translator", &results);
  RUN_TEST(test_optimization, "Optimization Framework", &results);
  
  /* Print overall results */
  test_print_results(&results);
  
  /* Return success only if all tests passed */
  return (results.failed == 0) ? 0 : 1;
}