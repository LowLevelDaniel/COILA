/**
 * @file config_loader.c
 * @brief Configuration loader implementation
 * @details Implementation of configuration loading and parsing for the COIL assembler.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "coil-assembler/config.h"
#include "../utils/memory.c"
#include "../utils/logging.c"

/* Maximum key length in configuration file */
#define MAX_KEY_LENGTH 128

/* Maximum value length in configuration file */
#define MAX_VALUE_LENGTH 4096

/* Maximum line length in configuration file */
#define MAX_LINE_LENGTH 4096

/* Maximum nesting depth for configuration objects */
#define MAX_NESTING_DEPTH 16

/**
 * @brief Parser state structure
 */
typedef struct {
  FILE* file;                   /* Input file */
  const char* text;             /* Input text (for string parsing) */
  size_t text_len;              /* Length of input text */
  size_t pos;                   /* Current position in input */
  int line;                     /* Current line number */
  int column;                   /* Current column number */
  char error_message[256];      /* Error message buffer */
  int has_error;                /* Whether an error occurred */
} parser_state_t;

/**
 * @brief Parser stack frame
 */
typedef struct {
  coil_config_value_t* value;   /* Current object/array value */
  int is_array;                 /* Whether the current container is an array */
  char key[MAX_KEY_LENGTH];     /* Current key (for objects) */
  int has_key;                  /* Whether a key has been parsed */
} parser_frame_t;

/* Forward declarations of parsing functions */
static coil_config_value_t* parse_value(parser_state_t* state);
static coil_config_value_t* parse_object(parser_state_t* state);
static coil_config_value_t* parse_array(parser_state_t* state);
static coil_config_value_t* parse_string(parser_state_t* state);
static coil_config_value_t* parse_number(parser_state_t* state);
static coil_config_value_t* parse_bool(parser_state_t* state, int value);
static coil_config_value_t* parse_null(parser_state_t* state);

/**
 * @brief Create a new configuration value
 * @param type Type of the value
 * @return New configuration value or NULL on failure
 */
static coil_config_value_t* create_value(coil_config_type_t type) {
  coil_config_value_t* value = (coil_config_value_t*)coil_malloc(sizeof(coil_config_value_t));
  if (!value) {
    return NULL;
  }
  
  value->type = type;
  
  /* Initialize value based on type */
  switch (type) {
    case COIL_CONFIG_TYPE_BOOL:
      value->data.bool_value = 0;
      break;
      
    case COIL_CONFIG_TYPE_INT:
      value->data.int_value = 0;
      break;
      
    case COIL_CONFIG_TYPE_FLOAT:
      value->data.float_value = 0.0;
      break;
      
    case COIL_CONFIG_TYPE_STRING:
      value->data.string_value = NULL;
      break;
      
    case COIL_CONFIG_TYPE_ARRAY:
      value->data.array.elements = NULL;
      value->data.array.count = 0;
      break;
      
    case COIL_CONFIG_TYPE_OBJECT:
      value->data.object.keys = NULL;
      value->data.object.values = NULL;
      value->data.object.count = 0;
      break;
      
    default:
      /* Invalid type */
      coil_free(value, sizeof(coil_config_value_t));
      return NULL;
  }
  
  return value;
}

/**
 * @brief Skip whitespace in the input
 * @param state Parser state
 */
static void skip_whitespace(parser_state_t* state) {
  if (state->file) {
    /* File mode */
    int c;
    while ((c = fgetc(state->file)) != EOF) {
      if (c == '\n') {
        state->line++;
        state->column = 0;
      } else if (c == '\r') {
        /* Skip */
      } else if (isspace(c)) {
        state->column++;
      } else {
        /* Put back non-whitespace character */
        ungetc(c, state->file);
        break;
      }
    }
  } else {
    /* String mode */
    while (state->pos < state->text_len) {
      char c = state->text[state->pos];
      if (c == '\n') {
        state->line++;
        state->column = 0;
        state->pos++;
      } else if (c == '\r') {
        state->pos++;
      } else if (isspace(c)) {
        state->column++;
        state->pos++;
      } else {
        break;
      }
    }
  }
}

/**
 * @brief Skip a comment in the input
 * @param state Parser state
 */
static void skip_comment(parser_state_t* state) {
  if (state->file) {
    /* File mode */
    int c = fgetc(state->file);
    if (c == '/') {
      /* Single-line comment */
      while ((c = fgetc(state->file)) != EOF) {
        if (c == '\n') {
          state->line++;
          state->column = 0;
          break;
        }
      }
    } else if (c == '*') {
      /* Multi-line comment */
      int prev = 0;
      while ((c = fgetc(state->file)) != EOF) {
        if (c == '/' && prev == '*') {
          break;
        }
        if (c == '\n') {
          state->line++;
          state->column = 0;
        } else {
          state->column++;
        }
        prev = c;
      }
    } else {
      /* Not a comment, put back the character */
      ungetc(c, state->file);
    }
  } else {
    /* String mode */
    if (state->pos + 1 < state->text_len && state->text[state->pos + 1] == '/') {
      /* Single-line comment */
      state->pos += 2;
      while (state->pos < state->text_len) {
        char c = state->text[state->pos++];
        if (c == '\n') {
          state->line++;
          state->column = 0;
          break;
        }
      }
    } else if (state->pos + 1 < state->text_len && state->text[state->pos + 1] == '*') {
      /* Multi-line comment */
      state->pos += 2;
      char prev = 0;
      while (state->pos < state->text_len) {
        char c = state->text[state->pos++];
        if (c == '/' && prev == '*') {
          break;
        }
        if (c == '\n') {
          state->line++;
          state->column = 0;
        } else {
          state->column++;
        }
        prev = c;
      }
    }
  }
}

/**
 * @brief Get the next character from the input
 * @param state Parser state
 * @return Next character or EOF on end of input
 */
static int next_char(parser_state_t* state) {
  int c;
  
  if (state->file) {
    /* File mode */
    c = fgetc(state->file);
  } else {
    /* String mode */
    if (state->pos >= state->text_len) {
      return EOF;
    }
    c = state->text[state->pos++];
  }
  
  if (c == '\n') {
    state->line++;
    state->column = 0;
  } else {
    state->column++;
  }
  
  return c;
}

/**
 * @brief Peek at the next character without consuming it
 * @param state Parser state
 * @return Next character or EOF on end of input
 */
static int peek_char(parser_state_t* state) {
  int c;
  
  if (state->file) {
    /* File mode */
    c = fgetc(state->file);
    ungetc(c, state->file);
  } else {
    /* String mode */
    if (state->pos >= state->text_len) {
      return EOF;
    }
    c = state->text[state->pos];
  }
  
  return c;
}

/**
 * @brief Put back a character to the input
 * @param state Parser state
 * @param c Character to put back
 */
static void put_back_char(parser_state_t* state, int c) {
  if (state->file) {
    /* File mode */
    ungetc(c, state->file);
  } else {
    /* String mode */
    if (state->pos > 0) {
      state->pos--;
    }
  }
  
  if (c == '\n') {
    state->line--;
    /* We don't know the previous column, so set to 0 */
    state->column = 0;
  } else {
    state->column--;
  }
}

/**
 * @brief Set an error in the parser state
 * @param state Parser state
 * @param format Error message format
 * @param ... Format arguments
 */
static void set_error(parser_state_t* state, const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(state->error_message, sizeof(state->error_message), format, args);
  va_end(args);
  
  state->has_error = 1;
}

/**
 * @brief Parse a string value
 * @param state Parser state
 * @return String configuration value or NULL on failure
 */
static coil_config_value_t* parse_string(parser_state_t* state) {
  /* Check for opening quote */
  int quote = next_char(state);
  if (quote != '"') {
    set_error(state, "Expected string literal at line %d, column %d", 
             state->line, state->column);
    return NULL;
  }
  
  /* Buffer to collect string content */
  char buffer[MAX_VALUE_LENGTH];
  size_t len = 0;
  
  /* Read string content */
  while (1) {
    int c = next_char(state);
    if (c == EOF) {
      set_error(state, "Unexpected end of file in string literal at line %d", 
               state->line);
      return NULL;
    }
    
    if (c == '"') {
      /* End of string */
      break;
    }
    
    if (c == '\\') {
      /* Escape sequence */
      c = next_char(state);
      switch (c) {
        case '"': c = '"'; break;
        case '\\': c = '\\'; break;
        case '/': c = '/'; break;
        case 'b': c = '\b'; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        default:
          set_error(state, "Invalid escape sequence '\\%c' at line %d, column %d", 
                   c, state->line, state->column);
          return NULL;
      }
    }
    
    /* Add character to buffer */
    if (len < sizeof(buffer) - 1) {
      buffer[len++] = (char)c;
    } else {
      set_error(state, "String literal too long at line %d", state->line);
      return NULL;
    }
  }
  
  /* Null-terminate the string */
  buffer[len] = '\0';
  
  /* Create string value */
  coil_config_value_t* value = create_value(COIL_CONFIG_TYPE_STRING);
  if (!value) {
    set_error(state, "Failed to allocate string value");
    return NULL;
  }
  
  /* Copy string content */
  value->data.string_value = coil_strdup(buffer);
  if (!value->data.string_value) {
    coil_free(value, sizeof(coil_config_value_t));
    set_error(state, "Failed to allocate string content");
    return NULL;
  }
  
  return value;
}

/**
 * @brief Parse a number value
 * @param state Parser state
 * @return Number configuration value or NULL on failure
 */
static coil_config_value_t* parse_number(parser_state_t* state) {
  /* Buffer to collect number content */
  char buffer[MAX_VALUE_LENGTH];
  size_t len = 0;
  int is_float = 0;
  
  /* Read sign if present */
  int c = peek_char(state);
  if (c == '-' || c == '+') {
    buffer[len++] = (char)next_char(state);
  }
  
  /* Read integer part */
  while (1) {
    c = peek_char(state);
    if (c == EOF || !isdigit(c)) {
      break;
    }
    buffer[len++] = (char)next_char(state);
  }
  
  /* Read decimal part if present */
  c = peek_char(state);
  if (c == '.') {
    is_float = 1;
    buffer[len++] = (char)next_char(state);
    
    /* Read fractional digits */
    while (1) {
      c = peek_char(state);
      if (c == EOF || !isdigit(c)) {
        break;
      }
      buffer[len++] = (char)next_char(state);
    }
  }
  
  /* Read exponent if present */
  c = peek_char(state);
  if (c == 'e' || c == 'E') {
    is_float = 1;
    buffer[len++] = (char)next_char(state);
    
    /* Read exponent sign if present */
    c = peek_char(state);
    if (c == '-' || c == '+') {
      buffer[len++] = (char)next_char(state);
    }
    
    /* Read exponent digits */
    int has_digits = 0;
    while (1) {
      c = peek_char(state);
      if (c == EOF || !isdigit(c)) {
        break;
      }
      buffer[len++] = (char)next_char(state);
      has_digits = 1;
    }
    
    if (!has_digits) {
      set_error(state, "Invalid number format: exponent has no digits at line %d", 
               state->line);
      return NULL;
    }
  }
  
  /* Null-terminate the buffer */
  buffer[len] = '\0';
  
  /* Create value based on type */
  coil_config_value_t* value;
  if (is_float) {
    value = create_value(COIL_CONFIG_TYPE_FLOAT);
    if (!value) {
      set_error(state, "Failed to allocate float value");
      return NULL;
    }
    value->data.float_value = atof(buffer);
  } else {
    value = create_value(COIL_CONFIG_TYPE_INT);
    if (!value) {
      set_error(state, "Failed to allocate integer value");
      return NULL;
    }
    value->data.int_value = atoll(buffer);
  }
  
  return value;
}

/**
 * @brief Parse a boolean value
 * @param state Parser state
 * @param value Boolean value (0 for false, non-zero for true)
 * @return Boolean configuration value or NULL on failure
 */
static coil_config_value_t* parse_bool(parser_state_t* state, int value) {
  /* Create boolean value */
  coil_config_value_t* config_value = create_value(COIL_CONFIG_TYPE_BOOL);
  if (!config_value) {
    set_error(state, "Failed to allocate boolean value");
    return NULL;
  }
  
  config_value->data.bool_value = value ? 1 : 0;
  
  return config_value;
}

/**
 * @brief Parse a null value
 * @param state Parser state
 * @return Null configuration value or NULL on failure
 */
static coil_config_value_t* parse_null(parser_state_t* state) {
  /* Create null value */
  coil_config_value_t* value = create_value(COIL_CONFIG_TYPE_NONE);
  if (!value) {
    set_error(state, "Failed to allocate null value");
    return NULL;
  }
  
  return value;
}

/**
 * @brief Parse an array value
 * @param state Parser state
 * @return Array configuration value or NULL on failure
 */
static coil_config_value_t* parse_array(parser_state_t* state) {
  /* Check for opening bracket */
  int c = next_char(state);
  if (c != '[') {
    set_error(state, "Expected array opening bracket '[' at line %d, column %d", 
             state->line, state->column);
    return NULL;
  }
  
  /* Create array value */
  coil_config_value_t* array = create_value(COIL_CONFIG_TYPE_ARRAY);
  if (!array) {
    set_error(state, "Failed to allocate array value");
    return NULL;
  }
  
  /* Parse array elements */
  int first_element = 1;
  while (1) {
    /* Skip whitespace and comments */
    skip_whitespace(state);
    c = peek_char(state);
    if (c == '/') {
      next_char(state);
      skip_comment(state);
      continue;
    }
    
    /* Check for closing bracket */
    if (c == ']') {
      next_char(state);
      break;
    }
    
    /* Check for comma between elements */
    if (!first_element) {
      c = next_char(state);
      if (c != ',') {
        set_error(state, "Expected comma between array elements at line %d, column %d", 
                 state->line, state->column);
        /* Free array */
        coil_config_destroy((coil_config_t*)array);
        return NULL;
      }
      skip_whitespace(state);
    }
    
    /* Parse element value */
    coil_config_value_t* element = parse_value(state);
    if (!element) {
      /* Error already set */
      coil_config_destroy((coil_config_t*)array);
      return NULL;
    }
    
    /* Add element to array */
    coil_config_value_t** new_elements = (coil_config_value_t**)coil_realloc(
        array->data.array.elements,
        array->data.array.count * sizeof(coil_config_value_t*),
        (array->data.array.count + 1) * sizeof(coil_config_value_t*));
    
    if (!new_elements) {
      set_error(state, "Failed to resize array elements");
      coil_config_destroy((coil_config_t*)array);
      coil_config_destroy((coil_config_t*)element);
      return NULL;
    }
    
    array->data.array.elements = new_elements;
    array->data.array.elements[array->data.array.count++] = element;
    
    first_element = 0;
  }
  
  return array;
}

/**
 * @brief Parse an object value
 * @param state Parser state
 * @return Object configuration value or NULL on failure
 */
static coil_config_value_t* parse_object(parser_state_t* state) {
  /* Check for opening brace */
  int c = next_char(state);
  if (c != '{') {
    set_error(state, "Expected object opening brace '{' at line %d, column %d", 
             state->line, state->column);
    return NULL;
  }
  
  /* Create object value */
  coil_config_value_t* object = create_value(COIL_CONFIG_TYPE_OBJECT);
  if (!object) {
    set_error(state, "Failed to allocate object value");
    return NULL;
  }
  
  /* Parse object properties */
  int first_property = 1;
  while (1) {
    /* Skip whitespace and comments */
    skip_whitespace(state);
    c = peek_char(state);
    if (c == '/') {
      next_char(state);
      skip_comment(state);
      continue;
    }
    
    /* Check for closing brace */
    if (c == '}') {
      next_char(state);
      break;
    }
    
    /* Check for comma between properties */
    if (!first_property) {
      c = next_char(state);
      if (c != ',') {
        set_error(state, "Expected comma between object properties at line %d, column %d", 
                 state->line, state->column);
        /* Free object */
        coil_config_destroy((coil_config_t*)object);
        return NULL;
      }
      skip_whitespace(state);
    }
    
    /* Parse property key */
    coil_config_value_t* key_value = parse_string(state);
    if (!key_value) {
      /* Error already set */
      coil_config_destroy((coil_config_t*)object);
      return NULL;
    }
    
    /* Extract key string */
    char* key = key_value->data.string_value;
    key_value->data.string_value = NULL;
    coil_config_destroy((coil_config_t*)key_value);
    
    /* Skip whitespace */
    skip_whitespace(state);
    
    /* Check for colon */
    c = next_char(state);
    if (c != ':') {
      set_error(state, "Expected colon after property key at line %d, column %d", 
               state->line, state->column);
      coil_free(key, strlen(key) + 1);
      coil_config_destroy((coil_config_t*)object);
      return NULL;
    }
    
    /* Skip whitespace */
    skip_whitespace(state);
    
    /* Parse property value */
    coil_config_value_t* value = parse_value(state);
    if (!value) {
      /* Error already set */
      coil_free(key, strlen(key) + 1);
      coil_config_destroy((coil_config_t*)object);
      return NULL;
    }
    
    /* Add property to object */
    char** new_keys = (char**)coil_realloc(
        object->data.object.keys,
        object->data.object.count * sizeof(char*),
        (object->data.object.count + 1) * sizeof(char*));
    
    coil_config_value_t** new_values = (coil_config_value_t**)coil_realloc(
        object->data.object.values,
        object->data.object.count * sizeof(coil_config_value_t*),
        (object->data.object.count + 1) * sizeof(coil_config_value_t*));
    
    if (!new_keys || !new_values) {
      set_error(state, "Failed to resize object properties");
      coil_free(key, strlen(key) + 1);
      coil_config_destroy((coil_config_t*)value);
      coil_config_destroy((coil_config_t*)object);
      return NULL;
    }
    
    object->data.object.keys = new_keys;
    object->data.object.values = new_values;
    object->data.object.keys[object->data.object.count] = key;
    object->data.object.values[object->data.object.count] = value;
    object->data.object.count++;
    
    first_property = 0;
  }
  
  return object;
}

/**
 * @brief Parse an identifier
 * @param state Parser state
 * @param buffer Buffer to store the identifier
 * @param buffer_size Size of buffer
 * @return 1 on success, 0 on failure
 */
static int parse_identifier(parser_state_t* state, char* buffer, size_t buffer_size) {
  size_t len = 0;
  
  /* First character must be letter or underscore */
  int c = peek_char(state);
  if (c == EOF || (!isalpha(c) && c != '_')) {
    set_error(state, "Invalid identifier at line %d, column %d", 
             state->line, state->column);
    return 0;
  }
  
  /* Read identifier */
  while (1) {
    c = peek_char(state);
    if (c == EOF || (!isalnum(c) && c != '_')) {
      break;
    }
    
    /* Add character to buffer */
    if (len < buffer_size - 1) {
      buffer[len++] = (char)next_char(state);
    } else {
      set_error(state, "Identifier too long at line %d", state->line);
      return 0;
    }
  }
  
  /* Null-terminate the buffer */
  buffer[len] = '\0';
  
  return 1;
}

/**
 * @brief Parse a value of any type
 * @param state Parser state
 * @return Configuration value or NULL on failure
 */
static coil_config_value_t* parse_value(parser_state_t* state) {
  /* Skip whitespace and comments */
  skip_whitespace(state);
  int c = peek_char(state);
  if (c == '/') {
    next_char(state);
    skip_comment(state);
    c = peek_char(state);
  }
  
  /* Parse value based on first character */
  switch (c) {
    case '{':
      return parse_object(state);
      
    case '[':
      return parse_array(state);
      
    case '"':
      return parse_string(state);
      
    case 't':
      /* true */
      {
        char buffer[5];
        for (int i = 0; i < 4; i++) {
          buffer[i] = (char)next_char(state);
        }
        buffer[4] = '\0';
        if (strcmp(buffer, "true") == 0) {
          return parse_bool(state, 1);
        } else {
          set_error(state, "Expected 'true' at line %d, column %d", 
                   state->line, state->column);
          return NULL;
        }
      }
      
    case 'f':
      /* false */
      {
        char buffer[6];
        for (int i = 0; i < 5; i++) {
          buffer[i] = (char)next_char(state);
        }
        buffer[5] = '\0';
        if (strcmp(buffer, "false") == 0) {
          return parse_bool(state, 0);
        } else {
          set_error(state, "Expected 'false' at line %d, column %d", 
                   state->line, state->column);
          return NULL;
        }
      }
      
    case 'n':
      /* null */
      {
        char buffer[5];
        for (int i = 0; i < 4; i++) {
          buffer[i] = (char)next_char(state);
        }
        buffer[4] = '\0';
        if (strcmp(buffer, "null") == 0) {
          return parse_null(state);
        } else {
          set_error(state, "Expected 'null' at line %d, column %d", 
                   state->line, state->column);
          return NULL;
        }
      }
      
    case '-':
    case '+':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return parse_number(state);
      
    default:
      set_error(state, "Unexpected character '%c' at line %d, column %d", 
               c, state->line, state->column);
      return NULL;
  }
}

/**
 * @brief Parse a configuration from a file
 * @param file File to parse
 * @return Configuration or NULL on failure
 */
static coil_config_t* parse_config_file(FILE* file) {
  if (!file) {
    return NULL;
  }
  
  /* Initialize parser state */
  parser_state_t state = {
    .file = file,
    .text = NULL,
    .text_len = 0,
    .pos = 0,
    .line = 1,
    .column = 0,
    .error_message = {0},
    .has_error = 0
  };
  
  /* Create configuration */
  coil_config_t* config = (coil_config_t*)coil_malloc(sizeof(coil_config_t));
  if (!config) {
    set_error(&state, "Failed to allocate configuration");
    return NULL;
  }
  
  /* Parse root object */
  config->root = parse_object(&state);
  if (!config->root) {
    /* Error already set */
    coil_free(config, sizeof(coil_config_t));
    return NULL;
  }
  
  /* Skip trailing whitespace and comments */
  skip_whitespace(&state);
  int c = peek_char(&state);
  if (c == '/') {
    next_char(&state);
    skip_comment(&state);
    c = peek_char(&state);
  }
  
  /* Check for trailing content */
  if (c != EOF) {
    set_error(&state, "Unexpected content after root object at line %d, column %d", 
             state.line, state.column);
    coil_config_destroy(config);
    return NULL;
  }
  
  return config;
}

/**
 * @brief Parse a configuration from a string
 * @param text Text to parse
 * @return Configuration or NULL on failure
 */
static coil_config_t* parse_config_string(const char* text) {
  if (!text) {
    return NULL;
  }
  
  /* Initialize parser state */
  parser_state_t state = {
    .file = NULL,
    .text = text,
    .text_len = strlen(text),
    .pos = 0,
    .line = 1,
    .column = 0,
    .error_message = {0},
    .has_error = 0
  };
  
  /* Create configuration */
  coil_config_t* config = (coil_config_t*)coil_malloc(sizeof(coil_config_t));
  if (!config) {
    set_error(&state, "Failed to allocate configuration");
    return NULL;
  }
  
  /* Parse root object */
  config->root = parse_object(&state);
  if (!config->root) {
    /* Error already set */
    coil_free(config, sizeof(coil_config_t));
    return NULL;
  }
  
  /* Skip trailing whitespace and comments */
  skip_whitespace(&state);
  int c = peek_char(&state);
  if (c == '/') {
    next_char(&state);
    skip_comment(&state);
    c = peek_char(&state);
  }
  
  /* Check for trailing content */
  if (c != EOF) {
    set_error(&state, "Unexpected content after root object at line %d, column %d", 
             state.line, state.column);
    coil_config_destroy(config);
    return NULL;
  }
  
  return config;
}

/**
 * @brief Create a new configuration object
 * @return New configuration object or NULL on failure
 */
coil_config_t* coil_config_create(void) {
  /* Allocate configuration */
  coil_config_t* config = (coil_config_t*)coil_malloc(sizeof(coil_config_t));
  if (!config) {
    return NULL;
  }
  
  /* Create root object */
  config->root = create_value(COIL_CONFIG_TYPE_OBJECT);
  if (!config->root) {
    coil_free(config, sizeof(coil_config_t));
    return NULL;
  }
  
  return config;
}

/**
 * @brief Destroy a configuration object
 * @param config Configuration object to destroy
 */
void coil_config_destroy(coil_config_t* config) {
  if (!config) {
    return;
  }
  
  /* Free root value recursively */
  if (config->root) {
    free_config_value(config->root);
  }
  
  /* Free config structure */
  coil_free(config, sizeof(coil_config_t));
}

/**
 * @brief Load configuration from a file
 * @param filename Path to the configuration file
 * @return Loaded configuration or NULL on failure
 */
coil_config_t* coil_config_load_file(const char* filename) {
  if (!filename) {
    return NULL;
  }
  
  /* Open file */
  FILE* file = fopen(filename, "r");
  if (!file) {
    coil_log_error("Failed to open configuration file: %s", filename);
    return NULL;
  }
  
  /* Parse configuration */
  coil_config_t* config = parse_config_file(file);
  
  /* Close file */
  fclose(file);
  
  return config;
}

/**
 * @brief Save configuration to a file
 * @param config Configuration to save
 * @param filename Path to the output file
 * @return 0 on success, non-zero on failure
 */
int coil_config_save_file(const coil_config_t* config, const char* filename) {
  if (!config || !filename) {
    return -1;
  }
  
  /* Open file */
  FILE* file = fopen(filename, "w");
  if (!file) {
    coil_log_error("Failed to open configuration file for writing: %s", filename);
    return -1;
  }
  
  /* TODO: Implement saving configuration to file */
  
  /* Close file */
  fclose(file);
  
  return 0;
}

/**
 * @brief Parse configuration from a string
 * @param text Configuration text
 * @return Parsed configuration or NULL on failure
 */
coil_config_t* coil_config_parse_string(const char* text) {
  if (!text) {
    return NULL;
  }
  
  return parse_config_string(text);
}