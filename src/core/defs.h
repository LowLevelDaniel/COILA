#ifndef COIL_CORE_DEFS_H
#define COIL_CORE_DEFS_H

#include <cstdint>

namespace coil {

// Instruction categories (bits 7-5 of opcode)
enum InstructionCategory : uint8_t {
  CAT_CF    = 0x00,  // 000xxxxx - Control Flow
  CAT_MEM   = 0x20,  // 001xxxxx - Memory Operations
  CAT_MATH  = 0x40,  // 010xxxxx - Arithmetic Operations
  CAT_BIT   = 0x60,  // 011xxxxx - Bit Manipulation
  CAT_VEC   = 0x80,  // 100xxxxx - Vector Operations
  CAT_ATM   = 0xA0,  // 101xxxxx - Atomic Operations
  CAT_VAR   = 0xC0,  // 110xxxxx - Variable Management
  CAT_FRAME = 0xE0   // 111xxxxx - Frame Management
};

// Control Flow operations (bits 4-0)
enum ControlFlowOp : uint8_t {
  CF_BR     = 0x00,  // Unconditional branch
  CF_BRC    = 0x01,  // Conditional branch
  CF_CALL   = 0x02,  // Function call
  CF_RET    = 0x03,  // Return from function
  CF_INT    = 0x04,  // Software interrupt
  CF_IRET   = 0x05,  // Return from interrupt
  CF_HLT    = 0x06,  // Halt execution
  CF_SYSC   = 0x07,  // System call
  CF_TRAP   = 0x08,  // Trap to exception handler
  CF_WFE    = 0x09,  // Wait for event
  CF_SEV    = 0x0A,  // Send event
  CF_FENCE  = 0x0B,  // Memory fence
  CF_YIELD  = 0x0C,  // Yield execution
  CF_SWITCH = 0x0D,  // Switch target architecture
  CF_NOP    = 0x0E   // No operation
};

// Memory operations (bits 4-0)
enum MemoryOp : uint8_t {
  MEM_MOV      = 0x00,  // Move data
  MEM_PUSH     = 0x01,  // Push to stack
  MEM_POP      = 0x02,  // Pop from stack
  MEM_LOAD     = 0x03,  // Load from memory
  MEM_STORE    = 0x04,  // Store to memory
  MEM_PREFETCH = 0x05,  // Prefetch memory
  MEM_EXCHANGE = 0x06,  // Exchange values
  MEM_COMPARE  = 0x07,  // Compare and update flags
  MEM_TEST     = 0x08,  // Test and update flags
  MEM_FILL     = 0x09,  // Fill memory block
  MEM_COPY     = 0x0A,  // Copy memory block
  MEM_ZERO     = 0x0B,  // Zero memory block
  MEM_PUSH_STATE = 0x0C, // Save register state
  MEM_POP_STATE  = 0x0D, // Restore register state
  MEM_OUT      = 0x0E,  // Output to I/O port
  MEM_IN       = 0x0F   // Input from I/O port
};

// Arithmetic operations (bits 4-0)
enum ArithmeticOp : uint8_t {
  MATH_ADD    = 0x00,  // Addition
  MATH_SUB    = 0x01,  // Subtraction
  MATH_MUL    = 0x02,  // Multiplication
  MATH_DIV    = 0x03,  // Division
  MATH_MOD    = 0x04,  // Modulo
  MATH_NEG    = 0x05,  // Negation
  MATH_INC    = 0x06,  // Increment
  MATH_DEC    = 0x07,  // Decrement
  MATH_ABS    = 0x08,  // Absolute value
  MATH_SQRT   = 0x09,  // Square root
  MATH_MIN    = 0x0A,  // Minimum
  MATH_MAX    = 0x0B,  // Maximum
  MATH_FMA    = 0x0C,  // Fused multiply-add
  MATH_ROUND  = 0x0D,  // Round
  MATH_FLOOR  = 0x0E,  // Floor
  MATH_CEIL   = 0x0F,  // Ceiling
  MATH_TRUNC  = 0x10   // Truncate
};

// Bit operations (bits 4-0)
enum BitOp : uint8_t {
  BIT_AND    = 0x00,  // Bitwise AND
  BIT_OR     = 0x01,  // Bitwise OR
  BIT_XOR    = 0x02,  // Bitwise XOR
  BIT_NOT    = 0x03,  // Bitwise NOT
  BIT_ANDN   = 0x04,  // Bitwise AND-NOT
  BIT_ORN    = 0x05,  // Bitwise OR-NOT
  BIT_XNOR   = 0x06,  // Bitwise XNOR
  BIT_SHL    = 0x07,  // Shift left logical
  BIT_SHR    = 0x08,  // Shift right logical
  BIT_SAR    = 0x09,  // Shift arithmetic right
  BIT_ROL    = 0x0A,  // Rotate left
  BIT_ROR    = 0x0B,  // Rotate right
  BIT_RCL    = 0x0C,  // Rotate left through carry
  BIT_RCR    = 0x0D,  // Rotate right through carry
  BIT_BSWAP  = 0x0E,  // Byte swap
  BIT_BITREV = 0x0F,  // Bit reverse
  BIT_CLZ    = 0x10,  // Count leading zeros
  BIT_CTZ    = 0x11,  // Count trailing zeros
  BIT_POPCNT = 0x12,  // Population count
  BIT_PARITY = 0x13,  // Parity
  BIT_EXTRACT = 0x14, // Bit field extract
  BIT_INSERT = 0x15,  // Bit field insert
  BIT_SET    = 0x16,  // Set bit
  BIT_CLR    = 0x17,  // Clear bit
  BIT_TST    = 0x18,  // Test bit
  BIT_TGL    = 0x19,  // Toggle bit
  BIT_CMP    = 0x1A   // Compare values and set flags
};

// Variable operations (bits 4-0)
enum VariableOp : uint8_t {
  VAR_DECL   = 0x00,  // Variable declaration
  VAR_PMT    = 0x01,  // Make variable permanent (extend lifetime)
  VAR_DMT    = 0x02,  // Destroy variable (end lifetime)
  VAR_DLT    = 0x03,  // Delete variable (release storage)
  VAR_ALIAS  = 0x04   // Create variable alias
};

// Frame operations (bits 4-0)
enum FrameOp : uint8_t {
  FRAME_ENTER = 0x00,  // Enter function frame
  FRAME_LEAVE = 0x01,  // Leave function frame
  FRAME_SAVE  = 0x02,  // Save registers
  FRAME_REST  = 0x03   // Restore registers
};

// Operand classes (bits 7-6 of type byte)
enum OperandClass : uint8_t {
  OPERAND_REGISTER  = 0x00,  // 00xxxxxx - Register operand
  OPERAND_IMMEDIATE = 0x40,  // 01xxxxxx - Immediate value
  OPERAND_MEMORY    = 0x80,  // 10xxxxxx - Memory operand
  OPERAND_VARIABLE  = 0xC0   // 11xxxxxx - Variable reference
};

// Register types (bits 5-0 of type byte for register operands)
enum RegisterType : uint8_t {
  REG_GP      = 0x00,  // General purpose register
  REG_FP      = 0x01,  // Floating point register
  REG_VEC     = 0x02,  // Vector register
  REG_SPECIAL = 0x03   // Special register
};

// Immediate operand types (bits 5-0 of type byte for immediate operands)
enum ImmediateType : uint8_t {
  IMM_INT8    = 0x00,  // 8-bit integer
  IMM_INT16   = 0x01,  // 16-bit integer
  IMM_INT32   = 0x02,  // 32-bit integer
  IMM_INT64   = 0x03,  // 64-bit integer
  IMM_FLOAT32 = 0x04,  // 32-bit float
  IMM_FLOAT64 = 0x05,  // 64-bit float
  IMM_SYMBOL  = 0x06   // Symbol reference
};

// Memory operand types (bits 5-0 of type byte for memory operands)
enum MemoryType : uint8_t {
  MEM_DIRECT     = 0x00,  // Direct memory access [addr]
  MEM_REG        = 0x01,  // Register indirect [reg]
  MEM_REG_DISP   = 0x02,  // Register + displacement [reg+disp]
  MEM_REG_REG    = 0x03,  // Register + register [reg1+reg2]
  MEM_REG_REG_SCALE = 0x04, // Register + scaled register [reg1+reg2*scale]
  MEM_REG_PRE_INC  = 0x05, // Pre-increment [++reg]
  MEM_REG_PRE_DEC  = 0x06, // Pre-decrement [--reg]
  MEM_REG_POST_INC = 0x07, // Post-increment [reg++]
  MEM_REG_POST_DEC = 0x08  // Post-decrement [reg--]
};

// Variable operand types (bits 5-0 of type byte for variable operands)
enum VariableRefType : uint8_t {
  VAR_DIRECT    = 0x00,  // Direct variable reference
  VAR_ADDR      = 0x01,  // Variable address
  VAR_ELEM      = 0x02,  // Array element
  VAR_FIELD     = 0x03   // Structure field
};

// Predefined virtual register IDs
enum VirtualRegisterID : uint8_t {
  // General purpose registers
  REG_R0  = 0x00,
  REG_R1  = 0x01,
  REG_R2  = 0x02,
  REG_R3  = 0x03,
  REG_R4  = 0x04,
  REG_R5  = 0x05,
  REG_R6  = 0x06,
  REG_R7  = 0x07,
  REG_R8  = 0x08,
  REG_R9  = 0x09,
  REG_R10 = 0x0A,
  REG_R11 = 0x0B,
  REG_R12 = 0x0C,
  REG_R13 = 0x0D,
  REG_R14 = 0x0E,
  REG_R15 = 0x0F,
  
  // Floating point registers
  REG_F0  = 0x10,
  REG_F1  = 0x11,
  REG_F2  = 0x12,
  REG_F3  = 0x13,
  REG_F4  = 0x14,
  REG_F5  = 0x15,
  REG_F6  = 0x16,
  REG_F7  = 0x17,
  REG_F8  = 0x18,
  REG_F9  = 0x19,
  REG_F10 = 0x1A,
  REG_F11 = 0x1B,
  REG_F12 = 0x1C,
  REG_F13 = 0x1D,
  REG_F14 = 0x1E,
  REG_F15 = 0x1F,
  
  // Vector registers
  REG_V0  = 0x20,
  REG_V1  = 0x21,
  REG_V2  = 0x22,
  REG_V3  = 0x23,
  REG_V4  = 0x24,
  REG_V5  = 0x25,
  REG_V6  = 0x26,
  REG_V7  = 0x27,
  REG_V8  = 0x28,
  REG_V9  = 0x29,
  REG_V10 = 0x2A,
  REG_V11 = 0x2B,
  REG_V12 = 0x2C,
  REG_V13 = 0x2D,
  REG_V14 = 0x2E,
  REG_V15 = 0x2F,
  
  // Special registers
  REG_PC    = 0x30,  // Program Counter
  REG_SP    = 0x31,  // Stack Pointer
  REG_FP    = 0x32,  // Frame Pointer
  REG_FLAGS = 0x33,  // Status Flags
  REG_LR    = 0x34   // Link Register
};

// Condition codes for conditional branches
enum ConditionCode : uint8_t {
  COND_EQ = 0x00,  // Equal
  COND_NE = 0x01,  // Not equal
  COND_LT = 0x02,  // Less than
  COND_LE = 0x03,  // Less than or equal
  COND_GT = 0x04,  // Greater than
  COND_GE = 0x05,  // Greater than or equal
  COND_Z  = 0x06,  // Zero
  COND_NZ = 0x07,  // Not zero
  COND_CS = 0x08,  // Carry set
  COND_CC = 0x09,  // Carry clear
  COND_VS = 0x0A,  // Overflow set
  COND_VC = 0x0B,  // Overflow clear
  COND_NS = 0x0C,  // Negative set
  COND_NC = 0x0D,  // Negative clear
  COND_PS = 0x0E,  // Parity set
  COND_PC = 0x0F   // Parity clear
};

// Basic types
enum BasicType : uint16_t {
  TYPE_VOID   = 0x0000,
  TYPE_INT8   = 0x0001,
  TYPE_INT16  = 0x0002,
  TYPE_INT32  = 0x0003,
  TYPE_INT64  = 0x0004,
  TYPE_INT128 = 0x0005,
  TYPE_UINT8  = 0x0006,
  TYPE_UINT16 = 0x0007,
  TYPE_UINT32 = 0x0008,
  TYPE_UINT64 = 0x0009,
  TYPE_UINT128 = 0x000A,
  TYPE_FP16   = 0x000B,
  TYPE_FP32   = 0x000C,
  TYPE_FP64   = 0x000D,
  TYPE_FP80   = 0x000E,
  TYPE_FP128  = 0x000F,
  TYPE_PTR    = 0x0010,  // Pointer type (base type follows)
  TYPE_VEC128 = 0x0020,  // 128-bit vector (element type follows)
  TYPE_VEC256 = 0x0021,  // 256-bit vector (element type follows)
  TYPE_VEC512 = 0x0022   // 512-bit vector (element type follows)
};

// Variable storage classes
enum StorageClass : uint8_t {
  STORAGE_AUTO = 0,     // Automatic storage (register or stack)
  STORAGE_REGISTER = 1, // Register only (no spilling)
  STORAGE_STACK = 2,    // Stack only
  STORAGE_STATIC = 3,   // Static storage (data section)
  STORAGE_THREAD = 4,   // Thread-local storage
  STORAGE_GLOBAL = 5    // Global storage
};

// Section types
enum SectionType : uint32_t {
  SECTION_NULL = 0,        // Null section
  SECTION_CODE = 1,        // Executable code
  SECTION_DATA = 2,        // Initialized data
  SECTION_BSS = 3,         // Uninitialized data
  SECTION_READONLY = 4,    // Read-only data
  SECTION_CONFIG = 5,      // Configuration data
  SECTION_SYMBOL = 6,      // Symbol table
  SECTION_STRING = 7,      // String table
  SECTION_RELOC = 8,       // Relocation information
  SECTION_DEBUG = 9,       // Debug information
  SECTION_TARGET = 10,     // Target-specific data
  SECTION_ABI = 11,        // ABI-specific data
  SECTION_COMMENT = 12,    // Comment section
  SECTION_NOTE = 13,       // Note section
  SECTION_VARIABLE = 14,   // Variable information
  SECTION_TYPE = 15        // Type information
};

// Section flags
enum SectionFlags : uint32_t {
  SECTION_FLAG_NONE = 0x00000000,  // No flags
  SECTION_FLAG_WRITE = 0x00000001, // Writable section
  SECTION_FLAG_EXEC = 0x00000002,  // Executable section
  SECTION_FLAG_ALLOC = 0x00000004, // Occupies memory during execution
  SECTION_FLAG_LOAD = 0x00000008,  // Section should be loaded
  SECTION_FLAG_TLS = 0x00000010,   // Thread-local storage
  SECTION_FLAG_MERGE = 0x00000020, // Can be merged
  SECTION_FLAG_STRINGS = 0x00000040, // Contains null-terminated strings
  SECTION_FLAG_GROUP = 0x00000080, // Section is a group
  SECTION_FLAG_COMPRESSED = 0x00000100, // Compressed section
  SECTION_FLAG_ENCRYPTED = 0x00000200  // Encrypted section
};

// Symbol types
enum SymbolType : uint16_t {
  SYMBOL_NONE = 0,         // Unspecified symbol type
  SYMBOL_FUNCTION = 1,     // Function symbol
  SYMBOL_DATA = 2,         // Data symbol
  SYMBOL_SECTION = 3,      // Section symbol
  SYMBOL_FILE = 4,         // File symbol
  SYMBOL_COMMON = 5,       // Common symbol
  SYMBOL_TLS = 6,          // Thread-local storage symbol
  SYMBOL_VARIABLE = 7,     // Variable symbol
  SYMBOL_TARGET = 8        // Target-specific symbol
};

// Symbol flags
enum SymbolFlags : uint16_t {
  SYMBOL_FLAG_NONE = 0x0000,     // No flags
  SYMBOL_FLAG_GLOBAL = 0x0001,   // Global symbol
  SYMBOL_FLAG_LOCAL = 0x0002,    // Local symbol
  SYMBOL_FLAG_WEAK = 0x0004,     // Weak symbol
  SYMBOL_FLAG_HIDDEN = 0x0008,   // Hidden symbol
  SYMBOL_FLAG_PROTECTED = 0x0010, // Protected symbol
  SYMBOL_FLAG_UNDEFINED = 0x0020, // Undefined symbol
  SYMBOL_FLAG_EXPORTED = 0x0040, // Exported symbol
  SYMBOL_FLAG_ENTRY = 0x0080,    // Entry point symbol
  SYMBOL_FLAG_CONSTRUCTOR = 0x0100, // Constructor function
  SYMBOL_FLAG_DESTRUCTOR = 0x0200  // Destructor function
};

// Architecture types
enum ArchType : uint8_t {
  ARCH_X86 = 0,            // x86 (32-bit)
  ARCH_X86_64 = 1,         // x86-64 (64-bit)
  ARCH_ARM = 2,            // ARM (32-bit)
  ARCH_ARM64 = 3,          // ARM64 (64-bit)
  ARCH_RISCV32 = 4,        // RISC-V (32-bit)
  ARCH_RISCV64 = 5,        // RISC-V (64-bit)
  ARCH_POWERPC = 6,        // PowerPC (32-bit)
  ARCH_POWERPC64 = 7,      // PowerPC (64-bit)
  ARCH_MIPS = 8,           // MIPS (32-bit)
  ARCH_MIPS64 = 9,         // MIPS (64-bit)
  ARCH_SPARC = 10,         // SPARC (32-bit)
  ARCH_SPARC64 = 11,       // SPARC (64-bit)
  ARCH_WASM = 12           // WebAssembly
};

} // namespace coil

#endif // COIL_CORE_DEFS_H