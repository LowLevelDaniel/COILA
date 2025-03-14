/**
 * @file memory.c
 * @brief Memory management utilities
 * @details Implementation of memory management utilities for the COIL assembler.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "coil-assembler/diagnostics.h"

/**
 * @brief Memory allocation stats
 */
typedef struct {
  size_t total_allocated;     /**< Total bytes allocated */
  size_t peak_allocated;      /**< Peak bytes allocated */
  size_t allocation_count;    /**< Number of allocations */
  size_t deallocation_count;  /**< Number of deallocations */
  size_t reallocation_count;  /**< Number of reallocations */
} coil_memory_stats_t;

/**
 * @brief Memory allocation tracking (only in debug builds)
 */
#ifdef COIL_DEBUG
static coil_memory_stats_t memory_stats = {0};
#endif

/**
 * @brief Allocate memory with tracking
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void* coil_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  
  void* ptr = malloc(size);
  
#ifdef COIL_DEBUG
  if (ptr) {
    memory_stats.total_allocated += size;
    memory_stats.allocation_count++;
    
    if (memory_stats.total_allocated > memory_stats.peak_allocated) {
      memory_stats.peak_allocated = memory_stats.total_allocated;
    }
  }
#endif
  
  return ptr;
}

/**
 * @brief Allocate zero-initialized memory with tracking
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void* coil_calloc(size_t count, size_t size) {
  if (count == 0 || size == 0) {
    return NULL;
  }
  
  void* ptr = calloc(count, size);
  
#ifdef COIL_DEBUG
  if (ptr) {
    memory_stats.total_allocated += (count * size);
    memory_stats.allocation_count++;
    
    if (memory_stats.total_allocated > memory_stats.peak_allocated) {
      memory_stats.peak_allocated = memory_stats.total_allocated;
    }
  }
#endif
  
  return ptr;
}

/**
 * @brief Reallocate memory with tracking
 * @param ptr Pointer to existing memory or NULL
 * @param old_size Current size of the memory block (for tracking)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory or NULL on failure
 */
void* coil_realloc(void* ptr, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    coil_free(ptr, old_size);
    return NULL;
  }
  
  if (ptr == NULL) {
    return coil_malloc(new_size);
  }
  
  void* new_ptr = realloc(ptr, new_size);
  
#ifdef COIL_DEBUG
  if (new_ptr) {
    memory_stats.total_allocated -= old_size;
    memory_stats.total_allocated += new_size;
    memory_stats.reallocation_count++;
    
    if (memory_stats.total_allocated > memory_stats.peak_allocated) {
      memory_stats.peak_allocated = memory_stats.total_allocated;
    }
  }
#endif
  
  return new_ptr;
}

/**
 * @brief Free memory with tracking
 * @param ptr Pointer to memory to free
 * @param size Size of the memory block (for tracking)
 */
void coil_free(void* ptr, size_t size) {
  if (ptr == NULL) {
    return;
  }
  
  free(ptr);
  
#ifdef COIL_DEBUG
  memory_stats.total_allocated -= size;
  memory_stats.deallocation_count++;
#endif
}

/**
 * @brief Duplicate a string with tracking
 * @param str String to duplicate
 * @return Pointer to new string or NULL on failure
 */
char* coil_strdup(const char* str) {
  if (str == NULL) {
    return NULL;
  }
  
  size_t len = strlen(str) + 1;
  char* new_str = (char*)coil_malloc(len);
  if (new_str == NULL) {
    return NULL;
  }
  
  memcpy(new_str, str, len);
  return new_str;
}

/**
 * @brief Duplicate a memory block with tracking
 * @param ptr Pointer to memory to duplicate
 * @param size Size of the memory block
 * @return Pointer to new memory or NULL on failure
 */
void* coil_memdup(const void* ptr, size_t size) {
  if (ptr == NULL || size == 0) {
    return NULL;
  }
  
  void* new_ptr = coil_malloc(size);
  if (new_ptr == NULL) {
    return NULL;
  }
  
  memcpy(new_ptr, ptr, size);
  return new_ptr;
}

/**
 * @brief Allocate aligned memory
 * @param size Size in bytes to allocate
 * @param alignment Alignment boundary (must be a power of 2)
 * @return Pointer to aligned memory or NULL on failure
 */
void* coil_aligned_malloc(size_t size, size_t alignment) {
  void* ptr = NULL;
  
  /* Ensure alignment is a power of 2 */
  if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
    return NULL;
  }
  
#ifdef _MSC_VER
  ptr = _aligned_malloc(size, alignment);
#else
  if (posix_memalign(&ptr, alignment, size) != 0) {
    ptr = NULL;
  }
#endif
  
#ifdef COIL_DEBUG
  if (ptr) {
    memory_stats.total_allocated += size;
    memory_stats.allocation_count++;
    
    if (memory_stats.total_allocated > memory_stats.peak_allocated) {
      memory_stats.peak_allocated = memory_stats.total_allocated;
    }
  }
#endif
  
  return ptr;
}

/**
 * @brief Free aligned memory
 * @param ptr Pointer to aligned memory
 * @param size Size of the memory block (for tracking)
 */
void coil_aligned_free(void* ptr, size_t size) {
  if (ptr == NULL) {
    return;
  }
  
#ifdef _MSC_VER
  _aligned_free(ptr);
#else
  free(ptr);
#endif
  
#ifdef COIL_DEBUG
  memory_stats.total_allocated -= size;
  memory_stats.deallocation_count++;
#endif
}

/**
 * @brief Get memory allocation statistics
 * @return Memory statistics structure
 */
coil_memory_stats_t coil_get_memory_stats(void) {
#ifdef COIL_DEBUG
  return memory_stats;
#else
  coil_memory_stats_t empty_stats = {0};
  return empty_stats;
#endif
}

/**
 * @brief Reset memory allocation statistics
 */
void coil_reset_memory_stats(void) {
#ifdef COIL_DEBUG
  memset(&memory_stats, 0, sizeof(memory_stats));
#endif
}

/**
 * @brief Print memory allocation statistics
 */
void coil_print_memory_stats(void) {
#ifdef COIL_DEBUG
  coil_diagnostics_report(NULL, COIL_DIAG_INFO, COIL_DIAG_CATEGORY_GENERAL, 0,
                         "Memory statistics:");
  coil_diagnostics_reportf(NULL, COIL_DIAG_INFO, COIL_DIAG_CATEGORY_GENERAL, 0,
                          "  Total allocated: %zu bytes", memory_stats.total_allocated);
  coil_diagnostics_reportf(NULL, COIL_DIAG_INFO, COIL_DIAG_CATEGORY_GENERAL, 0,
                          "  Peak allocated: %zu bytes", memory_stats.peak_allocated);
  coil_diagnostics_reportf(NULL, COIL_DIAG_INFO, COIL_DIAG_CATEGORY_GENERAL, 0,
                          "  Allocation count: %zu", memory_stats.allocation_count);
  coil_diagnostics_reportf(NULL, COIL_DIAG_INFO, COIL_DIAG_CATEGORY_GENERAL, 0,
                          "  Deallocation count: %zu", memory_stats.deallocation_count);
  coil_diagnostics_reportf(NULL, COIL_DIAG_INFO, COIL_DIAG_CATEGORY_GENERAL, 0,
                          "  Reallocation count: %zu", memory_stats.reallocation_count);
#endif
}