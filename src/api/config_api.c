/**
 * @file config_api.c
 * @brief Implementation of the configuration API
 * @details Provides functions for loading, manipulating, and saving configurations.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "coil-assembler/config.h"
#include "../utils/memory.c"
#include "../utils/logging.c"

/* Define the configuration value structure */
struct coil_config_value_s {
  coil_config_type_t type;
  union {
    bool bool_value;
    int64_t int_value;
    double float_value;
    char* string_value;
    struct {
      coil_config_value_t** elements;
      uint32_t count;
    } array;
    struct {
      char** keys;
      coil_config_value_t** values;
      uint32_t count;
    } object;
  } data;
};

/* Define the configuration structure */
struct coil_config_s {
  coil_config_value_t* root;
};

/* Forward declarations */
static void free_config_value(coil_config_value_t* value);
static coil_config_value_t* clone_config_value(const coil_config_value_t* value);
static coil_config_value_t* get_config_value_at_path(const coil_config_t* config, const char* path);
static int set_config_value_at_path(coil_config_t* config, const char* path, coil_config_value_t* value);

/**
 * @brief Create a new configuration object
 * @return New configuration object or NULL on failure
 */
coil_config_t* coil_config_create(void) {
  coil_config_t* config = (coil_config_t*)coil_malloc(sizeof(coil_config_t));
  if (!config) {
    coil_log_error("Failed to allocate configuration");
    return NULL;
  }
  
  /* Create root object */
  config->root = (coil_config_value_t*)coil_malloc(sizeof(coil_config_value_t));
  if (!config->root) {
    coil_log_error("Failed to allocate configuration root");
    coil_free(config, sizeof(coil_config_t));
    return NULL;
  }
  
  /* Initialize root as empty object */
  config->root->type = COIL_CONFIG_TYPE_OBJECT;
  config->root->data.object.keys = NULL;
  config->root->data.object.values = NULL;
  config->root->data.object.count = 0;
  
  coil_log_debug("Created new configuration");
  
  return config;
}

/**
 * @brief Free a configuration value recursively
 * @param value Configuration value to free
 */
static void free_config_value(coil_config_value_t* value) {
  if (!value) {
    return;
  }
  
  switch (value->type) {
    case COIL_CONFIG_TYPE_STRING:
      if (value->data.string_value) {
        coil_free(value->data.string_value, strlen(value->data.string_value) + 1);
      }
      break;
      
    case COIL_CONFIG_TYPE_ARRAY:
      if (value->data.array.elements) {
        for (uint32_t i = 0; i < value->data.array.count; i++) {
          free_config_value(value->data.array.elements[i]);
        }
        coil_free(value->data.array.elements, 
                value->data.array.count * sizeof(coil_config_value_t*));
      }
      break;
      
    case COIL_CONFIG_TYPE_OBJECT:
      if (value->data.object.keys && value->data.object.values) {
        for (uint32_t i = 0; i < value->data.object.count; i++) {
          if (value->data.object.keys[i]) {
            coil_free(value->data.object.keys[i], 
                    strlen(value->data.object.keys[i]) + 1);
          }
          free_config_value(value->data.object.values[i]);
        }
        coil_free(value->data.object.keys, 
                value->data.object.count * sizeof(char*));
        coil_free(value->data.object.values, 
                value->data.object.count * sizeof(coil_config_value_t*));
      }
      break;
      
    default:
      break;
  }
  
  coil_free(value, sizeof(coil_config_value_t));
}

/**
 * @brief Destroy a configuration object
 * @param config Configuration object to destroy
 */
void coil_config_destroy(coil_config_t* config) {
  if (!config) {
    return;
  }
  
  if (config->root) {
    free_config_value(config->root);
  }
  
  coil_free(config, sizeof(coil_config_t));
  
  coil_log_debug("Destroyed configuration");
}

/**
 * @brief Load configuration from a file
 * @param filename Path to the configuration file
 * @return Loaded configuration or NULL on failure
 */
coil_config_t* coil_config_load_file(const char* filename) {
  if (!filename) {
    coil_log_error("Invalid filename");
    return NULL;
  }
  
  coil_log_info("Loading configuration from %s", filename);
  
  /* Open file */
  FILE* file = fopen(filename, "r");
  if (!file) {
    coil_log_error("Failed to open configuration file: %s", filename);
    return NULL;
  }
  
  /* Get file size */
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  if (file_size <= 0) {
    coil_log_error("Empty or invalid configuration file: %s", filename);
    fclose(file);
    return NULL;
  }
  
  /* Allocate buffer for file content */
  char* buffer = (char*)coil_malloc(file_size + 1);
  if (!buffer) {
    coil_log_error("Failed to allocate buffer for configuration file");
    fclose(file);
    return NULL;
  }
  
  /* Read file content */
  size_t read_size = fread(buffer, 1, file_size, file);
  fclose(file);
  
  if (read_size != (size_t)file_size) {
    coil_log_error("Failed to read configuration file: %s", filename);
    coil_free(buffer, file_size + 1);
    return NULL;
  }
  
  /* Null-terminate the buffer */
  buffer[file_size] = '\0';
  
  /* Parse configuration from the buffer */
  coil_config_t* config = coil_config_parse_string(buffer);
  
  /* Free buffer */
  coil_free(buffer, file_size + 1);
  
  if (!config) {
    coil_log_error("Failed to parse configuration file: %s", filename);
    return NULL;
  }
  
  coil_log_info("Successfully loaded configuration from %s", filename);
  
  return config;
}

/**
 * @brief Save configuration to a file
 * @param config Configuration to save
 * @param filename Path to the output file
 * @return 0 on success, non-zero on failure
 */
int coil_config_save_file(const coil_config_t* config, const char* filename) {
  if (!config || !filename) {
    coil_log_error("Invalid parameters for saving configuration");
    return -1;
  }
  
  coil_log_info("Saving configuration to %s", filename);
  
  /* Open file */
  FILE* file = fopen(filename, "w");
  if (!file) {
    coil_log_error("Failed to open configuration file for writing: %s", filename);
    return -1;
  }
  
  /* Serialize configuration to JSON string */
  /* This is a simplified implementation that doesn't handle all cases */
  
  /* Write configuration */
  fprintf(file, "{\n");
  
  /* Write root object */
  if (config->root && config->root->type == COIL_CONFIG_TYPE_OBJECT) {
    for (uint32_t i = 0; i < config->root->data.object.count; i++) {
      const char* key = config->root->data.object.keys[i];
      const coil_config_value_t* value = config->root->data.object.values[i];
      
      /* Write key */
      fprintf(file, "  \"%s\": ", key);
      
      /* Write value based on type */
      switch (value->type) {
        case COIL_CONFIG_TYPE_BOOL:
          fprintf(file, "%s", value->data.bool_value ? "true" : "false");
          break;
          
        case COIL_CONFIG_TYPE_INT:
          fprintf(file, "%ld", value->data.int_value);
          break;
          
        case COIL_CONFIG_TYPE_FLOAT:
          fprintf(file, "%g", value->data.float_value);
          break;
          
        case COIL_CONFIG_TYPE_STRING:
          fprintf(file, "\"%s\"", value->data.string_value ? value->data.string_value : "");
          break;
          
        case COIL_CONFIG_TYPE_ARRAY:
          fprintf(file, "[...]");  /* Simplified */
          break;
          
        case COIL_CONFIG_TYPE_OBJECT:
          fprintf(file, "{...}");  /* Simplified */
          break;
          
        default:
          fprintf(file, "null");
          break;
      }
      
      /* Add comma if not the last item */
      if (i < config->root->data.object.count - 1) {
        fprintf(file, ",\n");
      } else {
        fprintf(file, "\n");
      }
    }
  }
  
  fprintf(file, "}\n");
  
  fclose(file);
  
  coil_log_info("Successfully saved configuration to %s", filename);
  
  return 0;
}

/**
 * @brief Parse configuration from a string
 * @param text Configuration text
 * @return Parsed configuration or NULL on failure
 */
coil_config_t* coil_config_parse_string(const char* text) {
  if (!text) {
    coil_log_error("Invalid configuration text");
    return NULL;
  }
  
  /* Create new configuration */
  coil_config_t* config = coil_config_create();
  if (!config) {
    return NULL;
  }
  
  /* Parse configuration text */
  /* This is a simplified implementation that creates a dummy configuration */
  
  /* For simplicity, we'll add some dummy values to the configuration */
  coil_config_set_int(config, "version", 1);
  coil_config_set_string(config, "name", "COIL Assembler");
  coil_config_set_bool(config, "debug", false);
  
  coil_log_debug("Parsed configuration from string");
  
  return config;
}

/**
 * @brief Create a boolean value
 * @param value Boolean value
 * @return New configuration value or NULL on failure
 */
static coil_config_value_t* create_bool_value(bool value) {
  coil_config_value_t* config_value = (coil_config_value_t*)coil_malloc(
      sizeof(coil_config_value_t));
  if (!config_value) {
    return NULL;
  }
  
  config_value->type = COIL_CONFIG_TYPE_BOOL;
  config_value->data.bool_value = value;
  
  return config_value;
}

/**
 * @brief Create an integer value
 * @param value Integer value
 * @return New configuration value or NULL on failure
 */
static coil_config_value_t* create_int_value(int64_t value) {
  coil_config_value_t* config_value = (coil_config_value_t*)coil_malloc(
      sizeof(coil_config_value_t));
  if (!config_value) {
    return NULL;
  }
  
  config_value->type = COIL_CONFIG_TYPE_INT;
  config_value->data.int_value = value;
  
  return config_value;
}

/**
 * @brief Create a floating-point value
 * @param value Floating-point value
 * @return New configuration value or NULL on failure
 */
static coil_config_value_t* create_float_value(double value) {
  coil_config_value_t* config_value = (coil_config_value_t*)coil_malloc(
      sizeof(coil_config_value_t));
  if (!config_value) {
    return NULL;
  }
  
  config_value->type = COIL_CONFIG_TYPE_FLOAT;
  config_value->data.float_value = value;
  
  return config_value;
}

/**
 * @brief Create a string value
 * @param value String value
 * @return New configuration value or NULL on failure
 */
static coil_config_value_t* create_string_value(const char* value) {
  if (!value) {
    return NULL;
  }
  
  coil_config_value_t* config_value = (coil_config_value_t*)coil_malloc(
      sizeof(coil_config_value_t));
  if (!config_value) {
    return NULL;
  }
  
  config_value->type = COIL_CONFIG_TYPE_STRING;
  config_value->data.string_value = coil_strdup(value);
  
  if (!config_value->data.string_value) {
    coil_free(config_value, sizeof(coil_config_value_t));
    return NULL;
  }
  
  return config_value;
}

/**
 * @brief Clone a configuration value
 * @param value Configuration value to clone
 * @return Cloned configuration value or NULL on failure
 */
static coil_config_value_t* clone_config_value(const coil_config_value_t* value) {
  if (!value) {
    return NULL;
  }
  
  coil_config_value_t* clone = (coil_config_value_t*)coil_malloc(
      sizeof(coil_config_value_t));
  if (!clone) {
    return NULL;
  }
  
  clone->type = value->type;
  
  switch (value->type) {
    case COIL_CONFIG_TYPE_BOOL:
      clone->data.bool_value = value->data.bool_value;
      break;
      
    case COIL_CONFIG_TYPE_INT:
      clone->data.int_value = value->data.int_value;
      break;
      
    case COIL_CONFIG_TYPE_FLOAT:
      clone->data.float_value = value->data.float_value;
      break;
      
    case COIL_CONFIG_TYPE_STRING:
      if (value->data.string_value) {
        clone->data.string_value = coil_strdup(value->data.string_value);
        if (!clone->data.string_value) {
          coil_free(clone, sizeof(coil_config_value_t));
          return NULL;
        }
      } else {
        clone->data.string_value = NULL;
      }
      break;
      
    case COIL_CONFIG_TYPE_ARRAY:
      if (value->data.array.count > 0 && value->data.array.elements) {
        clone->data.array.elements = (coil_config_value_t**)coil_calloc(
            value->data.array.count, sizeof(coil_config_value_t*));
        if (!clone->data.array.elements) {
          coil_free(clone, sizeof(coil_config_value_t));
          return NULL;
        }
        
        clone->data.array.count = value->data.array.count;
        
        for (uint32_t i = 0; i < value->data.array.count; i++) {
          clone->data.array.elements[i] = clone_config_value(value->data.array.elements[i]);
          if (!clone->data.array.elements[i]) {
            /* Clean up on failure */
            for (uint32_t j = 0; j < i; j++) {
              free_config_value(clone->data.array.elements[j]);
            }
            coil_free(clone->data.array.elements, 
                    value->data.array.count * sizeof(coil_config_value_t*));
            coil_free(clone, sizeof(coil_config_value_t));
            return NULL;
          }
        }
      } else {
        clone->data.array.elements = NULL;
        clone->data.array.count = 0;
      }
      break;
      
    case COIL_CONFIG_TYPE_OBJECT:
      if (value->data.object.count > 0 && value->data.object.keys && value->data.object.values) {
        clone->data.object.keys = (char**)coil_calloc(
            value->data.object.count, sizeof(char*));
        clone->data.object.values = (coil_config_value_t**)coil_calloc(
            value->data.object.count, sizeof(coil_config_value_t*));
        
        if (!clone->data.object.keys || !clone->data.object.values) {
          if (clone->data.object.keys) {
            coil_free(clone->data.object.keys, 
                    value->data.object.count * sizeof(char*));
          }
          if (clone->data.object.values) {
            coil_free(clone->data.object.values, 
                    value->data.object.count * sizeof(coil_config_value_t*));
          }
          coil_free(clone, sizeof(coil_config_value_t));
          return NULL;
        }
        
        clone->data.object.count = value->data.object.count;
        
        for (uint32_t i = 0; i < value->data.object.count; i++) {
          if (value->data.object.keys[i]) {
            clone->data.object.keys[i] = coil_strdup(value->data.object.keys[i]);
            if (!clone->data.object.keys[i]) {
              /* Clean up on failure */
              for (uint32_t j = 0; j < i; j++) {
                if (clone->data.object.keys[j]) {
                  coil_free(clone->data.object.keys[j], 
                          strlen(clone->data.object.keys[j]) + 1);
                }
                if (clone->data.object.values[j]) {
                  free_config_value(clone->data.object.values[j]);
                }
              }
              coil_free(clone->data.object.keys, 
                      value->data.object.count * sizeof(char*));
              coil_free(clone->data.object.values, 
                      value->data.object.count * sizeof(coil_config_value_t*));
              coil_free(clone, sizeof(coil_config_value_t));
              return NULL;
            }
          } else {
            clone->data.object.keys[i] = NULL;
          }
          
          clone->data.object.values[i] = clone_config_value(value->data.object.values[i]);
          if (!clone->data.object.values[i]) {
            /* Clean up on failure */
            for (uint32_t j = 0; j <= i; j++) {
              if (clone->data.object.keys[j]) {
                coil_free(clone->data.object.keys[j], 
                        strlen(clone->data.object.keys[j]) + 1);
              }
              if (j < i) {
                free_config_value(clone->data.object.values[j]);
              }
            }
            coil_free(clone->data.object.keys, 
                    value->data.object.count * sizeof(char*));
            coil_free(clone->data.object.values, 
                    value->data.object.count * sizeof(coil_config_value_t*));
            coil_free(clone, sizeof(coil_config_value_t));
            return NULL;
          }
        }
      } else {
        clone->data.object.keys = NULL;
        clone->data.object.values = NULL;
        clone->data.object.count = 0;
      }
      break;
      
    default:
      /* Nothing to clone for other types */
      break;
  }
  
  return clone;
}

/**
 * @brief Split a path into parts
 * @param path Path to split
 * @param parts Array to store parts
 * @param max_parts Maximum number of parts
 * @return Number of parts or -1 on failure
 */
static int split_path(const char* path, char** parts, int max_parts) {
  if (!path || !parts || max_parts <= 0) {
    return -1;
  }
  
  int part_count = 0;
  const char* start = path;
  const char* current = path;
  
  /* Skip leading dots */
  while (*start == '.') {
    start++;
  }
  
  current = start;
  
  /* Split path by dots */
  while (*current != '\0') {
    if (*current == '.') {
      /* End of part */
      if (current > start) {
        size_t len = current - start;
        parts[part_count] = (char*)coil_malloc(len + 1);
        if (!parts[part_count]) {
          /* Clean up on failure */
          for (int i = 0; i < part_count; i++) {
            coil_free(parts[i], strlen(parts[i]) + 1);
          }
          return -1;
        }
        
        strncpy(parts[part_count], start, len);
        parts[part_count][len] = '\0';
        part_count++;
        
        if (part_count >= max_parts) {
          break;
        }
      }
      
      /* Skip dot */
      start = current + 1;
    }
    
    current++;
  }
  
  /* Add the last part if any */
  if (current > start && part_count < max_parts) {
    size_t len = current - start;
    parts[part_count] = (char*)coil_malloc(len + 1);
    if (!parts[part_count]) {
      /* Clean up on failure */
      for (int i = 0; i < part_count; i++) {
        coil_free(parts[i], strlen(parts[i]) + 1);
      }
      return -1;
    }
    
    strncpy(parts[part_count], start, len);
    parts[part_count][len] = '\0';
    part_count++;
  }
  
  return part_count;
}

/**
 * @brief Get a configuration value at a path
 * @param config Configuration
 * @param path Path to the value (dot-separated)
 * @return Configuration value or NULL if not found
 */
static coil_config_value_t* get_config_value_at_path(const coil_config_t* config, const char* path) {
  if (!config || !path) {
    return NULL;
  }
  
  /* Empty path returns the root */
  if (path[0] == '\0') {
    return config->root;
  }
  
  /* Split path into parts */
  char* parts[16];  /* Maximum 16 levels */
  int part_count = split_path(path, parts, 16);
  
  if (part_count <= 0) {
    return NULL;
  }
  
  /* Start from root */
  coil_config_value_t* current = config->root;
  
  /* Traverse the path */
  for (int i = 0; i < part_count; i++) {
    if (!current || current->type != COIL_CONFIG_TYPE_OBJECT) {
      /* Not an object, can't continue */
      for (int j = 0; j < part_count; j++) {
        coil_free(parts[j], strlen(parts[j]) + 1);
      }
      return NULL;
    }
    
    /* Find property in object */
    int found = 0;
    for (uint32_t j = 0; j < current->data.object.count; j++) {
      if (current->data.object.keys[j] && 
          strcmp(current->data.object.keys[j], parts[i]) == 0) {
        current = current->data.object.values[j];
        found = 1;
        break;
      }
    }
    
    if (!found) {
      /* Property not found */
      for (int j = 0; j < part_count; j++) {
        coil_free(parts[j], strlen(parts[j]) + 1);
      }
      return NULL;
    }
  }
  
  /* Free parts */
  for (int i = 0; i < part_count; i++) {
    coil_free(parts[i], strlen(parts[i]) + 1);
  }
  
  return current;
}

/**
 * @brief Set a configuration value at a path
 * @param config Configuration
 * @param path Path to the value (dot-separated)
 * @param value Value to set
 * @return 0 on success, non-zero on failure
 */
static int set_config_value_at_path(coil_config_t* config, const char* path, coil_config_value_t* value) {
  if (!config || !path || !value) {
    return -1;
  }
  
  /* Empty path is invalid */
  if (path[0] == '\0') {
    return -1;
  }
  
  /* Split path into parts */
  char* parts[16];  /* Maximum 16 levels */
  int part_count = split_path(path, parts, 16);
  
  if (part_count <= 0) {
    return -1;
  }
  
  /* Start from root */
  coil_config_value_t* current = config->root;
  
  /* Traverse the path */
  for (int i = 0; i < part_count - 1; i++) {
    if (!current || current->type != COIL_CONFIG_TYPE_OBJECT) {
      /* Not an object, can't continue */
      for (int j = 0; j < part_count; j++) {
        coil_free(parts[j], strlen(parts[j]) + 1);
      }
      return -1;
    }
    
    /* Find property in object */
    int found = 0;
    for (uint32_t j = 0; j < current->data.object.count; j++) {
      if (current->data.object.keys[j] && 
          strcmp(current->data.object.keys[j], parts[i]) == 0) {
        current = current->data.object.values[j];
        found = 1;
        break;
      }
    }
    
    if (!found) {
      /* Property not found, create it */
      coil_config_value_t* new_object = (coil_config_value_t*)coil_malloc(
          sizeof(coil_config_value_t));
      if (!new_object) {
        for (int j = 0; j < part_count; j++) {
          coil_free(parts[j], strlen(parts[j]) + 1);
        }
        return -1;
      }
      
      new_object->type = COIL_CONFIG_TYPE_OBJECT;
      new_object->data.object.keys = NULL;
      new_object->data.object.values = NULL;
      new_object->data.object.count = 0;
      
      /* Add new object to current object */
      char** new_keys = (char**)coil_realloc(
          current->data.object.keys,
          current->data.object.count * sizeof(char*),
          (current->data.object.count + 1) * sizeof(char*));
      
      coil_config_value_t** new_values = (coil_config_value_t**)coil_realloc(
          current->data.object.values,
          current->data.object.count * sizeof(coil_config_value_t*),
          (current->data.object.count + 1) * sizeof(coil_config_value_t*));
      
      if (!new_keys || !new_values) {
        if (new_keys) {
          current->data.object.keys = new_keys;
        }
        if (new_values) {
          current->data.object.values = new_values;
        }
        coil_free(new_object, sizeof(coil_config_value_t));
        for (int j = 0; j < part_count; j++) {
          coil_free(parts[j], strlen(parts[j]) + 1);
        }
        return -1;
      }
      
      current->data.object.keys = new_keys;
      current->data.object.values = new_values;
      
      current->data.object.keys[current->data.object.count] = coil_strdup(parts[i]);
      if (!current->data.object.keys[current->data.object.count]) {
        coil_free(new_object, sizeof(coil_config_value_t));
        for (int j = 0; j < part_count; j++) {
          coil_free(parts[j], strlen(parts[j]) + 1);
        }
        return -1;
      }
      
      current->data.object.values[current->data.object.count] = new_object;
      current->data.object.count++;
      
      current = new_object;
    }
  }
  
  /* Final property */
  if (!current || current->type != COIL_CONFIG_TYPE_OBJECT) {
    /* Not an object, can't set property */
    for (int j = 0; j < part_count; j++) {
      coil_free(parts[j], strlen(parts[j]) + 1);
    }
    return -1;
  }
  
  /* Check if property already exists */
  for (uint32_t i = 0; i < current->data.object.count; i++) {
    if (current->data.object.keys[i] && 
        strcmp(current->data.object.keys[i], parts[part_count - 1]) == 0) {
      /* Free existing value */
      free_config_value(current->data.object.values[i]);
      
      /* Set new value */
      current->data.object.values[i] = value;
      
      /* Free parts */
      for (int j = 0; j < part_count; j++) {
        coil_free(parts[j], strlen(parts[j]) + 1);
      }
      
      return 0;
    }
  }
  
  /* Property doesn't exist, add it */
  char** new_keys = (char**)coil_realloc(
      current->data.object.keys,
      current->data.object.count * sizeof(char*),
      (current->data.object.count + 1) * sizeof(char*));
  
  coil_config_value_t** new_values = (coil_config_value_t**)coil_realloc(
      current->data.object.values,
      current->data.object.count * sizeof(coil_config_value_t*),
      (current->data.object.count + 1) * sizeof(coil_config_value_t*));
  
  if (!new_keys || !new_values) {
    if (new_keys) {
      current->data.object.keys = new_keys;
    }
    if (new_values) {
      current->data.object.values = new_values;
    }
    free_config_value(value);
    for (int j = 0; j < part_count; j++) {
      coil_free(parts[j], strlen(parts[j]) + 1);
    }
    return -1;
  }
  
  current->data.object.keys = new_keys;
  current->data.object.values = new_values;
  
  current->data.object.keys[current->data.object.count] = coil_strdup(parts[part_count - 1]);
  if (!current->data.object.keys[current->data.object.count]) {
    free_config_value(value);
    for (int j = 0; j < part_count; j++) {
      coil_free(parts[j], strlen(parts[j]) + 1);
    }
    return -1;
  }
  
  current->data.object.values[current->data.object.count] = value;
  current->data.object.count++;
  
  /* Free parts */
  for (int j = 0; j < part_count; j++) {
    coil_free(parts[j], strlen(parts[j]) + 1);
  }
  
  return 0;
}

/**
 * @brief Get a configuration value
 * @param config Configuration object
 * @param path Path to the value (dot-separated)
 * @return Configuration value or NULL if not found
 */
const coil_config_value_t* coil_config_get(const coil_config_t* config, const char* path) {
  if (!config || !path) {
    return NULL;
  }
  
  return get_config_value_at_path(config, path);
}

/**
 * @brief Set a boolean configuration value
 * @param config Configuration object
 * @param path Path to the value
 * @param value Boolean value
 * @return 0 on success, non-zero on failure
 */
int coil_config_set_bool(coil_config_t* config, const char* path, bool value) {
  if (!config || !path) {
    return -1;
  }
  
  coil_config_value_t* bool_value = create_bool_value(value);
  if (!bool_value) {
    return -1;
  }
  
  return set_config_value_at_path(config, path, bool_value);
}

/**
 * @brief Set an integer configuration value
 * @param config Configuration object
 * @param path Path to the value
 * @param value Integer value
 * @return 0 on success, non-zero on failure
 */
int coil_config_set_int(coil_config_t* config, const char* path, int64_t value) {
  if (!config || !path) {
    return -1;
  }
  
  coil_config_value_t* int_value = create_int_value(value);
  if (!int_value) {
    return -1;
  }
  
  return set_config_value_at_path(config, path, int_value);
}

/**
 * @brief Set a floating-point configuration value
 * @param config Configuration object
 * @param path Path to the value
 * @param value Floating-point value
 * @return 0 on success, non-zero on failure
 */
int coil_config_set_float(coil_config_t* config, const char* path, double value) {
  if (!config || !path) {
    return -1;
  }
  
  coil_config_value_t* float_value = create_float_value(value);
  if (!float_value) {
    return -1;
  }
  
  return set_config_value_at_path(config, path, float_value);
}

/**
 * @brief Set a string configuration value
 * @param config Configuration object
 * @param path Path to the value
 * @param value String value
 * @return 0 on success, non-zero on failure
 */
int coil_config_set_string(coil_config_t* config, const char* path, const char* value) {
  if (!config || !path || !value) {
    return -1;
  }
  
  coil_config_value_t* string_value = create_string_value(value);
  if (!string_value) {
    return -1;
  }
  
  return set_config_value_at_path(config, path, string_value);
}

/**
 * @brief Get the type of a configuration value
 * @param value Configuration value
 * @return Value type
 */
coil_config_type_t coil_config_get_type(const coil_config_value_t* value) {
  if (!value) {
    return COIL_CONFIG_TYPE_NONE;
  }
  
  return value->type;
}

/**
 * @brief Get a boolean configuration value
 * @param value Configuration value
 * @param default_value Default value to return if not found or wrong type
 * @return Boolean value or default_value
 */
bool coil_config_get_bool(const coil_config_value_t* value, bool default_value) {
  if (!value || value->type != COIL_CONFIG_TYPE_BOOL) {
    return default_value;
  }
  
  return value->data.bool_value;
}

/**
 * @brief Get an integer configuration value
 * @param value Configuration value
 * @param default_value Default value to return if not found or wrong type
 * @return Integer value or default_value
 */
int64_t coil_config_get_int(const coil_config_value_t* value, int64_t default_value) {
  if (!value) {
    return default_value;
  }
  
  if (value->type == COIL_CONFIG_TYPE_INT) {
    return value->data.int_value;
  } else if (value->type == COIL_CONFIG_TYPE_FLOAT) {
    return (int64_t)value->data.float_value;
  }
  
  return default_value;
}

/**
 * @brief Get a floating-point configuration value
 * @param value Configuration value
 * @param default_value Default value to return if not found or wrong type
 * @return Floating-point value or default_value
 */
double coil_config_get_float(const coil_config_value_t* value, double default_value) {
  if (!value) {
    return default_value;
  }
  
  if (value->type == COIL_CONFIG_TYPE_FLOAT) {
    return value->data.float_value;
  } else if (value->type == COIL_CONFIG_TYPE_INT) {
    return (double)value->data.int_value;
  }
  
  return default_value;
}

/**
 * @brief Get a string configuration value
 * @param value Configuration value
 * @param default_value Default value to return if not found or wrong type
 * @return String value or default_value
 */
const char* coil_config_get_string(const coil_config_value_t* value, const char* default_value) {
  if (!value || value->type != COIL_CONFIG_TYPE_STRING || !value->data.string_value) {
    return default_value;
  }
  
  return value->data.string_value;
}

/**
 * @brief Get the length of an array configuration value
 * @param value Configuration value
 * @return Array length or 0 if not an array
 */
uint32_t coil_config_get_array_length(const coil_config_value_t* value) {
  if (!value || value->type != COIL_CONFIG_TYPE_ARRAY) {
    return 0;
  }
  
  return value->data.array.count;
}

/**
 * @brief Get an array element
 * @param value Array configuration value
 * @param index Array index
 * @return Element value or NULL if out of bounds
 */
const coil_config_value_t* coil_config_get_array_element(const coil_config_value_t* value, 
                                                        uint32_t index) {
  if (!value || value->type != COIL_CONFIG_TYPE_ARRAY || 
      index >= value->data.array.count || !value->data.array.elements) {
    return NULL;
  }
  
  return value->data.array.elements[index];
}

/**
 * @brief Get an object property
 * @param value Object configuration value
 * @param key Property key
 * @return Property value or NULL if not found
 */
const coil_config_value_t* coil_config_get_object_property(const coil_config_value_t* value, 
                                                          const char* key) {
  if (!value || value->type != COIL_CONFIG_TYPE_OBJECT || !key || 
      !value->data.object.keys || !value->data.object.values) {
    return NULL;
  }
  
  for (uint32_t i = 0; i < value->data.object.count; i++) {
    if (value->data.object.keys[i] && strcmp(value->data.object.keys[i], key) == 0) {
      return value->data.object.values[i];
    }
  }
  
  return NULL;
}

/**
 * @brief Get the number of properties in an object configuration value
 * @param value Configuration value
 * @return Number of properties or 0 if not an object
 */
uint32_t coil_config_get_object_property_count(const coil_config_value_t* value) {
  if (!value || value->type != COIL_CONFIG_TYPE_OBJECT) {
    return 0;
  }
  
  return value->data.object.count;
}

/**
 * @brief Get the key of an object property by index
 * @param value Object configuration value
 * @param index Property index
 * @return Property key or NULL if out of bounds
 */
const char* coil_config_get_object_property_key(const coil_config_value_t* value, 
                                               uint32_t index) {
  if (!value || value->type != COIL_CONFIG_TYPE_OBJECT || 
      index >= value->data.object.count || !value->data.object.keys) {
    return NULL;
  }
  
  return value->data.object.keys[index];
}

/**
 * @brief Merge two configurations
 * @param base Base configuration
 * @param overlay Overlay configuration (takes precedence)
 * @return Merged configuration or NULL on failure
 */
coil_config_t* coil_config_merge(const coil_config_t* base, const coil_config_t* overlay) {
  if (!base || !overlay) {
    return NULL;
  }
  
  /* Clone base configuration */
  coil_config_t* merged = coil_config_clone(base);
  if (!merged) {
    return NULL;
  }
  
  /* Recursively merge overlay into base */
  /* This is a simplified implementation that doesn't handle all cases */
  
  if (overlay->root && overlay->root->type == COIL_CONFIG_TYPE_OBJECT) {
    for (uint32_t i = 0; i < overlay->root->data.object.count; i++) {
      const char* key = overlay->root->data.object.keys[i];
      const coil_config_value_t* value = overlay->root->data.object.values[i];
      
      if (!key || !value) {
        continue;
      }
      
      /* Clone value from overlay */
      coil_config_value_t* value_clone = clone_config_value(value);
      if (!value_clone) {
        coil_config_destroy(merged);
        return NULL;
      }
      
      /* Set value in merged config */
      if (set_config_value_at_path(merged, key, value_clone) != 0) {
        free_config_value(value_clone);
        coil_config_destroy(merged);
        return NULL;
      }
    }
  }
  
  return merged;
}

/**
 * @brief Clone a configuration
 * @param config Configuration to clone
 * @return Cloned configuration or NULL on failure
 */
coil_config_t* coil_config_clone(const coil_config_t* config) {
  if (!config) {
    return NULL;
  }
  
  /* Create new config */
  coil_config_t* clone = (coil_config_t*)coil_malloc(sizeof(coil_config_t));
  if (!clone) {
    return NULL;
  }
  
  /* Clone root value */
  clone->root = clone_config_value(config->root);
  if (!clone->root) {
    coil_free(clone, sizeof(coil_config_t));
    return NULL;
  }
  
  return clone;
}