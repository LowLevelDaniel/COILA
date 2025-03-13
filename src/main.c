/**
 * @file main.c
 * @brief Main entry point for the COIL assembler
 * 
 * This file contains the main function and command-line argument parsing
 * for the COIL assembler.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "include/coil_assembler.h"
#include "utils/logging.h"
#include "utils/memory_management.h"
#include "core/binary_parser.h"
#include "core/instruction_decoder.h"
#include "core/target_config.h"
#include "core/translator.h"
#include "core/optimization.h"
#include "core/native_generator.h"

/**
 * @brief Maximum length of a filename
 */
#define MAX_FILENAME_LENGTH 256

/**
 * @brief Command line options
 */
typedef struct {
  char input_file[MAX_FILENAME_LENGTH];     /**< Input file path */
  char output_file[MAX_FILENAME_LENGTH];    /**< Output file path */
  char target_name[MAX_FILENAME_LENGTH];    /**< Target architecture name */
  char config_file[MAX_FILENAME_LENGTH];    /**< Target configuration file */
  coil_opt_level_t opt_level;               /**< Optimization level */
  uint8_t verbose;                          /**< Verbosity level */
  bool debug_info;                          /**< Whether to include debug information */
  bool help;                                /**< Whether to display help */
  bool version;                             /**< Whether to display version */
} cmd_options_t;

/**
 * @brief Displays the version information
 */
static void display_version(void) {
  printf("COIL Assembler %s\n", coil_assembler_version());
  printf("Copyright (c) 2025 COIL Assembler Team\n");
  printf("MIT License\n");
}

/**
 * @brief Displays the help message
 *
 * @param[in] program_name Name of the program executable
 */
static void display_help(const char* program_name) {
  printf("Usage: %s [OPTIONS] input_file output_file\n\n", program_name);
  printf("Options:\n");
  printf("  -t, --target=NAME     Target architecture (default: auto-detect)\n");
  printf("  -c, --config=FILE     Target configuration file\n");
  printf("  -O, --optimize=LEVEL  Optimization level (0-3, default: 1)\n");
  printf("  -v, --verbose         Increase verbosity\n");
  printf("  -d, --debug           Include debug information\n");
  printf("  -h, --help            Display help and exit\n");
  printf("  -V, --version         Output version information and exit\n");
}

/**
 * @brief Parses command line arguments
 *
 * @param[in] argc Argument count
 * @param[in] argv Argument values
 * @param[out] options Parsed options
 * @return true if parsing was successful, false otherwise
 */
static bool parse_arguments(int argc, char* argv[], cmd_options_t* options) {
  if (options == NULL) {
    return false;
  }
  
  /* Initialize default values */
  memset(options, 0, sizeof(cmd_options_t));
  options->opt_level = COIL_OPT_BASIC;
  
  /* Process arguments */
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      /* Option argument */
      if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
        options->help = true;
      } else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
        options->version = true;
      } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
        options->verbose++;
      } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
        options->debug_info = true;
      } else if (strncmp(argv[i], "-t=", 3) == 0) {
        strncpy(options->target_name, argv[i] + 3, sizeof(options->target_name) - 1);
      } else if (strncmp(argv[i], "--target=", 9) == 0) {
        strncpy(options->target_name, argv[i] + 9, sizeof(options->target_name) - 1);
      } else if (strncmp(argv[i], "-c=", 3) == 0) {
        strncpy(options->config_file, argv[i] + 3, sizeof(options->config_file) - 1);
      } else if (strncmp(argv[i], "--config=", 9) == 0) {
        strncpy(options->config_file, argv[i] + 9, sizeof(options->config_file) - 1);
      } else if (strcmp(argv[i], "-O0") == 0 || strcmp(argv[i], "--optimize=0") == 0) {
        options->opt_level = COIL_OPT_NONE;
      } else if (strcmp(argv[i], "-O1") == 0 || strcmp(argv[i], "--optimize=1") == 0) {
        options->opt_level = COIL_OPT_BASIC;
      } else if (strcmp(argv[i], "-O2") == 0 || strcmp(argv[i], "--optimize=2") == 0) {
        options->opt_level = COIL_OPT_MODERATE;
      } else if (strcmp(argv[i], "-O3") == 0 || strcmp(argv[i], "--optimize=3") == 0) {
        options->opt_level = COIL_OPT_AGGRESSIVE;
      } else if (strncmp(argv[i], "-O", 2) == 0) {
        int opt = atoi(argv[i] + 2);
        switch (opt) {
          case 0:
            options->opt_level = COIL_OPT_NONE;
            break;
          case 1:
            options->opt_level = COIL_OPT_BASIC;
            break;
          case 2:
            options->opt_level = COIL_OPT_MODERATE;
            break;
          case 3:
            options->opt_level = COIL_OPT_AGGRESSIVE;
            break;
          default:
            fprintf(stderr, "Invalid optimization level: %d\n", opt);
            return false;
        }
      } else {
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return false;
      }
    } else {
      /* Positional argument */
      if (options->input_file[0] == '\0') {
        strncpy(options->input_file, argv[i], sizeof(options->input_file) - 1);
      } else if (options->output_file[0] == '\0') {
        strncpy(options->output_file, argv[i], sizeof(options->output_file) - 1);
      } else {
        fprintf(stderr, "Too many positional arguments\n");
        return false;
      }
    }
  }
  
  /* Check required arguments if not displaying help or version */
  if (!options->help && !options->version) {
    if (options->input_file[0] == '\0') {
      fprintf(stderr, "Input file not specified\n");
      return false;
    }
    
    if (options->output_file[0] == '\0') {
      fprintf(stderr, "Output file not specified\n");
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Reads a binary file into memory
 *
 * @param[in] filename File path
 * @param[out] data Pointer to receive the file data
 * @param[out] size Pointer to receive the file size
 * @return Error code indicating success or failure
 */
static error_t read_binary_file(const char* filename, uint8_t** data, size_t* size) {
  if (filename == NULL || data == NULL || size == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *data = NULL;
  *size = 0;
  
  /* Open file */
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    log_error("Failed to open input file: %s", filename);
    return ERROR_FILE_IO;
  }
  
  /* Get file size */
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  if (file_size < 0) {
    log_error("Failed to determine size of input file: %s", filename);
    fclose(file);
    return ERROR_FILE_IO;
  }
  
  /* Allocate buffer */
  uint8_t* buffer = memory_alloc(file_size);
  if (buffer == NULL) {
    log_error("Failed to allocate memory for input file: %s", filename);
    fclose(file);
    return ERROR_MEMORY;
  }
  
  /* Read file data */
  size_t read_size = fread(buffer, 1, file_size, file);
  fclose(file);
  
  if (read_size != (size_t)file_size) {
    log_error("Failed to read input file: %s", filename);
    memory_free(buffer);
    return ERROR_FILE_IO;
  }
  
  *data = buffer;
  *size = read_size;
  
  return ERROR_NONE;
}

/**
 * @brief Main assembler workflow
 *
 * @param[in] options Command line options
 * @return Error code indicating success or failure
 */
static error_t assemble(const cmd_options_t* options) {
  if (options == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Read input file */
  uint8_t* input_data = NULL;
  size_t input_size = 0;
  
  error_t read_result = read_binary_file(options->input_file, &input_data, &input_size);
  if (read_result != ERROR_NONE) {
    return read_result;
  }
  
  log_info("Read %zu bytes from %s", input_size, options->input_file);
  
  /* Create COIL assembler configuration */
  coil_assembler_config_t config;
  memset(&config, 0, sizeof(config));
  
  if (options->target_name[0] != '\0') {
    config.target_name = options->target_name;
  }
  
  if (options->config_file[0] != '\0') {
    config.config_file = options->config_file;
  }
  
  config.opt_level = options->opt_level;
  config.verbose = options->verbose;
  config.debug_info = options->debug_info;
  
  /* Create assembler instance */
  coil_assembler_handle_t assembler = NULL;
  
  coil_status_t assembler_result = coil_assembler_create(&config, &assembler);
  if (assembler_result != COIL_SUCCESS) {
    log_error("Failed to create assembler: %s", coil_status_string(assembler_result));
    memory_free(input_data);
    return ERROR_INTERNAL;
  }
  
  /* Assemble the input file */
  coil_status_t assemble_result = coil_assembler_assemble(
    assembler,
    input_data,
    input_size,
    NULL,  /* We don't need the binary output directly */
    NULL
  );
  
  if (assemble_result != COIL_SUCCESS) {
    log_error("Failed to assemble input file: %s", coil_status_string(assemble_result));
    coil_assembler_destroy(assembler);
    memory_free(input_data);
    return ERROR_INTERNAL;
  }
  
  /* Write the output file */
  assemble_result = coil_assembler_assemble_file(
    assembler,
    options->input_file,
    options->output_file
  );
  
  if (assemble_result != COIL_SUCCESS) {
    log_error("Failed to write output file: %s", coil_status_string(assemble_result));
    coil_assembler_destroy(assembler);
    memory_free(input_data);
    return ERROR_INTERNAL;
  }
  
  log_info("Successfully assembled %s to %s", options->input_file, options->output_file);
  
  /* Cleanup */
  coil_assembler_destroy(assembler);
  memory_free(input_data);
  
  return ERROR_NONE;
}

/**
 * @brief Program entry point
 *
 * @param[in] argc Argument count
 * @param[in] argv Argument values
 * @return Exit code (0 for success, non-zero for error)
 */
int main(int argc, char* argv[]) {
  /* Parse command line arguments */
  cmd_options_t options;
  if (!parse_arguments(argc, argv, &options)) {
    display_help(argv[0]);
    return 1;
  }
  
  /* Handle help and version options */
  if (options.help) {
    display_help(argv[0]);
    return 0;
  }
  
  if (options.version) {
    display_version();
    return 0;
  }
  
  /* Initialize subsystems */
  memory_init(true);  /* Enable memory tracking */
  log_init(options.verbose);
  
  /* Run assembler */
  error_t result = assemble(&options);
  
  /* Check for memory leaks */
  size_t leak_count = 0;
  if (memory_check_leaks(&leak_count)) {
    log_warning("Memory leaks detected: %zu allocations not freed", leak_count);
    
    /* Dump leak information to a file if in verbose mode */
    if (options.verbose > 0) {
      memory_dump_leaks("coil_assembler_leaks.log");
      log_info("Memory leak information written to coil_assembler_leaks.log");
    }
  }
  
  /* Cleanup subsystems */
  log_shutdown();
  memory_shutdown();
  
  return (result == ERROR_NONE) ? 0 : 1;
}