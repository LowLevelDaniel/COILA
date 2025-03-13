/**
 * @file memory_management.c
 * @brief Implementation of memory management utilities for the COIL assembler
 * 
 * This module implements memory management utilities, including allocators,
 * memory tracking, and memory leak detection.
 *
 * @author COIL Assembler Team
 * @date 2025-03-13
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "memory_management.h"
#include "logging.h"

/**
 * @brief Maximum number of tracked allocations
 */
#define MAX_TRACKED_ALLOCATIONS 10000

/**
 * @brief Default memory pool block size in bytes
 */
#define DEFAULT_BLOCK_SIZE 4096

/**
 * @brief Memory allocation record
 */
typedef struct {
  void* ptr;                  /**< Allocated memory pointer */
  size_t size;                /**< Size of allocation in bytes */
  const char* file;           /**< Source file where allocation occurred */
  int line;                   /**< Line number where allocation occurred */
  const char* function;       /**< Function where allocation occurred */
} allocation_record_t;

/**
 * @brief Block in a memory pool
 */
typedef struct block_t {
  void* memory;               /**< Block memory */
  size_t size;                /**< Block size in bytes */
  size_t used;                /**< Used bytes in block */
  struct block_t* next;       /**< Next block in pool */
} block_t;

/**
 * @brief Memory pool implementation
 */
struct memory_pool_t {
  size_t obj_size;            /**< Size of each object in the pool */
  void** free_list;           /**< List of free objects */
  size_t free_count;          /**< Number of free objects */
  size_t capacity;            /**< Total capacity of free list */
  block_t* blocks;            /**< Linked list of memory blocks */
};

/**
 * @brief Global memory management state
 */
static struct {
  bool initialized;           /**< Whether memory management is initialized */
  bool tracking_enabled;      /**< Whether memory tracking is enabled */
  allocation_record_t* allocations; /**< Array of allocation records */
  size_t allocation_count;    /**< Number of active allocations */
  memory_stats_t stats;       /**< Memory statistics */
} memory_state = {
  false,    /* initialized */
  false,    /* tracking_enabled */
  NULL,     /* allocations */
  0,        /* allocation_count */
  {0, 0, 0, 0, 0} /* stats */
};

error_t memory_init(bool enable_tracking) {
  if (memory_state.initialized) {
    return ERROR_NONE;  /* Already initialized */
  }
  
  memory_state.tracking_enabled = enable_tracking;
  
  /* Initialize statistics */
  memory_state.stats.current_usage = 0;
  memory_state.stats.peak_usage = 0;
  memory_state.stats.total_allocations = 0;
  memory_state.stats.total_frees = 0;
  memory_state.stats.active_allocations = 0;
  
  /* Allocate tracking array if enabled */
  if (enable_tracking) {
    memory_state.allocations = calloc(MAX_TRACKED_ALLOCATIONS, sizeof(allocation_record_t));
    if (memory_state.allocations == NULL) {
      return ERROR_MEMORY;
    }
    memory_state.allocation_count = 0;
  }
  
  memory_state.initialized = true;
  return ERROR_NONE;
}

error_t memory_shutdown(void) {
  if (!memory_state.initialized) {
    return ERROR_NONE;  /* Not initialized */
  }
  
  /* Check for leaks if tracking enabled */
  if (memory_state.tracking_enabled && memory_state.allocation_count > 0) {
    log_warning("Memory leak detected: %zu allocations still active", 
               memory_state.allocation_count);
  }
  
  /* Free the tracking array */
  if (memory_state.allocations != NULL) {
    free(memory_state.allocations);
    memory_state.allocations = NULL;
  }
  
  memory_state.initialized = false;
  return ERROR_NONE;
}

/**
 * @brief Adds an allocation to the tracking array
 *
 * @param[in] ptr Allocated memory pointer
 * @param[in] size Size of allocation in bytes
 * @param[in] file Source file where allocation occurred
 * @param[in] line Line number where allocation occurred
 * @param[in] function Function where allocation occurred
 * @return true if tracking succeeded, false otherwise
 */
static bool track_allocation(
  void* ptr,
  size_t size,
  const char* file,
  int line,
  const char* function
) {
  if (!memory_state.tracking_enabled || ptr == NULL) {
    return false;
  }
  
  /* Check if we've reached the tracking limit */
  if (memory_state.allocation_count >= MAX_TRACKED_ALLOCATIONS) {
    log_error("Maximum tracked allocations reached");
    return false;
  }
  
  /* Find an empty slot */
  for (size_t i = 0; i < MAX_TRACKED_ALLOCATIONS; i++) {
    if (memory_state.allocations[i].ptr == NULL) {
      memory_state.allocations[i].ptr = ptr;
      memory_state.allocations[i].size = size;
      memory_state.allocations[i].file = file;
      memory_state.allocations[i].line = line;
      memory_state.allocations[i].function = function;
      memory_state.allocation_count++;
      return true;
    }
  }
  
  /* This should not happen if allocation_count < MAX_TRACKED_ALLOCATIONS */
  log_error("Failed to track allocation");
  return false;
}

/**
 * @brief Removes an allocation from the tracking array
 *
 * @param[in] ptr Memory pointer to free
 * @return Size of freed allocation, or 0 if not found
 */
static size_t untrack_allocation(void* ptr) {
  if (!memory_state.tracking_enabled || ptr == NULL) {
    return 0;
  }
  
  /* Find the allocation in the tracking array */
  for (size_t i = 0; i < MAX_TRACKED_ALLOCATIONS; i++) {
    if (memory_state.allocations[i].ptr == ptr) {
      size_t size = memory_state.allocations[i].size;
      memory_state.allocations[i].ptr = NULL;
      memory_state.allocations[i].size = 0;
      memory_state.allocations[i].file = NULL;
      memory_state.allocations[i].line = 0;
      memory_state.allocations[i].function = NULL;
      memory_state.allocation_count--;
      return size;
    }
  }
  
  log_warning("Freeing untracked allocation at %p", ptr);
  return 0;
}

void* memory_alloc_ex(
  size_t size,
  const char* file,
  int line,
  const char* function
) {
  if (!memory_state.initialized) {
    if (memory_init(false) != ERROR_NONE) {
      return NULL;
    }
  }
  
  /* Allocate memory */
  void* ptr = malloc(size);
  if (ptr == NULL) {
    log_error("Memory allocation failed for %zu bytes at %s:%d", 
             size, file, line);
    return NULL;
  }
  
  /* Update statistics */
  memory_state.stats.total_allocations++;
  memory_state.stats.active_allocations++;
  memory_state.stats.current_usage += size;
  if (memory_state.stats.current_usage > memory_state.stats.peak_usage) {
    memory_state.stats.peak_usage = memory_state.stats.current_usage;
  }
  
  /* Track the allocation if enabled */
  track_allocation(ptr, size, file, line, function);
  
  return ptr;
}

void* memory_calloc_ex(
  size_t count,
  size_t size,
  const char* file,
  int line,
  const char* function
) {
  size_t total_size = count * size;
  
  /* Check for overflow */
  if (count > 0 && total_size / count != size) {
    log_error("Memory allocation size overflow at %s:%d", file, line);
    return NULL;
  }
  
  /* Allocate memory */
  void* ptr = memory_alloc_ex(total_size, file, line, function);
  if (ptr != NULL) {
    /* Zero out the memory */
    memset(ptr, 0, total_size);
  }
  
  return ptr;
}

void* memory_realloc_ex(
  void* ptr,
  size_t size,
  const char* file,
  int line,
  const char* function
) {
  if (!memory_state.initialized) {
    if (memory_init(false) != ERROR_NONE) {
      return NULL;
    }
  }
  
  if (ptr == NULL) {
    /* If ptr is NULL, this is equivalent to memory_alloc */
    return memory_alloc_ex(size, file, line, function);
  }
  
  if (size == 0) {
    /* If size is 0, this is equivalent to memory_free */
    memory_free(ptr);
    return NULL;
  }
  
  /* Get the current size for stats update */
  size_t old_size = 0;
  if (memory_state.tracking_enabled) {
    /* Find the allocation in the tracking array */
    for (size_t i = 0; i < MAX_TRACKED_ALLOCATIONS; i++) {
      if (memory_state.allocations[i].ptr == ptr) {
        old_size = memory_state.allocations[i].size;
        break;
      }
    }
  }
  
  /* Reallocate memory */
  void* new_ptr = realloc(ptr, size);
  if (new_ptr == NULL) {
    log_error("Memory reallocation failed for %zu bytes at %s:%d", 
             size, file, line);
    return NULL;
  }
  
  /* Update tracking if pointer changed */
  if (memory_state.tracking_enabled && new_ptr != ptr) {
    /* Remove old tracking record */
    untrack_allocation(ptr);
    
    /* Add new tracking record */
    track_allocation(new_ptr, size, file, line, function);
  } else if (memory_state.tracking_enabled) {
    /* Update size in existing tracking record */
    for (size_t i = 0; i < MAX_TRACKED_ALLOCATIONS; i++) {
      if (memory_state.allocations[i].ptr == ptr) {
        memory_state.allocations[i].size = size;
        break;
      }
    }
  }
  
  /* Update statistics */
  if (old_size > 0) {
    memory_state.stats.current_usage -= old_size;
  }
  memory_state.stats.current_usage += size;
  if (memory_state.stats.current_usage > memory_state.stats.peak_usage) {
    memory_state.stats.peak_usage = memory_state.stats.current_usage;
  }
  
  return new_ptr;
}

error_t memory_free(void* ptr) {
  if (!memory_state.initialized || ptr == NULL) {
    return ERROR_NONE;
  }
  
  /* Remove from tracking and get size */
  size_t size = untrack_allocation(ptr);
  
  /* Free the memory */
  free(ptr);
  
  /* Update statistics */
  memory_state.stats.total_frees++;
  memory_state.stats.active_allocations--;
  if (size > 0) {
    memory_state.stats.current_usage -= size;
  }
  
  return ERROR_NONE;
}

memory_pool_t* memory_pool_create(size_t obj_size, size_t initial_capacity) {
  if (!memory_state.initialized) {
    if (memory_init(false) != ERROR_NONE) {
      return NULL;
    }
  }
  
  /* Ensure object size is at least sizeof(void*) for free list */
  if (obj_size < sizeof(void*)) {
    obj_size = sizeof(void*);
  }
  
  /* Align object size to pointer size for better performance */
  size_t align = sizeof(void*);
  obj_size = (obj_size + align - 1) & ~(align - 1);
  
  /* Allocate pool structure */
  memory_pool_t* pool = memory_alloc(sizeof(memory_pool_t));
  if (pool == NULL) {
    return NULL;
  }
  
  /* Initialize pool fields */
  pool->obj_size = obj_size;
  pool->free_list = NULL;
  pool->free_count = 0;
  pool->capacity = 0;
  pool->blocks = NULL;
  
  /* If initial capacity is specified, pre-allocate objects */
  if (initial_capacity > 0) {
    /* Calculate block size */
    size_t block_size = initial_capacity * obj_size;
    if (block_size < DEFAULT_BLOCK_SIZE) {
      block_size = DEFAULT_BLOCK_SIZE;
    }
    
    /* Allocate block */
    block_t* block = memory_alloc(sizeof(block_t));
    if (block == NULL) {
      memory_free(pool);
      return NULL;
    }
    
    block->memory = memory_alloc(block_size);
    if (block->memory == NULL) {
      memory_free(block);
      memory_free(pool);
      return NULL;
    }
    
    block->size = block_size;
    block->used = 0;
    block->next = NULL;
    pool->blocks = block;
    
    /* Allocate free list */
    pool->capacity = initial_capacity;
    pool->free_list = memory_alloc(initial_capacity * sizeof(void*));
    if (pool->free_list == NULL) {
      memory_free(block->memory);
      memory_free(block);
      memory_free(pool);
      return NULL;
    }
    
    /* Pre-allocate objects */
    size_t count = block_size / obj_size;
    if (count > initial_capacity) {
      count = initial_capacity;
    }
    
    uint8_t* mem = (uint8_t*)block->memory;
    for (size_t i = 0; i < count; i++) {
      pool->free_list[i] = mem + (i * obj_size);
      pool->free_count++;
    }
    
    block->used = count * obj_size;
  }
  
  return pool;
}

void* memory_pool_alloc(memory_pool_t* pool) {
  if (pool == NULL) {
    return NULL;
  }
  
  void* ptr = NULL;
  
  /* Check if we have free objects */
  if (pool->free_count > 0) {
    /* Use object from free list */
    ptr = pool->free_list[--pool->free_count];
  } else {
    /* Need to allocate new objects */
    
    /* Find a block with enough space */
    block_t* block = pool->blocks;
    while (block != NULL && block->size - block->used < pool->obj_size) {
      block = block->next;
    }
    
    /* If no suitable block found, allocate a new one */
    if (block == NULL) {
      size_t block_size = DEFAULT_BLOCK_SIZE;
      if (block_size < pool->obj_size * 8) {
        block_size = pool->obj_size * 8;
      }
      
      block = memory_alloc(sizeof(block_t));
      if (block == NULL) {
        return NULL;
      }
      
      block->memory = memory_alloc(block_size);
      if (block->memory == NULL) {
        memory_free(block);
        return NULL;
      }
      
      block->size = block_size;
      block->used = 0;
      block->next = pool->blocks;
      pool->blocks = block;
    }
    
    /* Allocate object from block */
    ptr = (uint8_t*)block->memory + block->used;
    block->used += pool->obj_size;
  }
  
  /* Zero the memory */
  memset(ptr, 0, pool->obj_size);
  
  return ptr;
}

error_t memory_pool_free(memory_pool_t* pool, void* ptr) {
  if (pool == NULL || ptr == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Validate that the pointer belongs to one of our blocks */
  block_t* block = pool->blocks;
  bool valid = false;
  
  while (block != NULL && !valid) {
    uint8_t* mem_start = (uint8_t*)block->memory;
    uint8_t* mem_end = mem_start + block->size;
    
    if ((uint8_t*)ptr >= mem_start && (uint8_t*)ptr < mem_end) {
      /* Check alignment */
      if (((uint8_t*)ptr - mem_start) % pool->obj_size == 0) {
        valid = true;
      }
    }
    
    block = block->next;
  }
  
  if (!valid) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Ensure we have space in the free list */
  if (pool->free_count >= pool->capacity) {
    size_t new_capacity = pool->capacity == 0 ? 16 : pool->capacity * 2;
    void** new_list = memory_realloc(pool->free_list, new_capacity * sizeof(void*));
    if (new_list == NULL) {
      return ERROR_MEMORY;
    }
    
    pool->free_list = new_list;
    pool->capacity = new_capacity;
  }
  
  /* Add to free list */
  pool->free_list[pool->free_count++] = ptr;
  
  return ERROR_NONE;
}

error_t memory_pool_destroy(memory_pool_t* pool) {
  if (pool == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Free all blocks */
  block_t* block = pool->blocks;
  while (block != NULL) {
    block_t* next = block->next;
    memory_free(block->memory);
    memory_free(block);
    block = next;
  }
  
  /* Free the free list */
  memory_free(pool->free_list);
  
  /* Free the pool structure */
  memory_free(pool);
  
  return ERROR_NONE;
}

error_t memory_get_stats(memory_stats_t* stats) {
  if (!memory_state.initialized) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  if (stats == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Copy statistics */
  *stats = memory_state.stats;
  
  return ERROR_NONE;
}

bool memory_check_leaks(size_t* leak_count) {
  if (!memory_state.initialized || !memory_state.tracking_enabled) {
    if (leak_count != NULL) {
      *leak_count = 0;
    }
    return false;
  }
  
  if (leak_count != NULL) {
    *leak_count = memory_state.allocation_count;
  }
  
  return memory_state.allocation_count > 0;
}

error_t memory_dump_leaks(const char* filename) {
  if (!memory_state.initialized || !memory_state.tracking_enabled) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  if (filename == NULL) {
    return ERROR_INVALID_ARGUMENT;
  }
  
  /* Open dump file */
  FILE* file = fopen(filename, "w");
  if (file == NULL) {
    return ERROR_FILE_IO;
  }
  
  /* Write header */
  fprintf(file, "Memory Leak Report\n");
  fprintf(file, "=================\n\n");
  fprintf(file, "Total leaks: %zu\n", memory_state.allocation_count);
  fprintf(file, "Total leaked bytes: %zu\n\n", memory_state.stats.current_usage);
  fprintf(file, "%-16s %-8s %-32s %-8s %s\n", 
         "Address", "Size", "Function", "Line", "File");
  fprintf(file, "--------------------------------------------------------------------------------\n");
  
  /* Write each leak */
  size_t total_size = 0;
  for (size_t i = 0; i < MAX_TRACKED_ALLOCATIONS; i++) {
    if (memory_state.allocations[i].ptr != NULL) {
      fprintf(file, "0x%-14p %-8zu %-32s %-8d %s\n", 
             memory_state.allocations[i].ptr,
             memory_state.allocations[i].size,
             memory_state.allocations[i].function,
             memory_state.allocations[i].line,
             memory_state.allocations[i].file);
      
      total_size += memory_state.allocations[i].size;
    }
  }
  
  /* Write footer */
  fprintf(file, "--------------------------------------------------------------------------------\n");
  fprintf(file, "Total: %zu leaks, %zu bytes\n", memory_state.allocation_count, total_size);
  
  fclose(file);
  return ERROR_NONE;
}