/**
 * @file pass_manager.c
 * @brief Optimization pass manager implementation
 * @details Implementation of the optimization pass manager component for the COIL assembler.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include "coil-assembler/assembler.h"
#include "coil-assembler/diagnostics.h"
#include "../utils/memory.c"
#include "../utils/logging.c"

/**
 * @brief Pass manager module
 */
typedef struct coil_pass_manager_module_s {
  const char* name;                         /**< Module name */
  struct coil_pass_manager_module_s* next;  /**< Next module in chain */
  int (*init)(void);                        /**< Module initialization function */
  int (*finalize)(void);                    /**< Module finalization function */
} coil_pass_manager_module_t;

/**
 * @brief Optimization pass descriptor
 */
typedef struct coil_opt_pass_s {
  const char* name;                        /**< Pass name */
  const char* description;                 /**< Pass description */
  int (*run)(coil_function_t*);            /**< Pass function */
  int enabled;                             /**< Whether the pass is enabled */
  coil_optimization_level_t min_level;     /**< Minimum optimization level */
  struct coil_opt_pass_s* next;            /**< Next pass in chain */
  coil_pass_manager_module_t* module;      /**< Module that registered this pass */
} coil_opt_pass_t;

/**
 * @brief Pass pipeline
 */
typedef struct coil_pass_pipeline_s {
  const char* name;                        /**< Pipeline name */
  coil_opt_pass_t* passes;                 /**< List of passes */
  struct coil_pass_pipeline_s* next;       /**< Next pipeline in chain */
} coil_pass_pipeline_t;

/**
 * @brief Pass manager context
 */
typedef struct {
  coil_diagnostics_context_t* diag_context;  /**< Diagnostics context */
  coil_optimization_level_t opt_level;       /**< Optimization level */
  coil_pass_manager_module_t* modules;       /**< Registered modules */
  coil_opt_pass_t* passes;                   /**< Registered passes */
  coil_pass_pipeline_t* pipelines;           /**< Registered pipelines */
  coil_pass_pipeline_t* current_pipeline;    /**< Current pipeline */
} coil_pass_manager_t;

/* Global pass manager instance */
static coil_pass_manager_t* g_pass_manager = NULL;

/**
 * @brief Initialize the pass manager
 * @param diag_context Diagnostics context (can be NULL)
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_init(coil_diagnostics_context_t* diag_context) {
  /* Check if already initialized */
  if (g_pass_manager) {
    return 0;
  }
  
  /* Allocate pass manager */
  g_pass_manager = (coil_pass_manager_t*)coil_calloc(1, sizeof(coil_pass_manager_t));
  if (!g_pass_manager) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_OPTIMIZER,
                            1, "Failed to allocate pass manager");
    }
    return -1;
  }
  
  /* Initialize pass manager */
  g_pass_manager->diag_context = diag_context;
  g_pass_manager->opt_level = COIL_OPT_LEVEL_1;
  g_pass_manager->modules = NULL;
  g_pass_manager->passes = NULL;
  g_pass_manager->pipelines = NULL;
  g_pass_manager->current_pipeline = NULL;
  
  coil_log_debug("Pass manager initialized");
  
  return 0;
}

/**
 * @brief Finalize the pass manager
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_finalize(void) {
  if (!g_pass_manager) {
    return 0;
  }
  
  /* Finalize all modules */
  coil_pass_manager_module_t* module = g_pass_manager->modules;
  while (module) {
    if (module->finalize) {
      module->finalize();
    }
    module = module->next;
  }
  
  /* Free all passes */
  coil_opt_pass_t* pass = g_pass_manager->passes;
  while (pass) {
    coil_opt_pass_t* next = pass->next;
    coil_free(pass, sizeof(coil_opt_pass_t));
    pass = next;
  }
  
  /* Free all pipelines */
  coil_pass_pipeline_t* pipeline = g_pass_manager->pipelines;
  while (pipeline) {
    coil_pass_pipeline_t* next = pipeline->next;
    coil_free(pipeline, sizeof(coil_pass_pipeline_t));
    pipeline = next;
  }
  
  /* Free all modules */
  module = g_pass_manager->modules;
  while (module) {
    coil_pass_manager_module_t* next = module->next;
    coil_free(module, sizeof(coil_pass_manager_module_t));
    module = next;
  }
  
  /* Free pass manager */
  coil_free(g_pass_manager, sizeof(coil_pass_manager_t));
  g_pass_manager = NULL;
  
  coil_log_debug("Pass manager finalized");
  
  return 0;
}

/**
 * @brief Register a new module with the pass manager
 * @param name Module name
 * @param init Module initialization function
 * @param finalize Module finalization function
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_register_module(const char* name,
                                     int (*init)(void),
                                     int (*finalize)(void)) {
  if (!g_pass_manager || !name) {
    return -1;
  }
  
  /* Check if module already exists */
  coil_pass_manager_module_t* module = g_pass_manager->modules;
  while (module) {
    if (strcmp(module->name, name) == 0) {
      /* Module already exists */
      if (g_pass_manager->diag_context) {
        coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_WARNING, 
                               COIL_DIAG_CATEGORY_OPTIMIZER,
                               2, "Module '%s' already registered", name);
      }
      return -1;
    }
    module = module->next;
  }
  
  /* Allocate new module */
  module = (coil_pass_manager_module_t*)coil_calloc(1, sizeof(coil_pass_manager_module_t));
  if (!module) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_OPTIMIZER,
                             3, "Failed to allocate module '%s'", name);
    }
    return -1;
  }
  
  /* Initialize module */
  module->name = name;
  module->init = init;
  module->finalize = finalize;
  module->next = NULL;
  
  /* Add to modules list */
  if (!g_pass_manager->modules) {
    g_pass_manager->modules = module;
  } else {
    coil_pass_manager_module_t* last = g_pass_manager->modules;
    while (last->next) {
      last = last->next;
    }
    last->next = module;
  }
  
  /* Initialize module if provided */
  if (init) {
    if (init() != 0) {
      if (g_pass_manager->diag_context) {
        coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                               COIL_DIAG_CATEGORY_OPTIMIZER,
                               4, "Failed to initialize module '%s'", name);
      }
      return -1;
    }
  }
  
  coil_log_debug("Registered module '%s'", name);
  
  return 0;
}

/**
 * @brief Find a module by name
 * @param name Module name
 * @return Module or NULL if not found
 */
coil_pass_manager_module_t* coil_pass_manager_find_module(const char* name) {
  if (!g_pass_manager || !name) {
    return NULL;
  }
  
  coil_pass_manager_module_t* module = g_pass_manager->modules;
  while (module) {
    if (strcmp(module->name, name) == 0) {
      return module;
    }
    module = module->next;
  }
  
  return NULL;
}

/**
 * @brief Register a new optimization pass
 * @param module_name Module name (or NULL for core)
 * @param name Pass name
 * @param description Pass description
 * @param run Pass function
 * @param min_level Minimum optimization level
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_register_pass(const char* module_name,
                                   const char* name,
                                   const char* description,
                                   int (*run)(coil_function_t*),
                                   coil_optimization_level_t min_level) {
  if (!g_pass_manager || !name || !run) {
    return -1;
  }
  
  /* Find module if specified */
  coil_pass_manager_module_t* module = NULL;
  if (module_name) {
    module = coil_pass_manager_find_module(module_name);
    if (!module) {
      if (g_pass_manager->diag_context) {
        coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                               COIL_DIAG_CATEGORY_OPTIMIZER,
                               5, "Module '%s' not found", module_name);
      }
      return -1;
    }
  }
  
  /* Check if pass already exists */
  coil_opt_pass_t* pass = g_pass_manager->passes;
  while (pass) {
    if (strcmp(pass->name, name) == 0) {
      /* Pass already exists */
      if (g_pass_manager->diag_context) {
        coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_WARNING, 
                               COIL_DIAG_CATEGORY_OPTIMIZER,
                               6, "Pass '%s' already registered", name);
      }
      return -1;
    }
    pass = pass->next;
  }
  
  /* Allocate new pass */
  pass = (coil_opt_pass_t*)coil_calloc(1, sizeof(coil_opt_pass_t));
  if (!pass) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_OPTIMIZER,
                             7, "Failed to allocate pass '%s'", name);
    }
    return -1;
  }
  
  /* Initialize pass */
  pass->name = name;
  pass->description = description;
  pass->run = run;
  pass->enabled = 1;
  pass->min_level = min_level;
  pass->module = module;
  pass->next = NULL;
  
  /* Add to passes list */
  if (!g_pass_manager->passes) {
    g_pass_manager->passes = pass;
  } else {
    coil_opt_pass_t* last = g_pass_manager->passes;
    while (last->next) {
      last = last->next;
    }
    last->next = pass;
  }
  
  coil_log_debug("Registered pass '%s'", name);
  
  return 0;
}

/**
 * @brief Find a pass by name
 * @param name Pass name
 * @return Pass or NULL if not found
 */
coil_opt_pass_t* coil_pass_manager_find_pass(const char* name) {
  if (!g_pass_manager || !name) {
    return NULL;
  }
  
  coil_opt_pass_t* pass = g_pass_manager->passes;
  while (pass) {
    if (strcmp(pass->name, name) == 0) {
      return pass;
    }
    pass = pass->next;
  }
  
  return NULL;
}

/**
 * @brief Enable or disable a pass
 * @param name Pass name
 * @param enabled Whether to enable the pass
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_enable_pass(const char* name, int enabled) {
  if (!g_pass_manager || !name) {
    return -1;
  }
  
  coil_opt_pass_t* pass = coil_pass_manager_find_pass(name);
  if (!pass) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_OPTIMIZER,
                             8, "Pass '%s' not found", name);
    }
    return -1;
  }
  
  pass->enabled = enabled;
  
  coil_log_debug("%s pass '%s'", enabled ? "Enabled" : "Disabled", name);
  
  return 0;
}

/**
 * @brief Create a new pipeline
 * @param name Pipeline name
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_create_pipeline(const char* name) {
  if (!g_pass_manager || !name) {
    return -1;
  }
  
  /* Check if pipeline already exists */
  coil_pass_pipeline_t* pipeline = g_pass_manager->pipelines;
  while (pipeline) {
    if (strcmp(pipeline->name, name) == 0) {
      /* Pipeline already exists */
      if (g_pass_manager->diag_context) {
        coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_WARNING, 
                               COIL_DIAG_CATEGORY_OPTIMIZER,
                               9, "Pipeline '%s' already exists", name);
      }
      return -1;
    }
    pipeline = pipeline->next;
  }
  
  /* Allocate new pipeline */
  pipeline = (coil_pass_pipeline_t*)coil_calloc(1, sizeof(coil_pass_pipeline_t));
  if (!pipeline) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_OPTIMIZER,
                             10, "Failed to allocate pipeline '%s'", name);
    }
    return -1;
  }
  
  /* Initialize pipeline */
  pipeline->name = name;
  pipeline->passes = NULL;
  pipeline->next = NULL;
  
  /* Add to pipelines list */
  if (!g_pass_manager->pipelines) {
    g_pass_manager->pipelines = pipeline;
  } else {
    coil_pass_pipeline_t* last = g_pass_manager->pipelines;
    while (last->next) {
      last = last->next;
    }
    last->next = pipeline;
  }
  
  coil_log_debug("Created pipeline '%s'", name);
  
  return 0;
}

/**
 * @brief Find a pipeline by name
 * @param name Pipeline name
 * @return Pipeline or NULL if not found
 */
coil_pass_pipeline_t* coil_pass_manager_find_pipeline(const char* name) {
  if (!g_pass_manager || !name) {
    return NULL;
  }
  
  coil_pass_pipeline_t* pipeline = g_pass_manager->pipelines;
  while (pipeline) {
    if (strcmp(pipeline->name, name) == 0) {
      return pipeline;
    }
    pipeline = pipeline->next;
  }
  
  return NULL;
}

/**
 * @brief Add a pass to a pipeline
 * @param pipeline_name Pipeline name
 * @param pass_name Pass name
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_add_pass_to_pipeline(const char* pipeline_name,
                                          const char* pass_name) {
  if (!g_pass_manager || !pipeline_name || !pass_name) {
    return -1;
  }
  
  /* Find pipeline */
  coil_pass_pipeline_t* pipeline = coil_pass_manager_find_pipeline(pipeline_name);
  if (!pipeline) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_OPTIMIZER,
                             11, "Pipeline '%s' not found", pipeline_name);
    }
    return -1;
  }
  
  /* Find pass */
  coil_opt_pass_t* pass = coil_pass_manager_find_pass(pass_name);
  if (!pass) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_OPTIMIZER,
                             12, "Pass '%s' not found", pass_name);
    }
    return -1;
  }
  
  /* Allocate new pass entry for the pipeline */
  coil_opt_pass_t* pipeline_pass = (coil_opt_pass_t*)coil_calloc(1, sizeof(coil_opt_pass_t));
  if (!pipeline_pass) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_OPTIMIZER,
                             13, "Failed to allocate pipeline pass entry");
    }
    return -1;
  }
  
  /* Copy pass properties */
  pipeline_pass->name = pass->name;
  pipeline_pass->description = pass->description;
  pipeline_pass->run = pass->run;
  pipeline_pass->enabled = pass->enabled;
  pipeline_pass->min_level = pass->min_level;
  pipeline_pass->module = pass->module;
  pipeline_pass->next = NULL;
  
  /* Add to pipeline's passes list */
  if (!pipeline->passes) {
    pipeline->passes = pipeline_pass;
  } else {
    coil_opt_pass_t* last = pipeline->passes;
    while (last->next) {
      last = last->next;
    }
    last->next = pipeline_pass;
  }
  
  coil_log_debug("Added pass '%s' to pipeline '%s'", pass_name, pipeline_name);
  
  return 0;
}

/**
 * @brief Set the current pipeline
 * @param name Pipeline name
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_set_pipeline(const char* name) {
  if (!g_pass_manager || !name) {
    return -1;
  }
  
  /* Find pipeline */
  coil_pass_pipeline_t* pipeline = coil_pass_manager_find_pipeline(name);
  if (!pipeline) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                             COIL_DIAG_CATEGORY_OPTIMIZER,
                             14, "Pipeline '%s' not found", name);
    }
    return -1;
  }
  
  g_pass_manager->current_pipeline = pipeline;
  
  coil_log_debug("Set current pipeline to '%s'", name);
  
  return 0;
}

/**
 * @brief Set the optimization level
 * @param level Optimization level
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_set_opt_level(coil_optimization_level_t level) {
  if (!g_pass_manager) {
    return -1;
  }
  
  g_pass_manager->opt_level = level;
  
  coil_log_debug("Set optimization level to %d", level);
  
  return 0;
}

/**
 * @brief Run the current pipeline on a function
 * @param function Function to optimize
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_run(coil_function_t* function) {
  if (!g_pass_manager || !function) {
    return -1;
  }
  
  /* Check if a current pipeline is set */
  if (!g_pass_manager->current_pipeline) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_report(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_OPTIMIZER,
                            15, "No current pipeline set");
    }
    return -1;
  }
  
  /* Log optimization start */
  coil_log_info("Running pipeline '%s' on function '%s'",
               g_pass_manager->current_pipeline->name,
               function->name);
  
  /* Run all passes in the pipeline */
  coil_opt_pass_t* pass = g_pass_manager->current_pipeline->passes;
  while (pass) {
    /* Skip disabled passes */
    if (!pass->enabled) {
      pass = pass->next;
      continue;
    }
    
    /* Skip passes below the current optimization level */
    if (pass->min_level > g_pass_manager->opt_level && 
        g_pass_manager->opt_level != COIL_OPT_LEVEL_S) {
      pass = pass->next;
      continue;
    }
    
    /* Run the pass */
    coil_log_debug("Running pass '%s'", pass->name);
    if (pass->run(function) != 0) {
      if (g_pass_manager->diag_context) {
        coil_diagnostics_reportf(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                               COIL_DIAG_CATEGORY_OPTIMIZER,
                               16, "Failed to run pass '%s'", pass->name);
      }
      return -1;
    }
    
    pass = pass->next;
  }
  
  coil_log_info("Finished pipeline '%s' on function '%s'",
               g_pass_manager->current_pipeline->name,
               function->name);
  
  return 0;
}

/**
 * @brief Create a default pipeline with standard passes
 * @return 0 on success, non-zero on failure
 */
int coil_pass_manager_create_default_pipeline(void) {
  if (!g_pass_manager) {
    return -1;
  }
  
  /* Create default pipeline */
  if (coil_pass_manager_create_pipeline("default") != 0) {
    return -1;
  }
  
  /* Add standard passes */
  const char* standard_passes[] = {
    "ConstantFolding",
    "DeadCodeElimination",
    "ConstantPropagation",
    "CommonSubexpressionElimination",
    "InstructionCombining",
    "LoopInvariantCodeMotion",
    "LoopUnrolling",
    "Vectorization",
    "PeepholeOptimizations",
    "TargetSpecific"
  };
  
  /* Add passes that exist */
  for (size_t i = 0; i < sizeof(standard_passes) / sizeof(standard_passes[0]); i++) {
    coil_opt_pass_t* pass = coil_pass_manager_find_pass(standard_passes[i]);
    if (pass) {
      coil_pass_manager_add_pass_to_pipeline("default", standard_passes[i]);
    }
  }
  
  /* Set as current pipeline */
  coil_pass_manager_set_pipeline("default");
  
  return 0;
}

/**
 * @brief Get a list of all registered passes
 * @param count Pointer to store the number of passes
 * @return Array of pass names or NULL on failure
 */
const char** coil_pass_manager_get_passes(uint32_t* count) {
  if (!g_pass_manager || !count) {
    return NULL;
  }
  
  /* Count passes */
  uint32_t pass_count = 0;
  coil_opt_pass_t* pass = g_pass_manager->passes;
  while (pass) {
    pass_count++;
    pass = pass->next;
  }
  
  *count = pass_count;
  
  if (pass_count == 0) {
    return NULL;
  }
  
  /* Allocate array of names */
  const char** names = (const char**)coil_calloc(pass_count, sizeof(const char*));
  if (!names) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_report(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_OPTIMIZER,
                            17, "Failed to allocate pass names array");
    }
    return NULL;
  }
  
  /* Fill array */
  pass = g_pass_manager->passes;
  for (uint32_t i = 0; i < pass_count; i++) {
    names[i] = pass->name;
    pass = pass->next;
  }
  
  return names;
}

/**
 * @brief Get a list of all registered pipelines
 * @param count Pointer to store the number of pipelines
 * @return Array of pipeline names or NULL on failure
 */
const char** coil_pass_manager_get_pipelines(uint32_t* count) {
  if (!g_pass_manager || !count) {
    return NULL;
  }
  
  /* Count pipelines */
  uint32_t pipeline_count = 0;
  coil_pass_pipeline_t* pipeline = g_pass_manager->pipelines;
  while (pipeline) {
    pipeline_count++;
    pipeline = pipeline->next;
  }
  
  *count = pipeline_count;
  
  if (pipeline_count == 0) {
    return NULL;
  }
  
  /* Allocate array of names */
  const char** names = (const char**)coil_calloc(pipeline_count, sizeof(const char*));
  if (!names) {
    if (g_pass_manager->diag_context) {
      coil_diagnostics_report(g_pass_manager->diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_OPTIMIZER,
                            18, "Failed to allocate pipeline names array");
    }
    return NULL;
  }
  
  /* Fill array */
  pipeline = g_pass_manager->pipelines;
  for (uint32_t i = 0; i < pipeline_count; i++) {
    names[i] = pipeline->name;
    pipeline = pipeline->next;
  }
  
  return names;
}

/**
 * @brief Get the current pipeline name
 * @return Current pipeline name or NULL if none
 */
const char* coil_pass_manager_get_current_pipeline(void) {
  if (!g_pass_manager || !g_pass_manager->current_pipeline) {
    return NULL;
  }
  
  return g_pass_manager->current_pipeline->name;
}

/**
 * @brief Get the current optimization level
 * @return Current optimization level
 */
coil_optimization_level_t coil_pass_manager_get_opt_level(void) {
  if (!g_pass_manager) {
    return COIL_OPT_LEVEL_0;
  }
  
  return g_pass_manager->opt_level;
}