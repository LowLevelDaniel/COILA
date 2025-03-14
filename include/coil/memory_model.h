/**
 * @file memory_model.h
 * @brief COIL memory model definitions
 * @details This file contains definitions for the COIL memory model,
 *          including memory spaces, access patterns, and ordering.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_MEMORY_MODEL_H
#define COIL_MEMORY_MODEL_H

#include <stdint.h>

/**
 * @brief Memory space identifiers
 */
typedef enum {
  COIL_MEM_SPACE_GENERIC = 0,  /**< Generic (unspecified) memory space */
  COIL_MEM_SPACE_GLOBAL = 1,   /**< Process-wide memory */
  COIL_MEM_SPACE_LOCAL = 2,    /**< Thread-local memory */
  COIL_MEM_SPACE_SHARED = 3,   /**< Memory shared between work items */
  COIL_MEM_SPACE_CONSTANT = 4, /**< Read-only memory */
  COIL_MEM_SPACE_PRIVATE = 5   /**< Function-local memory */
} coil_memory_space_t;

/**
 * @brief Memory ordering model
 */
typedef enum {
  COIL_MEM_ORDER_RELAXED = 0, /**< Relaxed ordering */
  COIL_MEM_ORDER_ACQUIRE = 1, /**< Acquire ordering */
  COIL_MEM_ORDER_RELEASE = 2, /**< Release ordering */
  COIL_MEM_ORDER_ACQ_REL = 3, /**< Acquire-release ordering */
  COIL_MEM_ORDER_SEQ_CST = 4  /**< Sequentially consistent ordering */
} coil_memory_order_t;

/**
 * @brief Memory access type
 */
typedef enum {
  COIL_MEM_ACCESS_NORMAL = 0,     /**< Normal memory access */
  COIL_MEM_ACCESS_VOLATILE = 1,   /**< Volatile memory access */
  COIL_MEM_ACCESS_ATOMIC = 2,     /**< Atomic memory access */
  COIL_MEM_ACCESS_NON_TEMPORAL = 3 /**< Non-temporal memory access */
} coil_memory_access_t;

/**
 * @brief Memory alignment requirements
 */
typedef enum {
  COIL_MEM_ALIGN_NONE = 0,    /**< No alignment requirement */
  COIL_MEM_ALIGN_NATURAL = 1, /**< Natural alignment for the type */
  COIL_MEM_ALIGN_PACKED = 2,  /**< Packed alignment (1 byte) */
  COIL_MEM_ALIGN_EXPLICIT = 3 /**< Explicit alignment value */
} coil_memory_alignment_t;

/**
 * @brief Memory access flags
 */
typedef enum {
  COIL_MEM_FLAG_NONE = 0x00,      /**< No flags */
  COIL_MEM_FLAG_VOLATILE = 0x01,  /**< Volatile access */
  COIL_MEM_FLAG_ATOMIC = 0x02,    /**< Atomic access */
  COIL_MEM_FLAG_NON_TEMPORAL = 0x04, /**< Non-temporal access */
  COIL_MEM_FLAG_READ_ONLY = 0x08, /**< Read-only access */
  COIL_MEM_FLAG_WRITE_ONLY = 0x10 /**< Write-only access */
} coil_memory_flags_t;

/**
 * @brief Memory address structure
 */
typedef struct {
  uint8_t space;     /**< Memory space (coil_memory_space_t) */
  uint8_t access;    /**< Access type (coil_memory_access_t) */
  uint8_t align_type; /**< Alignment type (coil_memory_alignment_t) */
  uint8_t order;     /**< Memory ordering (coil_memory_order_t) */
  uint32_t alignment; /**< Alignment value (when align_type is EXPLICIT) */
} coil_memory_address_t;

/**
 * @brief Global variable structure
 */
typedef struct {
  uint32_t id;          /**< Variable ID */
  char *name;           /**< Variable name */
  coil_memory_address_t address; /**< Memory address */
  uint32_t type;        /**< Variable type */
  uint32_t size;        /**< Variable size in bytes */
  uint32_t flags;       /**< Memory flags */
  uint8_t *initial_value; /**< Initial value (or NULL) */
  uint32_t initial_value_size; /**< Size of initial value */
} coil_global_variable_t;

/**
 * @brief Create a memory address structure
 * @param space Memory space
 * @param access Access type
 * @param align_type Alignment type
 * @param alignment Explicit alignment value (when align_type is EXPLICIT)
 * @param order Memory ordering
 * @return Memory address structure
 */
coil_memory_address_t coil_memory_address_create(coil_memory_space_t space,
                                               coil_memory_access_t access,
                                               coil_memory_alignment_t align_type,
                                               uint32_t alignment,
                                               coil_memory_order_t order);

/**
 * @brief Create a global variable
 * @param id Variable ID
 * @param name Variable name
 * @param address Memory address
 * @param type Variable type
 * @param size Variable size in bytes
 * @param flags Memory flags
 * @param initial_value Initial value (or NULL)
 * @param initial_value_size Size of initial value
 * @return New global variable or NULL on failure
 */
coil_global_variable_t* coil_global_variable_create(uint32_t id,
                                                 const char *name,
                                                 coil_memory_address_t address,
                                                 uint32_t type,
                                                 uint32_t size,
                                                 uint32_t flags,
                                                 const uint8_t *initial_value,
                                                 uint32_t initial_value_size);

/**
 * @brief Destroy a global variable
 * @param variable Global variable to destroy
 */
void coil_global_variable_destroy(coil_global_variable_t *variable);

/**
 * @brief Get the natural alignment for a type
 * @param type Type ID
 * @return Natural alignment in bytes
 */
uint32_t coil_memory_get_natural_alignment(uint32_t type);

/**
 * @brief Check if a memory access is atomic
 * @param address Memory address
 * @param size Access size in bytes
 * @return 1 if atomic, 0 if not
 */
int coil_memory_is_atomic_access(const coil_memory_address_t *address, uint32_t size);

/**
 * @brief Get the memory space name
 * @param space Memory space
 * @return Memory space name
 */
const char* coil_memory_space_get_name(coil_memory_space_t space);

/**
 * @brief Get the memory order name
 * @param order Memory order
 * @return Memory order name
 */
const char* coil_memory_order_get_name(coil_memory_order_t order);

#endif /* COIL_MEMORY_MODEL_H */