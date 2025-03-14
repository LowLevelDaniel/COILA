/**
 * @file config.h
 * @brief Configuration interface
 * @details This file defines the configuration interface for the COIL assembler,
 *          allowing customization of target properties and optimization parameters.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_CONFIG_H
#define COIL_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Configuration value types
 */
typedef enum {
  COIL_CONFIG_TYPE_NONE = 0,   /**< No value */
  COIL_CONFIG_TYPE_BOOL = 1,   /**< Boolean value */
  COIL_CONFIG_TYPE_INT = 2,    /**< Integer value */
  COIL_CONFIG_TYPE_FLOAT = 3,  /**< Floating-point value */
  COIL_CONFIG_TYPE_STRING = 4, /**< String value */
  COIL_CONFIG_TYPE_ARRAY = 5,  /**< Array of values */
  COIL_CONFIG_TYPE_OBJECT = 6  /**< Object with named properties */
} coil_config_type_t;

/**
 * @brief Forward declaration of configuration value
 */
typedef struct coil_config_value_s coil_config_value_t;

/**
 * @brief Forward declaration of configuration object
 */
typedef struct coil_config_s coil_config_t;

/**
 * @brief Create a new configuration object
 * @return New configuration object or NULL on failure
 */
coil_config_t* coil_config_create(void);

/**
 * @brief Destroy a configuration object
 * @param config Configuration object to destroy
 */
void coil_config_destroy(coil_config_t *config);

/**
 * @brief Load configuration from a file
 * @param filename Path to the configuration file
 * @return Loaded configuration or NULL on failure
 */
coil_config_t* coil_config_load_file(const char *filename);

/**
 * @brief Save configuration to a file
 * @param config Configuration to save
 * @param filename Path to the output file
 * @return 0 on success, non-zero on failure
 */
int coil_config_save_file(const coil_config_t *config, const char *filename);

/**
 * @brief Parse configuration from a string
 * @param text Configuration text
 * @return Parsed configuration or NULL on failure
 */
coil_config_t* coil_config_parse_string(const char *text);

/**
 * @brief Get a configuration value
 * @param config Configuration object
 * @param path Path to the value (dot-separated)
 * @return Configuration value or NULL if not found
 */
const coil_config_value_t* coil_config_get(const coil_config_t *config, const char *path);

/**
 * @brief Set a boolean configuration value
 * @param config Configuration object
 * @param path Path to the value
 * @param value Boolean value
 * @return 0 on success, non-zero on failure
 */
int coil_config_set_bool(coil_config_t *config, const char *path, bool value);

/**
 * @brief Set an integer configuration value
 * @param config Configuration object
 * @param path Path to the value
 * @param value Integer value
 * @return 0 on success, non-zero on failure
 */
int coil_config_set_int(coil_config_t *config, const char *path, int64_t value);

/**
 * @brief Set a floating-point configuration value
 * @param config Configuration object
 * @param path Path to the value
 * @param value Floating-point value
 * @return 0 on success, non-zero on failure
 */
int coil_config_set_float(coil_config_t *config, const char *path, double value);

/**
 * @brief Set a string configuration value
 * @param config Configuration object
 * @param path Path to the value
 * @param value String value
 * @return 0 on success, non-zero on failure
 */
int coil_config_set_string(coil_config_t *config, const char *path, const char *value);

/**
 * @brief Get the type of a configuration value
 * @param value Configuration value
 * @return Value type
 */
coil_config_type_t coil_config_get_type(const coil_config_value_t *value);

/**
 * @brief Get a boolean configuration value
 * @param value Configuration value
 * @param default_value Default value to return if not found or wrong type
 * @return Boolean value or default_value
 */
bool coil_config_get_bool(const coil_config_value_t *value, bool default_value);

/**
 * @brief Get an integer configuration value
 * @param value Configuration value
 * @param default_value Default value to return if not found or wrong type
 * @return Integer value or default_value
 */
int64_t coil_config_get_int(const coil_config_value_t *value, int64_t default_value);

/**
 * @brief Get a floating-point configuration value
 * @param value Configuration value
 * @param default_value Default value to return if not found or wrong type
 * @return Floating-point value or default_value
 */
double coil_config_get_float(const coil_config_value_t *value, double default_value);

/**
 * @brief Get a string configuration value
 * @param value Configuration value
 * @param default_value Default value to return if not found or wrong type
 * @return String value or default_value
 */
const char* coil_config_get_string(const coil_config_value_t *value, const char *default_value);

/**
 * @brief Get the length of an array configuration value
 * @param value Configuration value
 * @return Array length or 0 if not an array
 */
uint32_t coil_config_get_array_length(const coil_config_value_t *value);

/**
 * @brief Get an array element
 * @param value Array configuration value
 * @param index Array index
 * @return Element value or NULL if out of bounds
 */
const coil_config_value_t* coil_config_get_array_element(const coil_config_value_t *value, 
                                                        uint32_t index);

/**
 * @brief Get an object property
 * @param value Object configuration value
 * @param key Property key
 * @return Property value or NULL if not found
 */
const coil_config_value_t* coil_config_get_object_property(const coil_config_value_t *value, 
                                                          const char *key);

/**
 * @brief Get the number of properties in an object configuration value
 * @param value Configuration value
 * @return Number of properties or 0 if not an object
 */
uint32_t coil_config_get_object_property_count(const coil_config_value_t *value);

/**
 * @brief Get the key of an object property by index
 * @param value Object configuration value
 * @param index Property index
 * @return Property key or NULL if out of bounds
 */
const char* coil_config_get_object_property_key(const coil_config_value_t *value, 
                                               uint32_t index);

/**
 * @brief Merge two configurations
 * @param base Base configuration
 * @param overlay Overlay configuration (takes precedence)
 * @return Merged configuration or NULL on failure
 */
coil_config_t* coil_config_merge(const coil_config_t *base, const coil_config_t *overlay);

/**
 * @brief Clone a configuration
 * @param config Configuration to clone
 * @return Cloned configuration or NULL on failure
 */
coil_config_t* coil_config_clone(const coil_config_t *config);

#endif /* COIL_CONFIG_H */