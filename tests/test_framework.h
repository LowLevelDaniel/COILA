/**
 * @file test_framework.h
 * @brief Simple test framework for the COIL assembler
 * 
 * This module provides a simple test framework for unit testing
 * the COIL assembler components.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../src/utils/error_handling.h"

/**
 * @brief Test result structure
 */
typedef struct {
  int total;       /**< Total number of tests */
  int passed;      /**< Number of passed tests */
  int failed;      /**< Number of failed tests */
} test_results_t;

/**
 * @brief Initializes test results
 *
 * @param[out] results Test results to initialize
 */
static inline void test_init(test_results_t* results) {
  if (results != NULL) {
    results->total = 0;
    results->passed = 0;
    results->failed = 0;
  }
}

/**
 * @brief Tests a condition and updates results
 *
 * @param[in,out] results Test results to update
 * @param[in] condition Condition to test
 * @param[in] name Test name
 * @return true if the test passed, false otherwise
 */
static inline bool test_assert(test_results_t* results, bool condition, const char* name) {
  if (results != NULL) {
    results->total++;
    
    if (condition) {
      results->passed++;
      printf("[ PASS ] %s\n", name);
      return true;
    } else {
      results->failed++;
      printf("[ FAIL ] %s\n", name);
      return false;
    }
  }
  
  return false;
}

/**
 * @brief Asserts that a value is equal to an expected value
 *
 * @param[in,out] results Test results to update
 * @param[in] actual Actual value
 * @param[in] expected Expected value
 * @param[in] name Test name
 * @return true if the test passed, false otherwise
 */
static inline bool test_assert_int(test_results_t* results, int actual, int expected, const char* name) {
  bool condition = (actual == expected);
  
  if (!condition) {
    printf("        Expected: %d, Actual: %d\n", expected, actual);
  }
  
  return test_assert(results, condition, name);
}

/**
 * @brief Asserts that a string is equal to an expected string
 *
 * @param[in,out] results Test results to update
 * @param[in] actual Actual string
 * @param[in] expected Expected string
 * @param[in] name Test name
 * @return true if the test passed, false otherwise
 */
static inline bool test_assert_string(test_results_t* results, const char* actual, const char* expected, const char* name) {
  bool condition = (strcmp(actual, expected) == 0);
  
  if (!condition) {
    printf("        Expected: \"%s\", Actual: \"%s\"\n", expected, actual);
  }
  
  return test_assert(results, condition, name);
}

/**
 * @brief Asserts that an error code is equal to an expected error code
 *
 * @param[in,out] results Test results to update
 * @param[in] actual Actual error code
 * @param[in] expected Expected error code
 * @param[in] name Test name
 * @return true if the test passed, false otherwise
 */
static inline bool test_assert_error(test_results_t* results, error_t actual, error_t expected, const char* name) {
  bool condition = (actual == expected);
  
  if (!condition) {
    printf("        Expected: %s, Actual: %s\n", 
           error_message(expected), error_message(actual));
  }
  
  return test_assert(results, condition, name);
}

/**
 * @brief Prints test results
 *
 * @param[in] results Test results to print
 */
static inline void test_print_results(const test_results_t* results) {
  if (results != NULL) {
    printf("\nTest Results:\n");
    printf("  Total:  %d\n", results->total);
    printf("  Passed: %d\n", results->passed);
    printf("  Failed: %d\n", results->failed);
    
    if (results->failed == 0) {
      printf("\nAll tests passed!\n");
    } else {
      printf("\n%d tests failed.\n", results->failed);
    }
  }
}

/**
 * @brief Runs a test function
 *
 * @param[in] test_func Test function to run
 * @param[in] name Test name
 * @param[in,out] results Test results to update
 */
#define RUN_TEST(test_func, name, results) \
  do { \
    printf("\nRunning test: %s\n", name); \
    bool test_result = test_func(results); \
    if (test_result) { \
      printf("Test %s completed successfully.\n", name); \
    } else { \
      printf("Test %s failed.\n", name); \
    } \
  } while (0)

#endif /* TEST_FRAMEWORK_H */