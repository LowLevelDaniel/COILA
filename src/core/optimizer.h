/**
 * @file src/core/optimizer.h
 *
 * @brief General optimization framework
 *
 * Applies target-independent optimizations, performs register
 * allocation, schedules instructions for best performance,
 * and applies peephole optimizations.
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "core/ir/module.h"
#include "core/config.h"

/**
 * @brief Optimization context
 */
typedef struct {
  /** Target configuration */
  target_config_t* target;
  
  /** Optimization level */
  int level;
  
  /** Enable experimental optimizations */
  bool enable_experimental;
  
  /* Additional fields... */
} optimization_context_t;

/**
 * @brief Apply optimizations to a module
 *
 * @param module The module to optimize
 * @param context Optimization context
 * @return int 0 on success, error code otherwise
 */
int optimize_module(module_t* module, optimization_context_t* context);

/* Additional functions... */

#endif /* OPTIMIZER_H */