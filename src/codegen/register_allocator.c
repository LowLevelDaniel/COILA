/**
 * @file register_allocator.c
 * @brief Register allocation implementation
 * @details Implementation of the register allocator for the COIL assembler.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include "coil-assembler/target.h"
#include "coil-assembler/diagnostics.h"
#include "../utils/memory.c"
#include "../utils/logging.c"

/**
 * @brief Register allocation algorithm type
 */
typedef enum {
  REG_ALLOC_LINEAR_SCAN = 0,  /**< Linear scan allocation */
  REG_ALLOC_GRAPH_COLORING = 1, /**< Graph coloring allocation */
  REG_ALLOC_GREEDY = 2        /**< Greedy allocation */
} reg_alloc_algorithm_t;

/**
 * @brief Register class type
 */
typedef enum {
  REG_CLASS_GENERAL = 0,  /**< General purpose registers */
  REG_CLASS_FLOAT = 1,    /**< Floating point registers */
  REG_CLASS_VECTOR = 2,   /**< Vector registers */
  REG_CLASS_SPECIAL = 3   /**< Special purpose registers */
} reg_class_t;

/**
 * @brief Live interval structure
 */
typedef struct {
  uint32_t vreg;          /**< Virtual register */
  uint32_t preg;          /**< Physical register (0 if not allocated) */
  uint32_t start;         /**< Start position */
  uint32_t end;           /**< End position */
  reg_class_t reg_class;  /**< Register class */
  uint32_t spill_slot;    /**< Spill slot (if spilled) */
  int is_spilled;         /**< Whether the interval is spilled */
  coil_type_t data_type;  /**< Data type */
} live_interval_t;

/**
 * @brief Register allocator structure
 */
typedef struct {
  coil_target_context_t* target_context;    /**< Target context */
  coil_diagnostics_context_t* diag_context; /**< Diagnostics context */
  reg_alloc_algorithm_t algorithm;          /**< Allocation algorithm */
  
  uint32_t* pregs_general;   /**< Available general purpose registers */
  uint32_t pregs_general_count; /**< Number of general registers */
  uint32_t* pregs_float;     /**< Available floating point registers */
  uint32_t pregs_float_count; /**< Number of float registers */
  uint32_t* pregs_vector;    /**< Available vector registers */
  uint32_t pregs_vector_count; /**< Number of vector registers */
  
  uint32_t current_position; /**< Current position in the allocation */
  
  live_interval_t* intervals; /**< Live intervals */
  uint32_t interval_count;    /**< Number of live intervals */
  uint32_t interval_capacity; /**< Capacity of intervals array */
  
  uint32_t* active;          /**< Active intervals */
  uint32_t active_count;     /**< Number of active intervals */
  uint32_t active_capacity;  /**< Capacity of active array */
  
  uint32_t spill_count;      /**< Number of spilled registers */
  uint32_t* spill_slots;     /**< Spill slots (offset from frame pointer) */
  uint32_t spill_slot_count; /**< Number of spill slots */
  
  uint32_t next_vreg;        /**< Next virtual register to allocate */
} reg_allocator_t;

/**
 * @brief Create a register allocator
 * @param target_context Target context
 * @param diag_context Diagnostics context (can be NULL)
 * @return New register allocator or NULL on failure
 */
reg_allocator_t* reg_allocator_create(coil_target_context_t* target_context,
                                     coil_diagnostics_context_t* diag_context) {
  if (!target_context) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            1, "NULL target context");
    }
    return NULL;
  }
  
  /* Get target resources */
  const coil_target_resources_t* resources = coil_target_get_resources(target_context);
  if (!resources) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            2, "Failed to get target resources");
    }
    return NULL;
  }
  
  /* Allocate register allocator */
  reg_allocator_t* allocator = (reg_allocator_t*)coil_calloc(1, sizeof(reg_allocator_t));
  if (!allocator) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            3, "Failed to allocate register allocator");
    }
    return NULL;
  }
  
  /* Initialize register allocator */
  allocator->target_context = target_context;
  allocator->diag_context = diag_context;
  allocator->algorithm = REG_ALLOC_LINEAR_SCAN;
  
  /* Allocate register arrays */
  allocator->pregs_general = (uint32_t*)coil_calloc(resources->general_registers, 
                                                  sizeof(uint32_t));
  allocator->pregs_general_count = resources->general_registers;
  
  allocator->pregs_float = (uint32_t*)coil_calloc(resources->float_registers, 
                                                sizeof(uint32_t));
  allocator->pregs_float_count = resources->float_registers;
  
  allocator->pregs_vector = (uint32_t*)coil_calloc(resources->vector_registers, 
                                                 sizeof(uint32_t));
  allocator->pregs_vector_count = resources->vector_registers;
  
  /* Initialize register arrays */
  for (uint32_t i = 0; i < allocator->pregs_general_count; i++) {
    allocator->pregs_general[i] = i;
  }
  
  for (uint32_t i = 0; i < allocator->pregs_float_count; i++) {
    allocator->pregs_float[i] = i;
  }
  
  for (uint32_t i = 0; i < allocator->pregs_vector_count; i++) {
    allocator->pregs_vector[i] = i;
  }
  
  /* Allocate intervals array */
  allocator->interval_capacity = 256;
  allocator->intervals = (live_interval_t*)coil_calloc(allocator->interval_capacity, 
                                                     sizeof(live_interval_t));
  if (!allocator->intervals) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            4, "Failed to allocate live intervals");
    }
    reg_allocator_destroy(allocator);
    return NULL;
  }
  
  /* Allocate active array */
  allocator->active_capacity = 64;
  allocator->active = (uint32_t*)coil_calloc(allocator->active_capacity, 
                                           sizeof(uint32_t));
  if (!allocator->active) {
    if (diag_context) {
      coil_diagnostics_report(diag_context, COIL_DIAG_ERROR, 
                            COIL_DIAG_CATEGORY_GENERATOR,
                            5, "Failed to allocate active intervals");
    }
    reg_allocator_destroy(allocator);
    return NULL;
  }
  
  /* Initialize next virtual register */
  allocator->next_vreg = 1;
  
  return allocator;
}

/**
 * @brief Destroy a register allocator
 * @param allocator Register allocator to destroy
 */
void reg_allocator_destroy(reg_allocator_t* allocator) {
  if (!allocator) {
    return;
  }
  
  /* Free register arrays */
  if (allocator->pregs_general) {
    coil_free(allocator->pregs_general, allocator->pregs_general_count * sizeof(uint32_t));
  }
  
  if (allocator->pregs_float) {
    coil_free(allocator->pregs_float, allocator->pregs_float_count * sizeof(uint32_t));
  }
  
  if (allocator->pregs_vector) {
    coil_free(allocator->pregs_vector, allocator->pregs_vector_count * sizeof(uint32_t));
  }
  
  /* Free intervals array */
  if (allocator->intervals) {
    coil_free(allocator->intervals, allocator->interval_capacity * sizeof(live_interval_t));
  }
  
  /* Free active array */
  if (allocator->active) {
    coil_free(allocator->active, allocator->active_capacity * sizeof(uint32_t));
  }
  
  /* Free spill slots array */
  if (allocator->spill_slots) {
    coil_free(allocator->spill_slots, allocator->spill_slot_count * sizeof(uint32_t));
  }
  
  /* Free allocator */
  coil_free(allocator, sizeof(reg_allocator_t));
}

/**
 * @brief Set the allocation algorithm
 * @param allocator Register allocator
 * @param algorithm Allocation algorithm
 * @return 0 on success, non-zero on failure
 */
int reg_allocator_set_algorithm(reg_allocator_t* allocator, 
                               reg_alloc_algorithm_t algorithm) {
  if (!allocator) {
    return -1;
  }
  
  allocator->algorithm = algorithm;
  
  return 0;
}

/**
 * @brief Reset the register allocator for a new function
 * @param allocator Register allocator
 * @return 0 on success, non-zero on failure
 */
int reg_allocator_reset(reg_allocator_t* allocator) {
  if (!allocator) {
    return -1;
  }
  
  /* Reset intervals */
  allocator->interval_count = 0;
  
  /* Reset active intervals */
  allocator->active_count = 0;
  
  /* Reset current position */
  allocator->current_position = 0;
  
  /* Reset spill count */
  allocator->spill_count = 0;
  
  /* Reset spill slots */
  if (allocator->spill_slots) {
    coil_free(allocator->spill_slots, allocator->spill_slot_count * sizeof(uint32_t));
    allocator->spill_slots = NULL;
    allocator->spill_slot_count = 0;
  }
  
  /* Reset next virtual register */
  allocator->next_vreg = 1;
  
  return 0;
}

/**
 * @brief Add a live interval
 * @param allocator Register allocator
 * @param vreg Virtual register
 * @param start Start position
 * @param end End position
 * @param reg_class Register class
 * @param data_type Data type
 * @return Interval index or -1 on failure
 */
int reg_allocator_add_interval(reg_allocator_t* allocator,
                              uint32_t vreg,
                              uint32_t start,
                              uint32_t end,
                              reg_class_t reg_class,
                              coil_type_t data_type) {
  if (!allocator) {
    return -1;
  }
  
  /* Check if we need to expand the intervals array */
  if (allocator->interval_count >= allocator->interval_capacity) {
    uint32_t new_capacity = allocator->interval_capacity * 2;
    live_interval_t* new_intervals = (live_interval_t*)coil_realloc(
        allocator->intervals,
        allocator->interval_capacity * sizeof(live_interval_t),
        new_capacity * sizeof(live_interval_t));
    
    if (!new_intervals) {
      if (allocator->diag_context) {
        coil_diagnostics_report(allocator->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              6, "Failed to expand live intervals");
      }
      return -1;
    }
    
    allocator->intervals = new_intervals;
    allocator->interval_capacity = new_capacity;
  }
  
  /* Add the interval */
  allocator->intervals[allocator->interval_count].vreg = vreg;
  allocator->intervals[allocator->interval_count].preg = 0;
  allocator->intervals[allocator->interval_count].start = start;
  allocator->intervals[allocator->interval_count].end = end;
  allocator->intervals[allocator->interval_count].reg_class = reg_class;
  allocator->intervals[allocator->interval_count].spill_slot = 0;
  allocator->intervals[allocator->interval_count].is_spilled = 0;
  allocator->intervals[allocator->interval_count].data_type = data_type;
  
  return allocator->interval_count++;
}

/**
 * @brief Allocate a new virtual register
 * @param allocator Register allocator
 * @param reg_class Register class
 * @param data_type Data type
 * @return Virtual register ID or 0 on failure
 */
uint32_t reg_allocator_allocate_vreg(reg_allocator_t* allocator,
                                    reg_class_t reg_class,
                                    coil_type_t data_type) {
  if (!allocator) {
    return 0;
  }
  
  return allocator->next_vreg++;
}

/**
 * @brief Compare two intervals by start position (for sorting)
 * @param a First interval
 * @param b Second interval
 * @return Comparison result
 */
static int compare_intervals(const void* a, const void* b) {
  const live_interval_t* interval_a = (const live_interval_t*)a;
  const live_interval_t* interval_b = (const live_interval_t*)b;
  
  if (interval_a->start < interval_b->start) {
    return -1;
  } else if (interval_a->start > interval_b->start) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * @brief Expire old intervals
 * @param allocator Register allocator
 * @param current Current interval index
 */
static void expire_old_intervals(reg_allocator_t* allocator, uint32_t current) {
  /* Get current interval */
  live_interval_t* interval = &allocator->intervals[current];
  
  /* Iterate through active intervals */
  for (uint32_t i = 0; i < allocator->active_count; i++) {
    uint32_t active_idx = allocator->active[i];
    live_interval_t* active = &allocator->intervals[active_idx];
    
    /* If active interval ends before current interval starts, remove it */
    if (active->end <= interval->start) {
      /* Remove from active list by shifting remaining elements */
      for (uint32_t j = i; j < allocator->active_count - 1; j++) {
        allocator->active[j] = allocator->active[j + 1];
      }
      
      allocator->active_count--;
      i--; /* Adjust loop counter after removal */
    }
  }
}

/**
 * @brief Spill an interval
 * @param allocator Register allocator
 * @param interval_idx Interval index
 * @return 0 on success, non-zero on failure
 */
static int spill_interval(reg_allocator_t* allocator, uint32_t interval_idx) {
  if (!allocator || interval_idx >= allocator->interval_count) {
    return -1;
  }
  
  live_interval_t* interval = &allocator->intervals[interval_idx];
  
  /* Allocate spill slot */
  uint32_t slot_size;
  
  /* Calculate slot size based on data type */
  switch (coil_type_get_category(interval->data_type)) {
    case COIL_TYPE_CATEGORY_INTEGER:
    case COIL_TYPE_CATEGORY_FLOAT:
      slot_size = coil_type_get_width(interval->data_type) / 8;
      break;
      
    case COIL_TYPE_CATEGORY_POINTER:
      slot_size = 8; /* Assuming 64-bit pointers */
      break;
      
    case COIL_TYPE_CATEGORY_VECTOR:
      /* For vectors, size depends on element type and count */
      slot_size = coil_type_get_width(interval->data_type) / 8;
      break;
      
    default:
      /* Default to pointer size */
      slot_size = 8;
      break;
  }
  
  /* Ensure minimum alignment */
  if (slot_size < 4) {
    slot_size = 4;
  }
  
  /* Allocate spill slots */
  if (!allocator->spill_slots) {
    allocator->spill_slots = (uint32_t*)coil_calloc(32, sizeof(uint32_t));
    if (!allocator->spill_slots) {
      if (allocator->diag_context) {
        coil_diagnostics_report(allocator->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              7, "Failed to allocate spill slots");
      }
      return -1;
    }
    allocator->spill_slot_count = 32;
  }
  
  /* Assign spill slot */
  interval->spill_slot = allocator->spill_count++;
  interval->is_spilled = 1;
  
  /* Mark as spilled in the allocator */
  if (interval->spill_slot >= allocator->spill_slot_count) {
    uint32_t new_count = allocator->spill_slot_count * 2;
    uint32_t* new_slots = (uint32_t*)coil_realloc(
        allocator->spill_slots,
        allocator->spill_slot_count * sizeof(uint32_t),
        new_count * sizeof(uint32_t));
    
    if (!new_slots) {
      if (allocator->diag_context) {
        coil_diagnostics_report(allocator->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              8, "Failed to expand spill slots");
      }
      return -1;
    }
    
    allocator->spill_slots = new_slots;
    allocator->spill_slot_count = new_count;
  }
  
  /* Store slot size in spill slots array */
  allocator->spill_slots[interval->spill_slot] = slot_size;
  
  return 0;
}

/**
 * @brief Get the spill slot offset for a virtual register
 * @param allocator Register allocator
 * @param vreg Virtual register
 * @return Offset from frame pointer or -1 if not spilled
 */
int reg_allocator_get_spill_offset(reg_allocator_t* allocator, uint32_t vreg) {
  if (!allocator) {
    return -1;
  }
  
  /* Find the interval for the virtual register */
  for (uint32_t i = 0; i < allocator->interval_count; i++) {
    if (allocator->intervals[i].vreg == vreg && allocator->intervals[i].is_spilled) {
      /* Calculate offset based on spill slot */
      uint32_t offset = 0;
      for (uint32_t j = 0; j < allocator->intervals[i].spill_slot; j++) {
        offset += allocator->spill_slots[j];
      }
      
      return offset;
    }
  }
  
  return -1;
}

/**
 * @brief Check if a physical register is available
 * @param allocator Register allocator
 * @param reg_class Register class
 * @param preg Physical register
 * @return 1 if available, 0 if not
 */
static int is_register_available(reg_allocator_t* allocator, 
                                reg_class_t reg_class, 
                                uint32_t preg) {
  if (!allocator) {
    return 0;
  }
  
  /* Check if register is already allocated to an active interval */
  for (uint32_t i = 0; i < allocator->active_count; i++) {
    uint32_t active_idx = allocator->active[i];
    live_interval_t* active = &allocator->intervals[active_idx];
    
    if (active->reg_class == reg_class && active->preg == preg) {
      return 0;
    }
  }
  
  return 1;
}

/**
 * @brief Find a physical register for an interval
 * @param allocator Register allocator
 * @param interval_idx Interval index
 * @return Physical register or 0 if none available
 */
static uint32_t find_physical_register(reg_allocator_t* allocator, uint32_t interval_idx) {
  if (!allocator || interval_idx >= allocator->interval_count) {
    return 0;
  }
  
  live_interval_t* interval = &allocator->intervals[interval_idx];
  
  /* Get available registers based on register class */
  uint32_t* pregs;
  uint32_t pregs_count;
  
  switch (interval->reg_class) {
    case REG_CLASS_GENERAL:
      pregs = allocator->pregs_general;
      pregs_count = allocator->pregs_general_count;
      break;
      
    case REG_CLASS_FLOAT:
      pregs = allocator->pregs_float;
      pregs_count = allocator->pregs_float_count;
      break;
      
    case REG_CLASS_VECTOR:
      pregs = allocator->pregs_vector;
      pregs_count = allocator->pregs_vector_count;
      break;
      
    default:
      /* Unsupported register class */
      return 0;
  }
  
  /* Find first available register */
  for (uint32_t i = 0; i < pregs_count; i++) {
    if (is_register_available(allocator, interval->reg_class, pregs[i])) {
      return pregs[i];
    }
  }
  
  /* No register available */
  return 0;
}

/**
 * @brief Run linear scan register allocation
 * @param allocator Register allocator
 * @return 0 on success, non-zero on failure
 */
static int linear_scan_allocate(reg_allocator_t* allocator) {
  if (!allocator) {
    return -1;
  }
  
  /* Sort intervals by start position */
  qsort(allocator->intervals, allocator->interval_count, 
       sizeof(live_interval_t), compare_intervals);
  
  /* Process intervals in order */
  for (uint32_t i = 0; i < allocator->interval_count; i++) {
    live_interval_t* interval = &allocator->intervals[i];
    
    /* Expire old intervals */
    expire_old_intervals(allocator, i);
    
    /* Find a physical register */
    uint32_t preg = find_physical_register(allocator, i);
    if (preg != 0) {
      /* Assign physical register */
      interval->preg = preg;
      
      /* Check if we need to expand the active array */
      if (allocator->active_count >= allocator->active_capacity) {
        uint32_t new_capacity = allocator->active_capacity * 2;
        uint32_t* new_active = (uint32_t*)coil_realloc(
            allocator->active,
            allocator->active_capacity * sizeof(uint32_t),
            new_capacity * sizeof(uint32_t));
        
        if (!new_active) {
          if (allocator->diag_context) {
            coil_diagnostics_report(allocator->diag_context, COIL_DIAG_ERROR, 
                                  COIL_DIAG_CATEGORY_GENERATOR,
                                  9, "Failed to expand active intervals");
          }
          return -1;
        }
        
        allocator->active = new_active;
        allocator->active_capacity = new_capacity;
      }
      
      /* Add to active list */
      allocator->active[allocator->active_count++] = i;
    } else {
      /* No physical register available, need to spill */
      
      /* Find the interval with the furthest end point */
      uint32_t spill_idx = UINT32_MAX;
      uint32_t max_end = 0;
      
      for (uint32_t j = 0; j < allocator->active_count; j++) {
        uint32_t active_idx = allocator->active[j];
        live_interval_t* active = &allocator->intervals[active_idx];
        
        /* Only consider intervals of the same register class */
        if (active->reg_class != interval->reg_class) {
          continue;
        }
        
        if (active->end > max_end) {
          max_end = active->end;
          spill_idx = active_idx;
        }
      }
      
      /* If current interval ends before the furthest active interval,
       * spill the current interval */
      if (spill_idx != UINT32_MAX && interval->end > max_end) {
        spill_interval(allocator, i);
        
        /* Log spill */
        coil_log_debug("Spilled virtual register %u (interval %u) to stack slot %u",
                     interval->vreg, i, interval->spill_slot);
      } else if (spill_idx != UINT32_MAX) {
        /* Spill the active interval with the furthest end point */
        live_interval_t* spill_interval = &allocator->intervals[spill_idx];
        
        /* Steal its register */
        interval->preg = spill_interval->preg;
        
        /* Spill the interval */
        spill_interval->preg = 0;
        if (spill_interval(allocator, spill_idx) != 0) {
          return -1;
        }
        
        /* Log spill */
        coil_log_debug("Spilled virtual register %u (interval %u) to stack slot %u, "
                     "allocated register %u to virtual register %u (interval %u)",
                     spill_interval->vreg, spill_idx, spill_interval->spill_slot,
                     interval->preg, interval->vreg, i);
        
        /* Remove spilled interval from active list */
        for (uint32_t j = 0; j < allocator->active_count; j++) {
          if (allocator->active[j] == spill_idx) {
            /* Remove by shifting remaining elements */
            for (uint32_t k = j; k < allocator->active_count - 1; k++) {
              allocator->active[k] = allocator->active[k + 1];
            }
            
            allocator->active_count--;
            break;
          }
        }
        
        /* Add current interval to active list */
        allocator->active[allocator->active_count++] = i;
      } else {
        /* No active intervals to spill, must spill current interval */
        spill_interval(allocator, i);
        
        /* Log spill */
        coil_log_debug("Spilled virtual register %u (interval %u) to stack slot %u",
                     interval->vreg, i, interval->spill_slot);
      }
    }
  }
  
  return 0;
}

/**
 * @brief Run graph coloring register allocation
 * @param allocator Register allocator
 * @return 0 on success, non-zero on failure
 */
static int graph_coloring_allocate(reg_allocator_t* allocator) {
  /* Not implemented yet */
  if (allocator->diag_context) {
    coil_diagnostics_report(allocator->diag_context, COIL_DIAG_WARNING, 
                          COIL_DIAG_CATEGORY_GENERATOR,
                          10, "Graph coloring allocation not implemented, using linear scan");
  }
  
  /* Fall back to linear scan */
  return linear_scan_allocate(allocator);
}

/**
 * @brief Run greedy register allocation
 * @param allocator Register allocator
 * @return 0 on success, non-zero on failure
 */
static int greedy_allocate(reg_allocator_t* allocator) {
  /* Not implemented yet */
  if (allocator->diag_context) {
    coil_diagnostics_report(allocator->diag_context, COIL_DIAG_WARNING, 
                          COIL_DIAG_CATEGORY_GENERATOR,
                          11, "Greedy allocation not implemented, using linear scan");
  }
  
  /* Fall back to linear scan */
  return linear_scan_allocate(allocator);
}

/**
 * @brief Run register allocation
 * @param allocator Register allocator
 * @return 0 on success, non-zero on failure
 */
int reg_allocator_allocate(reg_allocator_t* allocator) {
  if (!allocator) {
    return -1;
  }
  
  /* Check if we have any intervals to allocate */
  if (allocator->interval_count == 0) {
    return 0;
  }
  
  /* Run allocation algorithm */
  switch (allocator->algorithm) {
    case REG_ALLOC_LINEAR_SCAN:
      return linear_scan_allocate(allocator);
      
    case REG_ALLOC_GRAPH_COLORING:
      return graph_coloring_allocate(allocator);
      
    case REG_ALLOC_GREEDY:
      return greedy_allocate(allocator);
      
    default:
      /* Unsupported algorithm */
      if (allocator->diag_context) {
        coil_diagnostics_report(allocator->diag_context, COIL_DIAG_ERROR, 
                              COIL_DIAG_CATEGORY_GENERATOR,
                              12, "Unsupported allocation algorithm");
      }
      return -1;
  }
}

/**
 * @brief Map a virtual register to a physical register
 * @param allocator Register allocator
 * @param vreg Virtual register
 * @return Physical register or 0 if spilled
 */
uint32_t reg_allocator_map_vreg(reg_allocator_t* allocator, uint32_t vreg) {
  if (!allocator) {
    return 0;
  }
  
  /* Find the interval for the virtual register */
  for (uint32_t i = 0; i < allocator->interval_count; i++) {
    if (allocator->intervals[i].vreg == vreg) {
      return allocator->intervals[i].preg;
    }
  }
  
  /* Virtual register not found */
  return 0;
}

/**
 * @brief Check if a virtual register is spilled
 * @param allocator Register allocator
 * @param vreg Virtual register
 * @return 1 if spilled, 0 if not
 */
int reg_allocator_is_spilled(reg_allocator_t* allocator, uint32_t vreg) {
  if (!allocator) {
    return 0;
  }
  
  /* Find the interval for the virtual register */
  for (uint32_t i = 0; i < allocator->interval_count; i++) {
    if (allocator->intervals[i].vreg == vreg) {
      return allocator->intervals[i].is_spilled;
    }
  }
  
  /* Virtual register not found */
  return 0;
}

/**
 * @brief Get the register class for a virtual register
 * @param allocator Register allocator
 * @param vreg Virtual register
 * @return Register class or -1 if not found
 */
int reg_allocator_get_reg_class(reg_allocator_t* allocator, uint32_t vreg) {
  if (!allocator) {
    return -1;
  }
  
  /* Find the interval for the virtual register */
  for (uint32_t i = 0; i < allocator->interval_count; i++) {
    if (allocator->intervals[i].vreg == vreg) {
      return allocator->intervals[i].reg_class;
    }
  }
  
  /* Virtual register not found */
  return -1;
}

/**
 * @brief Get the data type for a virtual register
 * @param allocator Register allocator
 * @param vreg Virtual register
 * @return Data type or 0 if not found
 */
coil_type_t reg_allocator_get_data_type(reg_allocator_t* allocator, uint32_t vreg) {
  if (!allocator) {
    return 0;
  }
  
  /* Find the interval for the virtual register */
  for (uint32_t i = 0; i < allocator->interval_count; i++) {
    if (allocator->intervals[i].vreg == vreg) {
      return allocator->intervals[i].data_type;
    }
  }
  
  /* Virtual register not found */
  return 0;
}

/**
 * @brief Get statistics about register allocation
 * @param allocator Register allocator
 * @param total_intervals Pointer to store total intervals (can be NULL)
 * @param spilled_intervals Pointer to store spilled intervals (can be NULL)
 * @param spill_slots Pointer to store number of spill slots (can be NULL)
 * @return 0 on success, non-zero on failure
 */
int reg_allocator_get_stats(reg_allocator_t* allocator,
                           uint32_t* total_intervals,
                           uint32_t* spilled_intervals,
                           uint32_t* spill_slots) {
  if (!allocator) {
    return -1;
  }
  
  if (total_intervals) {
    *total_intervals = allocator->interval_count;
  }
  
  if (spilled_intervals) {
    /* Count spilled intervals */
    uint32_t count = 0;
    for (uint32_t i = 0; i < allocator->interval_count; i++) {
      if (allocator->intervals[i].is_spilled) {
        count++;
      }
    }
    
    *spilled_intervals = count;
  }
  
  if (spill_slots) {
    *spill_slots = allocator->spill_count;
  }
  
  return 0;
}

/**
 * @brief Get the total frame size required for spill slots
 * @param allocator Register allocator
 * @return Frame size in bytes or 0 if no spill slots
 */
uint32_t reg_allocator_get_frame_size(reg_allocator_t* allocator) {
  if (!allocator) {
    return 0;
  }
  
  /* Sum up all spill slot sizes */
  uint32_t total_size = 0;
  for (uint32_t i = 0; i < allocator->spill_count; i++) {
    total_size += allocator->spill_slots[i];
  }
  
  /* Align to 16 bytes for standard calling conventions */
  if (total_size % 16 != 0) {
    total_size = (total_size + 15) & ~15;
  }
  
  return total_size;
}

/**
 * @brief Dump register allocation information for debugging
 * @param allocator Register allocator
 */
void reg_allocator_dump(reg_allocator_t* allocator) {
  if (!allocator) {
    return;
  }
  
  /* Log basic statistics */
  coil_log_info("Register allocation statistics:");
  coil_log_info("  Total intervals: %u", allocator->interval_count);
  
  /* Count spilled intervals */
  uint32_t spilled = 0;
  for (uint32_t i = 0; i < allocator->interval_count; i++) {
    if (allocator->intervals[i].is_spilled) {
      spilled++;
    }
  }
  
  coil_log_info("  Spilled intervals: %u", spilled);
  coil_log_info("  Spill slots: %u", allocator->spill_count);
  coil_log_info("  Frame size: %u bytes", reg_allocator_get_frame_size(allocator));
  
  /* Log detailed allocation information */
  coil_log_debug("Live intervals:");
  for (uint32_t i = 0; i < allocator->interval_count; i++) {
    live_interval_t* interval = &allocator->intervals[i];
    
    if (interval->is_spilled) {
      coil_log_debug("  vreg %u -> [spilled to slot %u], range [%u, %u], class %d",
                    interval->vreg, interval->spill_slot, 
                    interval->start, interval->end, interval->reg_class);
    } else {
      coil_log_debug("  vreg %u -> preg %u, range [%u, %u], class %d",
                    interval->vreg, interval->preg, 
                    interval->start, interval->end, interval->reg_class);
    }
  }
}