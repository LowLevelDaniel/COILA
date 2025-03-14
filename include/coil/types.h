/**
 * @file types.h
 * @brief COIL type system
 * @details This file contains definitions for the COIL type system, including
 *          primitive types, composite types, and type manipulation functions.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_TYPES_H
#define COIL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Type category masks
 */
#define COIL_TYPE_CATEGORY_MASK 0xF0000000
#define COIL_TYPE_WIDTH_MASK    0x0FF00000
#define COIL_TYPE_QUALIFIER_MASK 0x000FF000
#define COIL_TYPE_ATTRIBUTE_MASK 0x00000FFF

/**
 * @brief Shifts for type encoding
 */
#define COIL_TYPE_CATEGORY_SHIFT  28
#define COIL_TYPE_WIDTH_SHIFT     20
#define COIL_TYPE_QUALIFIER_SHIFT 12
#define COIL_TYPE_ATTRIBUTE_SHIFT 0

/**
 * @brief Type categories
 */
typedef enum {
  COIL_TYPE_CATEGORY_VOID     = 0x0,
  COIL_TYPE_CATEGORY_BOOLEAN  = 0x1,
  COIL_TYPE_CATEGORY_INTEGER  = 0x2,
  COIL_TYPE_CATEGORY_FLOAT    = 0x3,
  COIL_TYPE_CATEGORY_POINTER  = 0x4,
  COIL_TYPE_CATEGORY_VECTOR   = 0x5,
  COIL_TYPE_CATEGORY_ARRAY    = 0x6,
  COIL_TYPE_CATEGORY_STRUCT   = 0x7,
  COIL_TYPE_CATEGORY_FUNCTION = 0x8
} coil_type_category_t;

/**
 * @brief Type qualifiers
 */
typedef enum {
  COIL_TYPE_QUALIFIER_NONE     = 0x00,
  COIL_TYPE_QUALIFIER_UNSIGNED = 0x01,
  COIL_TYPE_QUALIFIER_CONST    = 0x02,
  COIL_TYPE_QUALIFIER_VOLATILE = 0x04,
  COIL_TYPE_QUALIFIER_RESTRICT = 0x08,
  COIL_TYPE_QUALIFIER_ATOMIC   = 0x10
} coil_type_qualifier_t;

/**
 * @brief Common predefined types
 */
#define COIL_TYPE_VOID    0x00000000
#define COIL_TYPE_BOOL    0x01000001
#define COIL_TYPE_INT8    0x02080000
#define COIL_TYPE_UINT8   0x02080100
#define COIL_TYPE_INT16   0x02100000
#define COIL_TYPE_UINT16  0x02100100
#define COIL_TYPE_INT32   0x02200000
#define COIL_TYPE_UINT32  0x02200100
#define COIL_TYPE_INT64   0x02400000
#define COIL_TYPE_UINT64  0x02400100
#define COIL_TYPE_FLOAT16 0x03100000
#define COIL_TYPE_FLOAT32 0x03200000
#define COIL_TYPE_FLOAT64 0x03400000
#define COIL_TYPE_PTR     0x04400000

/**
 * @brief COIL type definition
 * @details Represents a type in the COIL type system
 */
typedef uint32_t coil_type_t;

/**
 * @brief COIL structure field
 * @details Represents a field in a structure type
 */
typedef struct {
  const char *name;  /**< Field name */
  coil_type_t type;  /**< Field type */
  uint32_t    offset; /**< Byte offset within structure */
} coil_struct_field_t;

/**
 * @brief COIL structure type
 * @details Represents a composite structure type
 */
typedef struct {
  uint32_t            field_count; /**< Number of fields */
  coil_struct_field_t *fields;     /**< Array of fields */
  uint32_t            size;        /**< Total size in bytes */
  uint32_t            alignment;   /**< Structure alignment */
} coil_struct_type_t;

/**
 * @brief COIL function parameter
 * @details Represents a parameter in a function type
 */
typedef struct {
  const char *name; /**< Parameter name (optional) */
  coil_type_t type; /**< Parameter type */
} coil_function_param_t;

/**
 * @brief COIL function type
 * @details Represents a function type
 */
typedef struct {
  coil_type_t          return_type;  /**< Return type */
  uint32_t             param_count;  /**< Number of parameters */
  coil_function_param_t *params;     /**< Array of parameters */
  bool                 is_variadic;  /**< Whether function is variadic */
} coil_function_type_t;

/**
 * @brief Creates a new type
 * @param category Type category
 * @param width Width of the type in bits
 * @param qualifiers Type qualifiers
 * @param attributes Type-specific attributes
 * @return The encoded type
 */
coil_type_t coil_type_create(coil_type_category_t category, 
                             uint8_t width,
                             uint8_t qualifiers, 
                             uint16_t attributes);

/**
 * @brief Gets the category of a type
 * @param type The type to query
 * @return The type category
 */
coil_type_category_t coil_type_get_category(coil_type_t type);

/**
 * @brief Gets the width of a type in bits
 * @param type The type to query
 * @return The type width in bits
 */
uint8_t coil_type_get_width(coil_type_t type);

/**
 * @brief Gets the qualifiers of a type
 * @param type The type to query
 * @return The type qualifiers
 */
uint8_t coil_type_get_qualifiers(coil_type_t type);

/**
 * @brief Gets the attributes of a type
 * @param type The type to query
 * @return The type attributes
 */
uint16_t coil_type_get_attributes(coil_type_t type);

/**
 * @brief Creates a pointer type
 * @param base_type The type to point to
 * @param qualifiers Pointer qualifiers
 * @return The pointer type
 */
coil_type_t coil_type_create_pointer(coil_type_t base_type, uint8_t qualifiers);

/**
 * @brief Creates a vector type
 * @param element_type The vector element type
 * @param element_count The number of elements
 * @return The vector type
 */
coil_type_t coil_type_create_vector(coil_type_t element_type, uint8_t element_count);

/**
 * @brief Creates an array type
 * @param element_type The array element type
 * @param element_count The number of elements (0 for unsized array)
 * @return The array type
 */
coil_type_t coil_type_create_array(coil_type_t element_type, uint32_t element_count);

/**
 * @brief Creates a structure type
 * @param fields Array of structure fields
 * @param field_count Number of fields
 * @return Type ID for the structure
 */
coil_type_t coil_type_create_struct(coil_struct_field_t *fields, uint32_t field_count);

/**
 * @brief Creates a function type
 * @param return_type Function return type
 * @param params Array of parameter types
 * @param param_count Number of parameters
 * @param is_variadic Whether function is variadic
 * @return Type ID for the function
 */
coil_type_t coil_type_create_function(coil_type_t return_type,
                                      coil_function_param_t *params,
                                      uint32_t param_count,
                                      bool is_variadic);

/**
 * @brief Gets the size of a type in bytes
 * @param type The type to query
 * @return The size in bytes
 */
uint32_t coil_type_get_size(coil_type_t type);

/**
 * @brief Gets the alignment of a type in bytes
 * @param type The type to query
 * @return The alignment in bytes
 */
uint32_t coil_type_get_alignment(coil_type_t type);

/**
 * @brief Checks if two types are compatible
 * @param type1 First type
 * @param type2 Second type
 * @return true if types are compatible, false otherwise
 */
bool coil_type_is_compatible(coil_type_t type1, coil_type_t type2);

/**
 * @brief Gets a string representation of a type
 * @param type The type to convert
 * @param buffer Output buffer
 * @param size Size of the buffer
 * @return The buffer pointer
 */
char* coil_type_to_string(coil_type_t type, char *buffer, size_t size);

#endif /* COIL_TYPES_H */