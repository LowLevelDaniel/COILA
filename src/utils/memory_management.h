/**
 * @file memory_management.h
 * @brief Memory management utilities for the COIL assembler
 * 
 * This module provides memory management utilities, including allocators,
 * memory tracking, and memory leak detection.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#include <stddef.h>
#include <stdbool.h>
#include "error_handling.h"

/**
 * @brief Memory allocation statistics
 */
typedef struct {
  size_t current_usage;         /**< Current memory usage in bytes */
  size_t peak_usage;            /**< Peak memory usage in bytes */
  size_t total_allocations;     /**< Total number of allocations */
  size_t total_frees;           /**< Total number of frees */
  size_t active_allocations;    /**< Current number of active allocations */
} memory_stats_t;

/**
 * @brief Memory pool for efficient allocation of same-sized objects
 */
typedef struct memory_pool_t memory_pool_t;

/**
 * @brief Initializes the memory management system
 *
 * @param[in] enable_tracking Whether to enable memory tracking
 * @return Error code indicating success or failure
 */
error_t memory_init(bool enable_tracking);

/**
 * @brief Shuts down the memory management system
 *
 * @return Error code indicating success or failure
 */
error_t memory_shutdown(void);

/**
 * @brief Allocates memory with tracking
 *
 * @param[in] size Size of memory to allocate in bytes
 * @param[in] file Source file where allocation occurred
 * @param[in] line Line number where allocation occurred
 * @param[in] function Function where allocation occurred
 * @return Pointer to allocated memory, or NULL on failure
 */
void* memory_alloc_ex(
  size_t size,
  const char* file,
  int line,
  const char* function
);

/**
 * @brief Convenience macro for memory allocation
 */
#define memory_alloc(size) \
  memory_alloc_ex(size, __FILE__, __LINE__, __func__)

/**
 * @brief Allocates zeroed memory with tracking
 *
 * @param[in] count Number of elements
 * @param[in] size Size of each element in bytes
 * @param[in] file Source file where allocation occurred
 * @param[in] line Line number where allocation occurred
 * @param[in] function Function where allocation occurred
 * @return Pointer to allocated memory, or NULL on failure
 */
void* memory_calloc_ex(
  size_t count,
  size_t size,
  const char* file,
  int line,
  const char* function
);

/**
 * @brief Convenience macro for zeroed memory allocation
 */
#define memory_calloc(count, size) \
  memory_calloc_ex(count, size, __FILE__, __LINE__, __func__)

/**
 * @brief Reallocates memory with tracking
 *
 * @param[in] ptr Pointer to memory to reallocate, or NULL
 * @param[in] size New size in bytes
 * @param[in] file Source file where reallocation occurred
 * @param[in] line Line number where reallocation occurred
 * @param[in] function Function where reallocation occurred
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* memory_realloc_ex(
  void* ptr,
  size_t size,
  const char* file,
  int line,
  const char* function
);

/**
 * @brief Convenience macro for memory reallocation
 */
#define memory_realloc(ptr, size) \
  memory_realloc_ex(ptr, size, __FILE__, __LINE__, __func__)

/**
 * @brief Frees memory with tracking
 *
 * @param[in] ptr Pointer to memory to free
 * @return Error code indicating success or failure
 */
error_t memory_free(void* ptr);

/**
 * @brief Creates a memory pool for efficient allocation
 *
 * @param[in] obj_size Size of each object in the pool
 * @param[in] initial_capacity Initial capacity of the pool
 * @return Pointer to created pool, or NULL on failure
 */
memory_pool_t* memory_pool_create(size_t obj_size, size_t initial_capacity);

/**
 * @brief Allocates an object from a memory pool
 *
 * @param[in] pool Memory pool to allocate from
 * @return Pointer to allocated object, or NULL on failure
 */
void* memory_pool_alloc(memory_pool_t* pool);

/**
 * @brief Returns an object to a memory pool
 *
 * @param[in] pool Memory pool to return to
 * @param[in] ptr Pointer to object to return
 * @return Error code indicating success or failure
 */
error_t memory_pool_free(memory_pool_t* pool, void* ptr);

/**
 * @brief Destroys a memory pool
 *
 * @param[in] pool Memory pool to destroy
 * @return Error code indicating success or failure
 */
error_t memory_pool_destroy(memory_pool_t* pool);

/**
 * @brief Gets memory allocation statistics
 *
 * @param[out] stats Pointer to receive statistics
 * @return Error code indicating success or failure
 */
error_t memory_get_stats(memory_stats_t* stats);

/**
 * @brief Checks for memory leaks
 *
 * @param[out] leak_count Pointer to receive the number of leaks, or NULL
 * @return true if leaks were detected, false otherwise
 */
bool memory_check_leaks(size_t* leak_count);

/**
 * @brief Dumps memory leak information to a file
 *
 * @param[in] filename Path to output file
 * @return Error code indicating success or failure
 */
error_t memory_dump_leaks(const char* filename);

#endif /* MEMORY_MANAGEMENT_H */