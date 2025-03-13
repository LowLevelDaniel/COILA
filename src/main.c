/**
 * @file main.c
 * @brief Main program for COIL assembler.
 */

#include "coil_assembler.h"
#include "coil_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Print command-line usage help
 * 
 * @param program_name Name of the program
 */
void print_usage(const char* program_name) {
  printf("COIL Assembler - Translates COIL binary to native code\n");
  printf("Usage: %s [options] <input-file> -o <output-file>\n", program_name);
  printf("\n");
  printf("Options:\n");
  printf("  -h, --help                 Show this help message\n");
  printf("  -o, --output <file>        Specify output file\n");
  printf("  -t, --target <target>      Specify target architecture\n");
  printf("                             (default: x86_64)\n");
  printf("  -d, --device <device>      Specify device class\n");
  printf("                             (cpu, gpu, npu, tpu, dsp, fpga)\n");
  printf("  -O<level>                  Set optimization level (0-3)\n");
  printf("  -v, --verbose              Increase verbosity\n");
  printf("  -q, --quiet                Decrease verbosity\n");
  printf("\n");
  printf("Example:\n");
  printf("  %s -t x86_64 -O2 input.coil -o output.o\n", program_name);
}

/**
 * @brief Parse command-line arguments
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @param input_file Input file path
 * @param output_file Output file path
 * @param target_name Target architecture name
 * @param device_class Device class
 * @param opt_level Optimization level
 * @param verbosity Verbosity level
 * @return true if parsing succeeded, false otherwise
 */
bool parse_arguments(int argc, char* argv[], 
                    char** input_file, 
                    char** output_file,
                    char** target_name,
                    coil_device_class_t* device_class,
                    int* opt_level,
                    int* verbosity) {
  // Set defaults
  *input_file = NULL;
  *output_file = NULL;
  *target_name = strdup("x86_64");
  *device_class = COIL_DEVICE_CPU;
  *opt_level = 2;
  *verbosity = 1;  // Default to errors only
  
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return false;
    } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
      if (i + 1 < argc) {
        *output_file = strdup(argv[++i]);
      } else {
        fprintf(stderr, "Error: Missing output file\n");
        return false;
      }
    } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--target") == 0) {
      if (i + 1 < argc) {
        free(*target_name);
        *target_name = strdup(argv[++i]);
      } else {
        fprintf(stderr, "Error: Missing target name\n");
        return false;
      }
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--device") == 0) {
      if (i + 1 < argc) {
        i++;
        if (strcmp(argv[i], "cpu") == 0) {
          *device_class = COIL_DEVICE_CPU;
        } else if (strcmp(argv[i], "gpu") == 0) {
          *device_class = COIL_DEVICE_GPU;
        } else if (strcmp(argv[i], "npu") == 0) {
          *device_class = COIL_DEVICE_NPU;
        } else if (strcmp(argv[i], "tpu") == 0) {
          *device_class = COIL_DEVICE_TPU;
        } else if (strcmp(argv[i], "dsp") == 0) {
          *device_class = COIL_DEVICE_DSP;
        } else if (strcmp(argv[i], "fpga") == 0) {
          *device_class = COIL_DEVICE_FPGA;
        } else {
          fprintf(stderr, "Error: Unknown device class: %s\n", argv[i]);
          return false;
        }
      } else {
        fprintf(stderr, "Error: Missing device class\n");
        return false;
      }
    } else if (strncmp(argv[i], "-O", 2) == 0) {
      // Parse optimization level
      if (strlen(argv[i]) == 3 && argv[i][2] >= '0' && argv[i][2] <= '3') {
        *opt_level = argv[i][2] - '0';
      } else {
        fprintf(stderr, "Error: Invalid optimization level: %s\n", argv[i]);
        return false;
      }
    } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      (*verbosity)++;
    } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
      (*verbosity)--;
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
      return false;
    } else {
      // Assume it's the input file
      if (*input_file == NULL) {
        *input_file = strdup(argv[i]);
      } else {
        fprintf(stderr, "Error: Multiple input files not supported\n");
        return false;
      }
    }
  }
  
  // Validate required arguments
  if (*input_file == NULL) {
    fprintf(stderr, "Error: No input file specified\n");
    return false;
  }
  
  if (*output_file == NULL) {
    fprintf(stderr, "Error: No output file specified\n");
    return false;
  }
  
  return true;
}

/**
 * @brief Main function
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char* argv[]) {
  char* input_file = NULL;
  char* output_file = NULL;
  char* target_name = NULL;
  coil_device_class_t device_class;
  int opt_level;
  int verbosity;
  
  // Parse command-line arguments
  if (!parse_arguments(argc, argv, &input_file, &output_file, &target_name, 
                      &device_class, &opt_level, &verbosity)) {
    return 1;
  }
  
  // Set log level based on verbosity
  coil_set_log_level(verbosity);
  
  coil_info("COIL Assembler");
  coil_info("Input file: %s", input_file);
  coil_info("Output file: %s", output_file);
  coil_info("Target: %s", target_name);
  coil_info("Device class: %d", device_class);
  coil_info("Optimization level: %d", opt_level);
  
  // Initialize assembler
  coil_assembler_handle_t assembler = coil_assembler_init(target_name, device_class);
  if (!assembler) {
    coil_error("Failed to initialize assembler: %s", coil_get_error_message());
    
    // Clean up
    free(input_file);
    free(output_file);
    free(target_name);
    
    return 1;
  }
  
  // Set optimization level
  coil_error_t error = coil_set_optimization_level(assembler, opt_level);
  if (error != COIL_SUCCESS) {
    coil_error("Failed to set optimization level: %s", coil_get_error_message());
    
    // Clean up
    coil_assembler_cleanup(assembler);
    free(input_file);
    free(output_file);
    free(target_name);
    
    return 1;
  }
  
  // Load COIL module
  coil_module_handle_t module = coil_module_load(input_file);
  if (!module) {
    coil_error("Failed to load COIL module: %s", coil_get_error_message());
    
    // Clean up
    coil_assembler_cleanup(assembler);
    free(input_file);
    free(output_file);
    free(target_name);
    
    return 1;
  }
  
  // Assemble module
  error = coil_assemble(assembler, module, output_file);
  if (error != COIL_SUCCESS) {
    coil_error("Failed to assemble module: %s", coil_get_error_message());
    
    // Clean up
    coil_module_free(module);
    coil_assembler_cleanup(assembler);
    free(input_file);
    free(output_file);
    free(target_name);
    
    return 1;
  }
  
  coil_info("Assembly completed successfully");
  
  // Clean up
  coil_module_free(module);
  coil_assembler_cleanup(assembler);
  free(input_file);
  free(output_file);
  free(target_name);
  
  return 0;
}