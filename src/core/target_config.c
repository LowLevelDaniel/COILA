/**
 * @file target_config.c
 * @brief Implementation of target architecture configuration handler
 * 
 * This module implements loading, parsing, and managing target architecture
 * configurations, providing access to architecture capabilities and constraints.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "target_config.h"
#include "../utils/memory_management.h"
#include "../utils/logging.h"

/**
 * @brief Maximum line length in config file
 */
#define MAX_LINE_LENGTH 1024

/**
 * @brief Maximum key length in config file
 */
#define MAX_KEY_LENGTH 64

/**
 * @brief Maximum value length in config file
 */
#define MAX_VALUE_LENGTH 256

/**
 * @brief Configuration file section
 */
typedef enum {
  SECTION_NONE,
  SECTION_GENERAL,
  SECTION_FEATURES,
  SECTION_RESOURCES,
  SECTION_MEMORY,
  SECTION_OPTIMIZATION
} config_section_t;

/**
 * @brief Creates an empty target configuration
 *
 * @param[out] config Pointer to receive the empty configuration
 * @return Error code indicating success or failure
 */
error_t target_config_create_empty(target_config_t** config) {
  if (config == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  target_config_t* new_config = memory_calloc(1, sizeof(target_config_t));
  if (new_config == NULL) {
    return ERROR_MEMORY;
  }
  
  /* Initialize with default values */
  new_config->name = NULL;
  new_config->architecture = NULL;
  new_config->vendor = NULL;
  new_config->description = NULL;
  new_config->features = NULL;
  new_config->feature_count = 0;
  
  /* Default resource values */
  new_config->resources.general_registers = 0;
  new_config->resources.float_registers = 0;
  new_config->resources.vector_registers = 0;
  new_config->resources.vector_width = 0;
  new_config->resources.min_alignment = 1;
  new_config->resources.memory_models = NULL;
  new_config->resources.memory_model_count = 0;
  
  /* Default memory values */
  new_config->memory.alignment = 1;
  new_config->memory.page_size = 4096;
  new_config->memory.cacheline_size = 64;
  
  /* Default optimization values */
  new_config->optimization.vector_threshold = 4;
  new_config->optimization.unroll_factor = 4;
  new_config->optimization.use_fma = false;
  new_config->optimization.specialized_strategies = NULL;
  new_config->optimization.strategy_count = 0;
  
  *config = new_config;
  return ERROR_NONE;
}

/**
 * @brief Trims whitespace from the beginning and end of a string
 *
 * @param[in,out] str String to trim
 */
static void trim_string(char* str) {
  if (str == NULL) {
    return;
  }
  
  char* start = str;
  char* end = str + strlen(str) - 1;
  
  /* Trim leading whitespace */
  while (isspace((unsigned char)*start)) {
    start++;
  }
  
  /* Check if string is all whitespace */
  if (*start == '\0') {
    *str = '\0';
    return;
  }
  
  /* Trim trailing whitespace */
  while (end > start && isspace((unsigned char)*end)) {
    end--;
  }
  
  /* Null terminate the trimmed string */
  *(end + 1) = '\0';
  
  /* Move the trimmed string to the beginning if necessary */
  if (start != str) {
    memmove(str, start, (end - start) + 2);
  }
}

/**
 * @brief Parses a memory model string to its enum value
 *
 * @param[in] model_str Memory model string
 * @return Memory model enum value, or MEMORY_MODEL_STRONG on error
 */
static memory_model_t parse_memory_model(const char* model_str) {
  if (model_str == NULL) {
    return MEMORY_MODEL_STRONG;
  }
  
  if (strcmp(model_str, "strong") == 0) {
    return MEMORY_MODEL_STRONG;
  } else if (strcmp(model_str, "acquire-release") == 0) {
    return MEMORY_MODEL_ACQUIRE_RELEASE;
  } else if (strcmp(model_str, "relaxed") == 0) {
    return MEMORY_MODEL_RELAXED;
  }
  
  log_warning("Unknown memory model: %s, defaulting to strong", model_str);
  return MEMORY_MODEL_STRONG;
}

/**
 * @brief Parses a comma-separated list into a string array
 *
 * @param[in] list_str Comma-separated list string
 * @param[out] array Pointer to receive the allocated array
 * @param[out] count Pointer to receive the array count
 * @return Error code indicating success or failure
 */
static error_t parse_string_list(
  const char* list_str,
  char*** array,
  uint32_t* count
) {
  if (list_str == NULL || array == NULL || count == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *array = NULL;
  *count = 0;
  
  /* Copy the string for tokenization */
  char* list_copy = memory_alloc(strlen(list_str) + 1);
  if (list_copy == NULL) {
    return ERROR_MEMORY;
  }
  strcpy(list_copy, list_str);
  
  /* Count the number of items */
  char* token = strtok(list_copy, ",");
  uint32_t item_count = 0;
  
  while (token != NULL) {
    item_count++;
    token = strtok(NULL, ",");
  }
  
  if (item_count == 0) {
    memory_free(list_copy);
    return ERROR_NONE;
  }
  
  /* Allocate the array */
  char** new_array = memory_calloc(item_count, sizeof(char*));
  if (new_array == NULL) {
    memory_free(list_copy);
    return ERROR_MEMORY;
  }
  
  /* Copy the list string again for the second tokenization */
  strcpy(list_copy, list_str);
  
  /* Parse each item */
  token = strtok(list_copy, ",");
  uint32_t index = 0;
  
  while (token != NULL && index < item_count) {
    /* Trim the token */
    char trimmed_token[MAX_VALUE_LENGTH];
    strncpy(trimmed_token, token, sizeof(trimmed_token) - 1);
    trimmed_token[sizeof(trimmed_token) - 1] = '\0';
    trim_string(trimmed_token);
    
    /* Allocate and copy the item */
    new_array[index] = memory_alloc(strlen(trimmed_token) + 1);
    if (new_array[index] == NULL) {
      /* Free previously allocated items */
      for (uint32_t i = 0; i < index; i++) {
        memory_free(new_array[i]);
      }
      memory_free(new_array);
      memory_free(list_copy);
      return ERROR_MEMORY;
    }
    
    strcpy(new_array[index], trimmed_token);
    index++;
    
    token = strtok(NULL, ",");
  }
  
  memory_free(list_copy);
  
  *array = new_array;
  *count = item_count;
  
  return ERROR_NONE;
}

/**
 * @brief Parses a memory model list into an array
 *
 * @param[in] list_str Comma-separated list of memory models
 * @param[out] models Pointer to receive the allocated array
 * @param[out] count Pointer to receive the array count
 * @return Error code indicating success or failure
 */
static error_t parse_memory_model_list(
  const char* list_str,
  memory_model_t** models,
  uint32_t* count
) {
  if (list_str == NULL || models == NULL || count == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *models = NULL;
  *count = 0;
  
  /* Parse string list first */
  char** model_strings = NULL;
  uint32_t model_count = 0;
  
  error_t list_result = parse_string_list(list_str, &model_strings, &model_count);
  if (list_result != ERROR_NONE) {
    return list_result;
  }
  
  if (model_count == 0) {
    return ERROR_NONE;
  }
  
  /* Allocate memory model array */
  memory_model_t* new_models = memory_calloc(model_count, sizeof(memory_model_t));
  if (new_models == NULL) {
    /* Free string list */
    for (uint32_t i = 0; i < model_count; i++) {
      memory_free(model_strings[i]);
    }
    memory_free(model_strings);
    return ERROR_MEMORY;
  }
  
  /* Convert strings to enum values */
  for (uint32_t i = 0; i < model_count; i++) {
    new_models[i] = parse_memory_model(model_strings[i]);
  }
  
  /* Free string list */
  for (uint32_t i = 0; i < model_count; i++) {
    memory_free(model_strings[i]);
  }
  memory_free(model_strings);
  
  *models = new_models;
  *count = model_count;
  
  return ERROR_NONE;
}

/**
 * @brief Adds a feature to a target configuration
 *
 * @param[in,out] config Target configuration
 * @param[in] feature Feature name
 * @return Error code indicating success or failure
 */
static error_t add_feature(target_config_t* config, const char* feature) {
  if (config == NULL || feature == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check if feature already exists */
  for (uint32_t i = 0; i < config->feature_count; i++) {
    if (strcmp(config->features[i], feature) == 0) {
      return ERROR_NONE;  /* Feature already exists */
    }
  }
  
  /* Check if maximum feature count reached */
  if (config->feature_count >= MAX_FEATURES) {
    log_error("Maximum feature count reached (%u)", MAX_FEATURES);
    return ERROR_CONFIG;
  }
  
  /* Allocate or reallocate features array */
  if (config->features == NULL) {
    config->features = memory_calloc(1, sizeof(char*));
    if (config->features == NULL) {
      return ERROR_MEMORY;
    }
  } else {
    char** new_features = memory_realloc(
      config->features,
      (config->feature_count + 1) * sizeof(char*)
    );
    
    if (new_features == NULL) {
      return ERROR_MEMORY;
    }
    
    config->features = new_features;
  }
  
  /* Allocate and copy feature name */
  config->features[config->feature_count] = memory_alloc(strlen(feature) + 1);
  if (config->features[config->feature_count] == NULL) {
    return ERROR_MEMORY;
  }
  
  strcpy(config->features[config->feature_count], feature);
  config->feature_count++;
  
  return ERROR_NONE;
}

/**
 * @brief Parses a line from a configuration file
 *
 * @param[in] line Line to parse
 * @param[in,out] section Current section (updated)
 * @param[in,out] config Target configuration to update
 * @return Error code indicating success or failure
 */
static error_t parse_config_line(
  char* line,
  config_section_t* section,
  target_config_t* config
) {
  if (line == NULL || section == NULL || config == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Skip empty lines and comments */
  trim_string(line);
  if (line[0] == '\0' || line[0] == '#') {
    return ERROR_NONE;
  }
  
  /* Check for section header */
  if (line[0] == '[' && line[strlen(line) - 1] == ']') {
    /* Extract section name */
    char section_name[MAX_KEY_LENGTH];
    size_t section_len = strlen(line) - 2;
    
    if (section_len >= sizeof(section_name)) {
      log_error("Section name too long: %s", line);
      return ERROR_CONFIG;
    }
    
    strncpy(section_name, line + 1, section_len);
    section_name[section_len] = '\0';
    
    /* Determine section type */
    if (strcmp(section_name, "general") == 0) {
      *section = SECTION_GENERAL;
    } else if (strcmp(section_name, "features") == 0) {
      *section = SECTION_FEATURES;
    } else if (strcmp(section_name, "resources") == 0) {
      *section = SECTION_RESOURCES;
    } else if (strcmp(section_name, "memory") == 0) {
      *section = SECTION_MEMORY;
    } else if (strcmp(section_name, "optimization") == 0) {
      *section = SECTION_OPTIMIZATION;
    } else {
      log_warning("Unknown section: %s", section_name);
      *section = SECTION_NONE;
    }
    
    return ERROR_NONE;
  }
  
  /* Skip if no active section */
  if (*section == SECTION_NONE) {
    return ERROR_NONE;
  }
  
  /* Parse key-value pair */
  char* separator = strchr(line, '=');
  if (separator == NULL) {
    /* Handle special case for features section */
    if (*section == SECTION_FEATURES) {
      return add_feature(config, line);
    }
    
    log_warning("Invalid configuration line: %s", line);
    return ERROR_NONE;
  }
  
  /* Extract key and value */
  char key[MAX_KEY_LENGTH];
  char value[MAX_VALUE_LENGTH];
  
  size_t key_len = separator - line;
  if (key_len >= sizeof(key)) {
    log_error("Key too long: %s", line);
    return ERROR_CONFIG;
  }
  
  strncpy(key, line, key_len);
  key[key_len] = '\0';
  trim_string(key);
  
  strncpy(value, separator + 1, sizeof(value) - 1);
  value[sizeof(value) - 1] = '\0';
  trim_string(value);
  
  /* Process key-value pair based on section */
  switch (*section) {
    case SECTION_GENERAL:
      if (strcmp(key, "name") == 0) {
        if (config->name != NULL) {
          memory_free(config->name);
        }
        config->name = memory_alloc(strlen(value) + 1);
        if (config->name == NULL) {
          return ERROR_MEMORY;
        }
        strcpy(config->name, value);
      } else if (strcmp(key, "architecture") == 0) {
        if (config->architecture != NULL) {
          memory_free(config->architecture);
        }
        config->architecture = memory_alloc(strlen(value) + 1);
        if (config->architecture == NULL) {
          return ERROR_MEMORY;
        }
        strcpy(config->architecture, value);
      } else if (strcmp(key, "vendor") == 0) {
        if (config->vendor != NULL) {
          memory_free(config->vendor);
        }
        config->vendor = memory_alloc(strlen(value) + 1);
        if (config->vendor == NULL) {
          return ERROR_MEMORY;
        }
        strcpy(config->vendor, value);
      } else if (strcmp(key, "description") == 0) {
        if (config->description != NULL) {
          memory_free(config->description);
        }
        config->description = memory_alloc(strlen(value) + 1);
        if (config->description == NULL) {
          return ERROR_MEMORY;
        }
        strcpy(config->description, value);
      } else {
        log_warning("Unknown key in general section: %s", key);
      }
      break;
    
    case SECTION_FEATURES:
      /* Features are handled separately above */
      add_feature(config, key);
      break;
    
    case SECTION_RESOURCES:
      if (strcmp(key, "general_registers") == 0) {
        config->resources.general_registers = (uint32_t)strtoul(value, NULL, 0);
      } else if (strcmp(key, "float_registers") == 0) {
        config->resources.float_registers = (uint32_t)strtoul(value, NULL, 0);
      } else if (strcmp(key, "vector_registers") == 0) {
        config->resources.vector_registers = (uint32_t)strtoul(value, NULL, 0);
      } else if (strcmp(key, "vector_width") == 0) {
        config->resources.vector_width = (uint32_t)strtoul(value, NULL, 0);
      } else if (strcmp(key, "min_alignment") == 0) {
        config->resources.min_alignment = (uint32_t)strtoul(value, NULL, 0);
      } else if (strcmp(key, "memory_models") == 0) {
        /* Parse memory models */
        memory_model_t* models = NULL;
        uint32_t model_count = 0;
        
        error_t model_result = parse_memory_model_list(value, &models, &model_count);
        if (model_result != ERROR_NONE) {
          return model_result;
        }
        
        /* Free existing models if any */
        if (config->resources.memory_models != NULL) {
          memory_free(config->resources.memory_models);
        }
        
        config->resources.memory_models = models;
        config->resources.memory_model_count = model_count;
      } else {
        log_warning("Unknown key in resources section: %s", key);
      }
      break;
    
    case SECTION_MEMORY:
      if (strcmp(key, "alignment") == 0) {
        config->memory.alignment = (uint32_t)strtoul(value, NULL, 0);
      } else if (strcmp(key, "page_size") == 0) {
        config->memory.page_size = (uint32_t)strtoul(value, NULL, 0);
      } else if (strcmp(key, "cacheline_size") == 0) {
        config->memory.cacheline_size = (uint32_t)strtoul(value, NULL, 0);
      } else {
        log_warning("Unknown key in memory section: %s", key);
      }
      break;
    
    case SECTION_OPTIMIZATION:
      if (strcmp(key, "vector_threshold") == 0) {
        config->optimization.vector_threshold = (uint32_t)strtoul(value, NULL, 0);
      } else if (strcmp(key, "unroll_factor") == 0) {
        config->optimization.unroll_factor = (uint32_t)strtoul(value, NULL, 0);
      } else if (strcmp(key, "use_fma") == 0) {
        /* Parse boolean value */
        if (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "1") == 0) {
          config->optimization.use_fma = true;
        } else {
          config->optimization.use_fma = false;
        }
      } else if (strcmp(key, "specialized_strategies") == 0) {
        /* Parse strategies */
        char** strategies = NULL;
        uint32_t strategy_count = 0;
        
        error_t strategy_result = parse_string_list(value, &strategies, &strategy_count);
        if (strategy_result != ERROR_NONE) {
          return strategy_result;
        }
        
        /* Free existing strategies if any */
        if (config->optimization.specialized_strategies != NULL) {
          for (uint32_t i = 0; i < config->optimization.strategy_count; i++) {
            memory_free(config->optimization.specialized_strategies[i]);
          }
          memory_free(config->optimization.specialized_strategies);
        }
        
        config->optimization.specialized_strategies = strategies;
        config->optimization.strategy_count = strategy_count;
      } else {
        log_warning("Unknown key in optimization section: %s", key);
      }
      break;
    
    default:
      /* Should not happen */
      break;
  }
  
  return ERROR_NONE;
}

error_t target_config_load(const char* path, target_config_t** config) {
  if (path == NULL || config == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *config = NULL;
  
  /* Create empty configuration first */
  target_config_t* new_config = NULL;
  error_t create_result = target_config_create_empty(&new_config);
  if (create_result != ERROR_NONE) {
    return create_result;
  }
  
  /* Open configuration file */
  FILE* file = fopen(path, "r");
  if (file == NULL) {
    log_error("Failed to open configuration file: %s", path);
    target_config_free(new_config);
    return ERROR_FILE_IO;
  }
  
  /* Parse configuration file */
  char line[MAX_LINE_LENGTH];
  config_section_t current_section = SECTION_NONE;
  int line_number = 0;
  
  while (fgets(line, sizeof(line), file) != NULL) {
    line_number++;
    
    /* Remove newline */
    size_t len = strlen(line);
    if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[len - 1] = '\0';
    }
    
    /* Parse line */
    error_t parse_result = parse_config_line(line, &current_section, new_config);
    if (parse_result != ERROR_NONE) {
      log_error("Error parsing configuration file at line %d: %s",
               line_number, error_message(parse_result));
      fclose(file);
      target_config_free(new_config);
      return parse_result;
    }
  }
  
  fclose(file);
  
  /* Validate configuration */
  error_t validate_result = target_config_validate(new_config);
  if (validate_result != ERROR_NONE) {
    log_error("Invalid target configuration: %s", error_message(validate_result));
    target_config_free(new_config);
    return validate_result;
  }
  
  log_info("Successfully loaded target configuration: %s", new_config->name);
  
  *config = new_config;
  return ERROR_NONE;
}

error_t target_config_free(target_config_t* config) {
  if (config == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Free general fields */
  memory_free(config->name);
  memory_free(config->architecture);
  memory_free(config->vendor);
  memory_free(config->description);
  
  /* Free features */
  if (config->features != NULL) {
    for (uint32_t i = 0; i < config->feature_count; i++) {
      memory_free(config->features[i]);
    }
    memory_free(config->features);
  }
  
  /* Free memory models */
  memory_free(config->resources.memory_models);
  
  /* Free specialized strategies */
  if (config->optimization.specialized_strategies != NULL) {
    for (uint32_t i = 0; i < config->optimization.strategy_count; i++) {
      memory_free(config->optimization.specialized_strategies[i]);
    }
    memory_free(config->optimization.specialized_strategies);
  }
  
  /* Free config structure */
  memory_free(config);
  
  return ERROR_NONE;
}

bool target_config_has_feature(const target_config_t* config, const char* feature_name) {
  if (config == NULL || feature_name == NULL) {
    return false;
  }
  
  /* Search for feature */
  for (uint32_t i = 0; i < config->feature_count; i++) {
    if (strcmp(config->features[i], feature_name) == 0) {
      return true;
    }
  }
  
  return false;
}

error_t target_config_validate(const target_config_t* config) {
  if (config == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Check required fields */
  if (config->name == NULL || config->name[0] == '\0') {
    log_error("Target configuration missing name");
    return ERROR_CONFIG;
  }
  
  if (config->architecture == NULL || config->architecture[0] == '\0') {
    log_error("Target configuration missing architecture");
    return ERROR_CONFIG;
  }
  
  /* Check resource constraints */
  if (config->resources.min_alignment == 0) {
    log_error("Minimum alignment cannot be zero");
    return ERROR_CONFIG;
  }
  
  if (config->memory.alignment == 0) {
    log_error("Memory alignment cannot be zero");
    return ERROR_CONFIG;
  }
  
  if (config->memory.page_size == 0) {
    log_error("Page size cannot be zero");
    return ERROR_CONFIG;
  }
  
  if (config->memory.cacheline_size == 0) {
    log_error("Cache line size cannot be zero");
    return ERROR_CONFIG;
  }
  
  return ERROR_NONE;
}

/**
 * @brief Detects CPU features for X86 architecture
 *
 * @param[in,out] config Target configuration to update
 */
static void detect_x86_features(target_config_t* config) {
  /* Default to 64-bit architecture for modern systems */
  add_feature(config, "x86_64");
  
  /* Add basic x86 features */
  add_feature(config, "sse");
  add_feature(config, "sse2");
  
  /* More advanced features would typically be detected using CPUID
   * For this implementation, we'll add common features found on modern CPUs
   */
  add_feature(config, "sse3");
  add_feature(config, "ssse3");
  add_feature(config, "sse4.1");
  add_feature(config, "sse4.2");
  add_feature(config, "avx");
  add_feature(config, "fma");
  add_feature(config, "avx2");
  
  /* Configure resources */
  if (target_config_has_feature(config, "x86_64")) {
    config->resources.general_registers = 16;
  } else {
    config->resources.general_registers = 8;
  }
  
  config->resources.float_registers = 8;  /* x87 FPU stack */
  
  if (target_config_has_feature(config, "avx")) {
    config->resources.vector_registers = 16;
    config->resources.vector_width = 256;
  } else if (target_config_has_feature(config, "sse")) {
    config->resources.vector_registers = 8;
    config->resources.vector_width = 128;
  }
  
  /* Configure memory model */
  config->resources.memory_models = memory_calloc(1, sizeof(memory_model_t));
  if (config->resources.memory_models != NULL) {
    config->resources.memory_models[0] = MEMORY_MODEL_STRONG;
    config->resources.memory_model_count = 1;
  }
  
  /* Configure memory properties */
  config->memory.alignment = 1;  /* x86 allows unaligned access */
  config->memory.page_size = 4096;
  config->memory.cacheline_size = 64;
  
  /* Configure optimization properties */
  config->optimization.vector_threshold = 4;
  config->optimization.unroll_factor = 4;
  config->optimization.use_fma = target_config_has_feature(config, "fma");
}

/**
 * @brief Detects CPU features for ARM architecture
 *
 * @param[in,out] config Target configuration to update
 */
static void detect_arm_features(target_config_t* config) {
  /* Since we can't detect ARM features reliably, we'll add common ARMv8 features */
  add_feature(config, "aarch64");
  add_feature(config, "neon");
  add_feature(config, "crypto");
  add_feature(config, "crc");
  add_feature(config, "lse");  /* Large System Extensions */
  
  /* Configure resources */
  config->resources.general_registers = 31;  /* X0-X30 */
  config->resources.float_registers = 32;    /* V0-V31 */
  config->resources.vector_registers = 32;   /* V0-V31 (same as float) */
  config->resources.vector_width = 128;      /* NEON is 128-bit */
  
  /* Configure memory model */
  config->resources.memory_models = memory_calloc(2, sizeof(memory_model_t));
  if (config->resources.memory_models != NULL) {
    config->resources.memory_models[0] = MEMORY_MODEL_ACQUIRE_RELEASE;
    config->resources.memory_models[1] = MEMORY_MODEL_RELAXED;
    config->resources.memory_model_count = 2;
  }
  
  /* Configure memory properties */
  config->memory.alignment = 4;  /* ARM typically requires aligned access */
  config->memory.page_size = 4096;
  config->memory.cacheline_size = 64;
  
  /* Configure optimization properties */
  config->optimization.vector_threshold = 4;
  config->optimization.unroll_factor = 4;
  config->optimization.use_fma = true;  /* ARM NEON supports FMA */
}

error_t target_config_detect_current(target_config_t** config) {
  if (config == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Create empty configuration */
  target_config_t* new_config = NULL;
  error_t create_result = target_config_create_empty(&new_config);
  if (create_result != ERROR_NONE) {
    return create_result;
  }
  
  /* Set basic information */
  new_config->name = memory_alloc(32);
  new_config->architecture = memory_alloc(16);
  new_config->vendor = memory_alloc(16);
  new_config->description = memory_alloc(64);
  
  if (new_config->name == NULL || new_config->architecture == NULL ||
      new_config->vendor == NULL || new_config->description == NULL) {
    target_config_free(new_config);
    return ERROR_MEMORY;
  }
  
  /* Determine architecture */
  #if defined(__x86_64__) || defined(_M_X64)
    strcpy(new_config->architecture, "x86_64");
    strcpy(new_config->name, "x86_64-auto");
    strcpy(new_config->description, "Automatically detected x86_64 platform");
    detect_x86_features(new_config);
  #elif defined(__i386__) || defined(_M_IX86)
    strcpy(new_config->architecture, "x86");
    strcpy(new_config->name, "x86-auto");
    strcpy(new_config->description, "Automatically detected x86 platform");
    detect_x86_features(new_config);
  #elif defined(__aarch64__) || defined(_M_ARM64)
    strcpy(new_config->architecture, "aarch64");
    strcpy(new_config->name, "aarch64-auto");
    strcpy(new_config->description, "Automatically detected AArch64 platform");
    detect_arm_features(new_config);
  #elif defined(__arm__) || defined(_M_ARM)
    strcpy(new_config->architecture, "arm");
    strcpy(new_config->name, "arm-auto");
    strcpy(new_config->description, "Automatically detected ARM platform");
    detect_arm_features(new_config);
  #else
    strcpy(new_config->architecture, "generic");
    strcpy(new_config->name, "generic-auto");
    strcpy(new_config->description, "Automatically detected generic platform");
    
    /* Add generic configuration */
    new_config->resources.general_registers = 8;
    new_config->resources.float_registers = 8;
    new_config->resources.vector_registers = 0;
    new_config->resources.vector_width = 0;
    new_config->resources.min_alignment = 4;
    
    new_config->memory.alignment = 4;
    new_config->memory.page_size = 4096;
    new_config->memory.cacheline_size = 64;
    
    new_config->optimization.vector_threshold = 4;
    new_config->optimization.unroll_factor = 4;
    new_config->optimization.use_fma = false;
  #endif
  
  strcpy(new_config->vendor, "generic");
  
  /* Validate configuration */
  error_t validate_result = target_config_validate(new_config);
  if (validate_result != ERROR_NONE) {
    log_error("Invalid auto-detected target configuration: %s", error_message(validate_result));
    target_config_free(new_config);
    return validate_result;
  }
  
  log_info("Successfully detected target configuration: %s", new_config->name);
  
  *config = new_config;
  return ERROR_NONE;
}

/**
 * @brief Merge string fields, with override taking precedence
 *
 * @param[in] base Base string
 * @param[in] override Override string
 * @param[out] result Pointer to receive the merged string
 * @return Error code indicating success or failure
 */
static error_t merge_string(const char* base, const char* override, char** result) {
  if (result == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *result = NULL;
  
  if (override != NULL) {
    *result = memory_alloc(strlen(override) + 1);
    if (*result == NULL) {
      return ERROR_MEMORY;
    }
    strcpy(*result, override);
  } else if (base != NULL) {
    *result = memory_alloc(strlen(base) + 1);
    if (*result == NULL) {
      return ERROR_MEMORY;
    }
    strcpy(*result, base);
  }
  
  return ERROR_NONE;
}

/**
 * @brief Merges features from base and override configurations
 *
 * @param[in] base Base configuration
 * @param[in] override Override configuration
 * @param[out] result Result configuration
 * @return Error code indicating success or failure
 */
static error_t merge_features(
  const target_config_t* base,
  const target_config_t* override,
  target_config_t* result
) {
  if (base == NULL || override == NULL || result == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Start with base features */
  for (uint32_t i = 0; i < base->feature_count; i++) {
    error_t feature_result = add_feature(result, base->features[i]);
    if (feature_result != ERROR_NONE) {
      return feature_result;
    }
  }
  
  /* Add override features (duplicates will be ignored) */
  for (uint32_t i = 0; i < override->feature_count; i++) {
    error_t feature_result = add_feature(result, override->features[i]);
    if (feature_result != ERROR_NONE) {
      return feature_result;
    }
  }
  
  return ERROR_NONE;
}

/**
 * @brief Merges memory models from base and override configurations
 *
 * @param[in] base Base configuration
 * @param[in] override Override configuration
 * @param[out] result Result configuration
 * @return Error code indicating success or failure
 */
static error_t merge_memory_models(
  const target_config_t* base,
  const target_config_t* override,
  target_config_t* result
) {
  if (base == NULL || override == NULL || result == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* If override has memory models, use those */
  if (override->resources.memory_model_count > 0) {
    result->resources.memory_models = memory_calloc(
      override->resources.memory_model_count,
      sizeof(memory_model_t)
    );
    
    if (result->resources.memory_models == NULL) {
      return ERROR_MEMORY;
    }
    
    memcpy(
      result->resources.memory_models,
      override->resources.memory_models,
      override->resources.memory_model_count * sizeof(memory_model_t)
    );
    
    result->resources.memory_model_count = override->resources.memory_model_count;
  } else if (base->resources.memory_model_count > 0) {
    result->resources.memory_models = memory_calloc(
      base->resources.memory_model_count,
      sizeof(memory_model_t)
    );
    
    if (result->resources.memory_models == NULL) {
      return ERROR_MEMORY;
    }
    
    memcpy(
      result->resources.memory_models,
      base->resources.memory_models,
      base->resources.memory_model_count * sizeof(memory_model_t)
    );
    
    result->resources.memory_model_count = base->resources.memory_model_count;
  }
  
  return ERROR_NONE;
}

/**
 * @brief Merges specialized strategies from base and override configurations
 *
 * @param[in] base Base configuration
 * @param[in] override Override configuration
 * @param[out] result Result configuration
 * @return Error code indicating success or failure
 */
static error_t merge_specialized_strategies(
  const target_config_t* base,
  const target_config_t* override,
  target_config_t* result
) {
  if (base == NULL || override == NULL || result == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* If override has strategies, use those */
  if (override->optimization.strategy_count > 0) {
    result->optimization.specialized_strategies = memory_calloc(
      override->optimization.strategy_count,
      sizeof(char*)
    );
    
    if (result->optimization.specialized_strategies == NULL) {
      return ERROR_MEMORY;
    }
    
    for (uint32_t i = 0; i < override->optimization.strategy_count; i++) {
      result->optimization.specialized_strategies[i] = memory_alloc(
        strlen(override->optimization.specialized_strategies[i]) + 1
      );
      
      if (result->optimization.specialized_strategies[i] == NULL) {
        /* Free previously allocated strategies */
        for (uint32_t j = 0; j < i; j++) {
          memory_free(result->optimization.specialized_strategies[j]);
        }
        memory_free(result->optimization.specialized_strategies);
        result->optimization.specialized_strategies = NULL;
        return ERROR_MEMORY;
      }
      
      strcpy(
        result->optimization.specialized_strategies[i],
        override->optimization.specialized_strategies[i]
      );
    }
    
    result->optimization.strategy_count = override->optimization.strategy_count;
  } else if (base->optimization.strategy_count > 0) {
    result->optimization.specialized_strategies = memory_calloc(
      base->optimization.strategy_count,
      sizeof(char*)
    );
    
    if (result->optimization.specialized_strategies == NULL) {
      return ERROR_MEMORY;
    }
    
    for (uint32_t i = 0; i < base->optimization.strategy_count; i++) {
      result->optimization.specialized_strategies[i] = memory_alloc(
        strlen(base->optimization.specialized_strategies[i]) + 1
      );
      
      if (result->optimization.specialized_strategies[i] == NULL) {
        /* Free previously allocated strategies */
        for (uint32_t j = 0; j < i; j++) {
          memory_free(result->optimization.specialized_strategies[j]);
        }
        memory_free(result->optimization.specialized_strategies);
        result->optimization.specialized_strategies = NULL;
        return ERROR_MEMORY;
      }
      
      strcpy(
        result->optimization.specialized_strategies[i],
        base->optimization.specialized_strategies[i]
      );
    }
    
    result->optimization.strategy_count = base->optimization.strategy_count;
  }
  
  return ERROR_NONE;
}

error_t target_config_merge(
  const target_config_t* base,
  const target_config_t* override,
  target_config_t** result
) {
  if (base == NULL || override == NULL || result == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  *result = NULL;
  
  /* Create empty configuration */
  target_config_t* merged_config = NULL;
  error_t create_result = target_config_create_empty(&merged_config);
  if (create_result != ERROR_NONE) {
    return create_result;
  }
  
  /* Merge general fields */
  error_t name_result = merge_string(base->name, override->name, &merged_config->name);
  if (name_result != ERROR_NONE) {
    target_config_free(merged_config);
    return name_result;
  }
  
  error_t arch_result = merge_string(
    base->architecture,
    override->architecture,
    &merged_config->architecture
  );
  if (arch_result != ERROR_NONE) {
    target_config_free(merged_config);
    return arch_result;
  }
  
  error_t vendor_result = merge_string(
    base->vendor,
    override->vendor,
    &merged_config->vendor
  );
  if (vendor_result != ERROR_NONE) {
    target_config_free(merged_config);
    return vendor_result;
  }
  
  error_t desc_result = merge_string(
    base->description,
    override->description,
    &merged_config->description
  );
  if (desc_result != ERROR_NONE) {
    target_config_free(merged_config);
    return desc_result;
  }
  
  /* Merge features */
  error_t features_result = merge_features(base, override, merged_config);
  if (features_result != ERROR_NONE) {
    target_config_free(merged_config);
    return features_result;
  }
  
  /* Merge resource properties with override taking precedence */
  merged_config->resources.general_registers = 
    override->resources.general_registers ? 
    override->resources.general_registers : 
    base->resources.general_registers;
  
  merged_config->resources.float_registers = 
    override->resources.float_registers ? 
    override->resources.float_registers : 
    base->resources.float_registers;
  
  merged_config->resources.vector_registers = 
    override->resources.vector_registers ? 
    override->resources.vector_registers : 
    base->resources.vector_registers;
  
  merged_config->resources.vector_width = 
    override->resources.vector_width ? 
    override->resources.vector_width : 
    base->resources.vector_width;
  
  merged_config->resources.min_alignment = 
    override->resources.min_alignment ? 
    override->resources.min_alignment : 
    base->resources.min_alignment;
  
  /* Merge memory models */
  error_t models_result = merge_memory_models(base, override, merged_config);
  if (models_result != ERROR_NONE) {
    target_config_free(merged_config);
    return models_result;
  }
  
  /* Merge memory properties */
  merged_config->memory.alignment = 
    override->memory.alignment ? 
    override->memory.alignment : 
    base->memory.alignment;
  
  merged_config->memory.page_size = 
    override->memory.page_size ? 
    override->memory.page_size : 
    base->memory.page_size;
  
  merged_config->memory.cacheline_size = 
    override->memory.cacheline_size ? 
    override->memory.cacheline_size : 
    base->memory.cacheline_size;
  
  /* Merge optimization properties */
  merged_config->optimization.vector_threshold = 
    override->optimization.vector_threshold ? 
    override->optimization.vector_threshold : 
    base->optimization.vector_threshold;
  
  merged_config->optimization.unroll_factor = 
    override->optimization.unroll_factor ? 
    override->optimization.unroll_factor : 
    base->optimization.unroll_factor;
  
  /* For boolean properties, override always takes precedence */
  merged_config->optimization.use_fma = override->optimization.use_fma;
  
  /* Merge specialized strategies */
  error_t strategies_result = merge_specialized_strategies(base, override, merged_config);
  if (strategies_result != ERROR_NONE) {
    target_config_free(merged_config);
    return strategies_result;
  }
  
  /* Validate merged configuration */
  error_t validate_result = target_config_validate(merged_config);
  if (validate_result != ERROR_NONE) {
    log_error("Invalid merged target configuration: %s", error_message(validate_result));
    target_config_free(merged_config);
    return validate_result;
  }
  
  *result = merged_config;
  return ERROR_NONE;
}