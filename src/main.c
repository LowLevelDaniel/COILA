/**
 * @file src/main.c
 *
 * @brief LLT COIL Assembler Frontend
 *
 * Main entry point for the COIL assembler.
 */

#include <stdio.h>
#include <stdlib.h>
#include "utils/sstream.h"
#include "core/binary_parser.h"
#include "core/config.h"
#include "core/optimizer.h"
#include "core/native_generator.h"

int main(int argc, char **argv) {
  int result = 0;
  
  // Parse and validate arguments
  program_options_t options;
  if (parse_arguments(argc, argv, &options) != 0) {
    print_usage();
    return 1;
  }
  
  // Load target configuration
  target_config_t* target = load_target_config(options.target_config);
  if (!target) {
    fprintf(stderr, "Failed to load target configuration\n");
    return 1;
  }
  
  // Open input stream
  stream_t* input_stream = open_stream(options.input_file);
  if (!input_stream) {
    fprintf(stderr, "Failed to open input file\n");
    free_target_config(target);
    return 1;
  }
  
  // Parse COIL binary
  int error = 0;
  module_t* module = parse_coil_binary(input_stream, &error);
  if (!module) {
    fprintf(stderr, "Failed to parse COIL binary: error %d\n", error);
    close_stream(input_stream);
    free_target_config(target);
    return 1;
  }
  
  // Set up optimization context
  optimization_context_t opt_context;
  opt_context.target = target;
  opt_context.level = options.optimization_level;
  opt_context.enable_experimental = options.enable_experimental;
  
  // Apply target-independent optimizations
  if (options.optimization_level > 0) {
    if (optimize_module(module, &opt_context) != 0) {
      fprintf(stderr, "Optimization failed\n");
      result = 1;
      goto cleanup;
    }
  }
  
  // Apply target-specific optimizations
  if (options.optimization_level > 0) {
    if (apply_target_optimizations(module, target) != 0) {
      fprintf(stderr, "Target-specific optimization failed\n");
      result = 1;
      goto cleanup;
    }
  }
  
  // Generate native binary
  if (generate_native_binary(module, target, options.output_format, options.output_file) != 0) {
    fprintf(stderr, "Native code generation failed\n");
    result = 1;
    goto cleanup;
  }
  
cleanup:
  // Clean up resources
  free_module(module);
  close_stream(input_stream);
  free_target_config(target);
  
  return result;
}