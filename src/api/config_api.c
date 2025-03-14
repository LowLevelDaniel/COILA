/**
 * @file config_api.c
 * @brief Implementation of the configuration API
 * @details Provides functions for loading, manipulating, and saving configurations.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "coil-assembler/config.h"
#include "../utils/memory.c"

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

/**
 * @brief Create a new configuration object
 * @return New configuration object or NULL on failure
 */
coil_config_t* coil_config_create(void) {
  coil_config_t* config = (coil_config_t*)coil_malloc(sizeof(coil_config_t));
  if (!config) {
    return NULL;
  }
  
  /* Create root object */
  config->root = (coil_config_value_t*)coil_malloc(sizeof(coil_config_value_t));
  if (!config->root) {
    coil_free(config, sizeof(coil_config_t));
    return NULL;
  }
  
  /* Initialize root as empty object */
  config->root->type = COIL_CONFIG_TYPE_OBJECT;
  config->root->data.object.keys = NULL;
  config->root->data.object.values = NULL;
  config->root->data.object.count = 0;
  
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
}