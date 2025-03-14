/**
 * @file src/core/config.h
 *
 * @brief Target configuration system
 *
 * Defines target architecture capabilities, specifies hardware features,
 * describes memory model, and configures optimization parameters.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Target architecture configuration
 */
typedef struct {
  /** Target name */
  char* name;
  
  /** Architecture name */
  char* architecture;
  
  /** Vendor name */
  char* vendor;
  
  /** Hardware features */
  char** features;
  int feature_count;
  
  /** Resource properties */
  struct {
    int general_registers;
    int float_registers;
    int vector_registers;
    int vector_width;
    int min_alignment;
  } resources;
  
  /** Memory properties */
  struct {
    int page_size;
    int cacheline_size;
    char** memory_models;
    int memory_model_count;
  } memory;
  
  /** Optimization properties */
  struct {
    int vector_threshold;
    int unroll_factor;
    bool use_fma;
  } optimization;
  
} target_config_t;

/**
 * @brief Load target configuration from file
 *
 * @param path Path to configuration file
 * @return target_config_t* Loaded configuration or NULL on error
 */
target_config_t* load_target_config(const char* path);

/* Additional functions... */

#endif /* CONFIG_H */