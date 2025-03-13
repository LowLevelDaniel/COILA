/**
 * @file coil_internal.h
 * @brief Internal definitions for COIL Assembler.
 * 
 * This file defines the internal structures and functions used by
 * the COIL Assembler implementation.
 */

#ifndef COIL_INTERNAL_H
#define COIL_INTERNAL_H

#include "coil_assembler.h"

/**
 * @brief COIL binary file header.
 */
typedef struct {
  uint32_t magic;            /**< Magic number: "COIL" (0x434F494C) */
  uint32_t version;          /**< Version information (major.minor.patch) */
  uint32_t section_count;    /**< Number of sections */
  uint32_t flags;            /**< Module flags */
} coil_header_t;

/**
 * @brief COIL section types.
 */
typedef enum {
  SECTION_TYPE_TYPE = 1,      /**< Type definitions */
  SECTION_TYPE_FUNCTION,      /**< Function declarations */
  SECTION_TYPE_GLOBAL,        /**< Global variable declarations */
  SECTION_TYPE_CONSTANT,      /**< Constant data */
  SECTION_TYPE_CODE,          /**< Function implementations */
  SECTION_TYPE_RELOCATION,    /**< Relocation information */
  SECTION_TYPE_METADATA       /**< Optional metadata */
} section_type_t;

/**
 * @brief COIL section table entry.
 */
typedef struct {
  uint32_t section_type;      /**< Type of section */
  uint32_t offset;            /**< Byte offset from start of file */
  uint32_t size;              /**< Size of section in bytes */
} section_entry_t;

/**
 * @brief COIL type encoding (32-bit representation).
 *        [category:4][width:8][qualifiers:8][attributes:12]
 */
typedef uint32_t coil_type_encoding_t;

/**
 * @brief Type category constants.
 */
#define TYPE_CATEGORY_VOID     0x00
#define TYPE_CATEGORY_BOOLEAN  0x01
#define TYPE_CATEGORY_INTEGER  0x02
#define TYPE_CATEGORY_FLOAT    0x03
#define TYPE_CATEGORY_POINTER  0x04
#define TYPE_CATEGORY_VECTOR   0x05
#define TYPE_CATEGORY_ARRAY    0x06
#define TYPE_CATEGORY_STRUCTURE 0x07
#define TYPE_CATEGORY_FUNCTION 0x08

/**
 * @brief Type qualifier constants.
 */
#define TYPE_QUALIFIER_UNSIGNED 0x01
#define TYPE_QUALIFIER_CONST    0x02
#define TYPE_QUALIFIER_VOLATILE 0x04
#define TYPE_QUALIFIER_RESTRICT 0x08
#define TYPE_QUALIFIER_ATOMIC   0x10

/**
 * @brief COIL type information.
 */
typedef struct {
  coil_type_encoding_t encoding;  /**< Type encoding */
  char* name;                     /**< Type name (if available) */
  uint32_t size;                  /**< Size in bytes */
  uint32_t alignment;             /**< Alignment requirement */
  // Additional type-specific information can be added here
} coil_type_info_t;

/**
 * @brief COIL operand types.
 */
typedef enum {
  OPERAND_REGISTER,               /**< Register operand */
  OPERAND_IMMEDIATE,              /**< Immediate value */
  OPERAND_MEMORY,                 /**< Memory address */
  OPERAND_LABEL                   /**< Label reference */
} operand_type_t;

/**
 * @brief COIL operand.
 */
typedef struct {
  operand_type_t type;            /**< Operand type */
  coil_type_info_t data_type;     /**< Data type */
  union {
    uint32_t reg;                 /**< Register number */
    int64_t imm;                  /**< Immediate value */
    struct {
      uint32_t base;              /**< Base register */
      uint32_t index;             /**< Index register */
      uint32_t scale;             /**< Scale factor */
      int64_t disp;               /**< Displacement */
    } mem;                        /**< Memory address */
    char* label;                  /**< Label name */
  } value;
} coil_operand_t;

/**
 * @brief COIL instruction.
 */
typedef struct {
  uint8_t opcode;                 /**< Instruction opcode */
  uint8_t flags;                  /**< Instruction flags */
  uint8_t num_operands;           /**< Number of source operands */
  uint8_t dest;                   /**< Destination operand */
  coil_operand_t* operands;       /**< Source operands */
  coil_type_info_t type;          /**< Operation type */
} coil_instruction_t;

/**
 * @brief COIL basic block.
 */
typedef struct coil_basic_block {
  char* label;                    /**< Block label */
  coil_instruction_t* instructions; /**< Instructions in the block */
  size_t instr_count;             /**< Number of instructions */
  struct coil_basic_block* next;  /**< Next block in function */
} coil_basic_block_t;

/**
 * @brief COIL function.
 */
typedef struct {
  char* name;                     /**< Function name */
  coil_type_info_t return_type;   /**< Return type */
  coil_type_info_t* param_types;  /**< Parameter types */
  size_t param_count;             /**< Number of parameters */
  coil_basic_block_t* blocks;     /**< Basic blocks in the function */
  size_t block_count;             /**< Number of blocks */
  // Additional function attributes can be added here
} coil_function_t;

/**
 * @brief COIL global variable.
 */
typedef struct {
  char* name;                     /**< Variable name */
  coil_type_info_t type;          /**< Variable type */
  bool is_constant;               /**< Is this a constant? */
  uint8_t* initializer;           /**< Initial value */
  size_t init_size;               /**< Size of initializer */
} coil_global_t;

/**
 * @brief COIL module structure.
 */
typedef struct coil_module {
  char* name;                     /**< Module name */
  coil_function_t* functions;     /**< Functions in the module */
  size_t function_count;          /**< Number of functions */
  coil_global_t* globals;         /**< Global variables */
  size_t global_count;            /**< Number of global variables */
  coil_type_info_t* types;        /**< Type definitions */
  size_t type_count;              /**< Number of type definitions */
  uint8_t* code_section;          /**< Raw code section data */
  size_t code_size;               /**< Size of code section */
} coil_module_t;

/**
 * @brief Target architecture configuration.
 */
typedef struct target_config {
  char* name;                     /**< Target name */
  char* architecture;             /**< Architecture name (e.g., "x86_64") */
  char* vendor;                   /**< Vendor name (e.g., "generic") */
  coil_device_class_t device_class; /**< Device class */
  char** features;                /**< Supported features */
  size_t feature_count;           /**< Number of features */
  struct {
    uint32_t registers;           /**< Available hardware registers */
    uint32_t vector_width;        /**< Native vector width in bits */
    uint32_t min_alignment;       /**< Minimum memory alignment */
    char** memory_models;         /**< Supported memory ordering models */
    size_t memory_model_count;    /**< Number of memory models */
  } resources;
  struct {
    uint32_t vector_threshold;    /**< When to use vector operations */
    uint32_t unroll_factor;       /**< Default loop unrolling factor */
    bool use_fma;                 /**< Use fused multiply-add */
    int level;                    /**< Optimization level (0-3) */
  } optimization;
} target_config_t;

/**
 * @brief Function pointer type for instruction translation.
 */
typedef void (*translate_func_t)(coil_instruction_t*, void*);

/**
 * @brief Virtual instruction mapping.
 */
typedef struct {
  uint8_t coil_opcode;            /**< COIL opcode */
  char* native_mnemonic;          /**< Native instruction mnemonic */
  translate_func_t translate;     /**< Translation function */
} instr_mapping_t;

/**
 * @brief Native code output format enumeration.
 */
typedef enum {
  OUTPUT_FORMAT_ELF,              /**< Executable and Linkable Format */
  OUTPUT_FORMAT_MACHO,            /**< Mach-O Format */
  OUTPUT_FORMAT_COFF,             /**< Common Object File Format */
  OUTPUT_FORMAT_BINARY            /**< Raw binary format */
} output_format_t;

/**
 * @brief Virtual map for a target architecture.
 */
typedef struct {
  char* arch_name;                /**< Architecture name */
  coil_device_class_t device_class; /**< Device class */
  instr_mapping_t* mappings;      /**< Instruction mappings */
  size_t mapping_count;           /**< Number of mappings */
  output_format_t default_format; /**< Default output format */
  // Additional target-specific data and function pointers can be added here
} virtual_map_t;

/**
 * @brief Register allocation entry.
 */
typedef struct {
  uint32_t virtual_reg;           /**< Virtual register number */
  uint32_t physical_reg;          /**< Physical register number */
  bool is_allocated;              /**< Is register currently allocated? */
  bool is_dirty;                  /**< Does register need to be saved? */
} reg_alloc_entry_t;

/**
 * @brief Register allocator.
 */
typedef struct {
  reg_alloc_entry_t* entries;     /**< Register allocation entries */
  size_t entry_count;             /**< Number of entries */
  uint32_t available_regs;        /**< Bitmap of available registers */
} register_allocator_t;

/**
 * @brief Translation context.
 */
typedef struct {
  coil_module_t* module;          /**< COIL module */
  target_config_t* target;        /**< Target configuration */
  virtual_map_t* vmap;            /**< Virtual instruction map */
  register_allocator_t reg_alloc; /**< Register allocator */
  void* native_code;              /**< Generated native code */
  size_t code_size;               /**< Size of generated code */
  size_t code_capacity;           /**< Capacity of code buffer */
  output_format_t output_format;  /**< Output format */
  // Additional translation state can be added here
} translation_context_t;

/**
 * @brief COIL assembler structure.
 */
typedef struct coil_assembler {
  target_config_t* target;        /**< Target configuration */
  virtual_map_t* vmap;            /**< Virtual instruction map */
  coil_error_t last_error;        /**< Last error code */
  char error_message[256];        /**< Last error message */
  int optimization_level;         /**< Optimization level */
} coil_assembler_t;

// Function declarations for internal use

// Binary parsing functions
coil_error_t process_type_section(coil_module_t* module, const uint8_t* data, size_t size);
coil_error_t process_function_section(coil_module_t* module, const uint8_t* data, size_t size);
coil_error_t process_global_section(coil_module_t* module, const uint8_t* data, size_t size);
coil_error_t process_constant_section(coil_module_t* module, const uint8_t* data, size_t size);
coil_error_t process_code_section(coil_module_t* module, const uint8_t* data, size_t size);
coil_error_t process_relocation_section(coil_module_t* module, const uint8_t* data, size_t size);
coil_error_t process_metadata_section(coil_module_t* module, const uint8_t* data, size_t size);

// Virtual map functions
virtual_map_t* create_virtual_map(const char* arch_name, coil_device_class_t device_class);
coil_error_t add_instruction_mapping(virtual_map_t* map, uint8_t coil_opcode, 
                                   const char* native_mnemonic, 
                                   translate_func_t translate);
coil_error_t translate_instruction(translation_context_t* ctx, coil_instruction_t* instr);
void free_virtual_map(virtual_map_t* map);

// Architecture-specific initialization functions
virtual_map_t* initialize_x86_map(coil_device_class_t device_class);
virtual_map_t* initialize_arm_map(coil_device_class_t device_class);
virtual_map_t* initialize_riscv_map(coil_device_class_t device_class);
virtual_map_t* initialize_ppc_map(coil_device_class_t device_class);

// Translation functions
translation_context_t* create_translation_context(coil_module_t* module, 
                                              target_config_t* target,
                                              virtual_map_t* vmap);
coil_error_t translate_module(translation_context_t* ctx);
coil_error_t generate_native_code(translation_context_t* ctx, const char* output_filename);
void free_translation_context(translation_context_t* ctx);

// Register allocation functions
coil_error_t initialize_register_allocator(register_allocator_t* allocator, 
                                         uint32_t reg_count);
uint32_t allocate_register(register_allocator_t* allocator, uint32_t virtual_reg);
void free_register(register_allocator_t* allocator, uint32_t virtual_reg);
void free_register_allocator(register_allocator_t* allocator);

// Error handling
void set_error(coil_assembler_t* assembler, coil_error_t error, const char* format, ...);

// Utility functions
coil_type_info_t* find_type_by_encoding(coil_module_t* module, coil_type_encoding_t encoding);
coil_function_t* find_function_by_name(coil_module_t* module, const char* name);
const char* get_type_name(coil_type_encoding_t encoding);
uint32_t get_type_size(coil_type_encoding_t encoding, target_config_t* target);
uint32_t get_type_alignment(coil_type_encoding_t encoding, target_config_t* target);

#endif /* COIL_INTERNAL_H */