/**
 * @file dez_vm_types.h
 * @brief DEZ Virtual Machine Type Definitions
 *
 * This file contains all type definitions, constants, and enums
 * used throughout the DEZ virtual machine system.
 */

#ifndef DEZ_VM_TYPES_H
#define DEZ_VM_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// MEMORY CONSTANTS
// ============================================================================

// Memory size constants for 32-bit 16kB VM
#define DEZ_MEMORY_SIZE_BYTES 16384
#define DEZ_MEMORY_SIZE_WORDS 4096
#define DEZ_MAX_MEMORY_ADDRESS 0x3FFF
#define DEZ_MAX_32BIT_VALUE 0xFFFFFFFF
#define DEZ_MAX_PROGRAM_SIZE 1024 // Maximum number of instructions

// ============================================================================
// ASSEMBLER CONSTANTS
// ============================================================================

#define DEZ_MAX_LABEL_LENGTH 64
#define DEZ_MAX_LINE_LENGTH 256
#define DEZ_MAX_LABELS 1024
#define DEZ_SYMBOL_HASH_SIZE 256 // Hash table size for faster lookups

// ============================================================================
// INSTRUCTION CONSTANTS
// ============================================================================

#define DEZ_MAX_REGISTER_INDEX 15
#define DEZ_IMMEDIATE_MASK 0x0FFF
#define DEZ_EXT_IMMEDIATE_MASK 0x07FF
#define DEZ_IMMEDIATE_MODE_FLAG (1 << 11)

// ============================================================================
// INSTRUCTION DECODING MACROS
// ============================================================================

#define DEZ_DECODE_OPCODE(inst) ((inst) >> 24) & 0xFF
#define DEZ_DECODE_REG1(inst) ((inst) >> 20) & 0xF
#define DEZ_DECODE_REG2(inst) ((inst) >> 16) & 0xF
#define DEZ_DECODE_REG3(inst) ((inst) >> 12) & 0xF
#define DEZ_DECODE_IMMEDIATE(inst) ((inst) & DEZ_IMMEDIATE_MASK)
#define DEZ_DECODE_EXT_IMMEDIATE(inst) ((inst) & DEZ_EXT_IMMEDIATE_MASK)
#define DEZ_IS_IMMEDIATE_MODE(inst) (((inst) & DEZ_IMMEDIATE_MODE_FLAG) != 0)

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

// Register index enumeration
typedef enum {
  DEZ_R0 = 0,
  DEZ_R1 = 1,
  DEZ_R2 = 2,
  DEZ_R3 = 3,
  DEZ_R4 = 4,
  DEZ_R5 = 5,
  DEZ_R6 = 6,
  DEZ_R7 = 7,
  DEZ_R8 = 8,
  DEZ_R9 = 9,
  DEZ_R10 = 10,
  DEZ_R11 = 11,
  DEZ_R12 = 12,
  DEZ_R13 = 13,
  DEZ_R14 = 14,
  DEZ_R15 = 15
} dez_register_t;

// System call enumeration
typedef enum {
  DEZ_SYS_PRINT = 1,
  DEZ_SYS_PRINT_STR = 2,
  DEZ_SYS_PRINT_CHAR = 3,
  DEZ_SYS_READ = 4,
  DEZ_SYS_READ_STR = 5,
  DEZ_SYS_EXIT = 6,
  DEZ_SYS_DEBUG = 7
} dez_syscall_t;

// Instruction types with explicit opcode values
typedef enum {
  DEZ_INST_MOV = 0x10,   // move instruction
  DEZ_INST_ADD = 0x04,   // add instruction
  DEZ_INST_SUB = 0x05,   // subtraction instruction
  DEZ_INST_MUL = 0x06,   // multiplication instruction
  DEZ_INST_DIV = 0x07,   // division instruction
  DEZ_INST_LOAD = 0x01,  // load from memory instruction
  DEZ_INST_STORE = 0x03, // store instruction
  DEZ_INST_JMP = 0x08,   // jump instruction
  DEZ_INST_JZ = 0x09,    // jump if zero instruction
  DEZ_INST_JNZ = 0x0A,   // jump if not zero instruction
  DEZ_INST_JL = 0x19,    // jump if less instruction
  DEZ_INST_JG = 0x1A,    // jump if greater instruction
  DEZ_INST_JLE = 0x1B,   // jump if less or equal instruction
  DEZ_INST_JGE = 0x1C,   // jump if greater or equal instruction
  DEZ_INST_PUSH = 0x0B,  // push to stack instruction
  DEZ_INST_POP = 0x0C,   // pop from stack instruction
  DEZ_INST_SYS = 0x0D,   // syscall instruction
  DEZ_INST_CMP = 0x0E,   // compare instruction
  DEZ_INST_CALL = 0x0F,  // call label instruction
  DEZ_INST_RET = 0x11,   // return instruction
  DEZ_INST_AND = 0x13,   // bitwise AND instruction
  DEZ_INST_OR = 0x14,    // bitwise OR instruction
  DEZ_INST_XOR = 0x15,   // bitwise XOR instruction
  DEZ_INST_NOT = 0x16,   // bitwise NOT instruction
  DEZ_INST_SHL = 0x17,   // shift left instruction
  DEZ_INST_SHR = 0x18,   // shift right instruction
  DEZ_INST_INC = 0x1D,   // increment instruction
  DEZ_INST_DEC = 0x1E,   // decrement instruction
  DEZ_INST_HALT = 0x00,  // halt instruction
  DEZ_INST_NOP = 0x12,   // no operation
  DEZ_INST_UNKNOWN = 0xFF
} dez_instruction_type_t;

// Operand types
typedef enum {
  DEZ_OP_REGISTER,
  DEZ_OP_IMMEDIATE,
  DEZ_OP_MEMORY,
  DEZ_OP_LABEL,
  DEZ_OP_STRING
} dez_operand_type_t;

// Operand structure
typedef struct {
  dez_operand_type_t type;
  union {
    uint8_t reg;                      // Register number (0-15)
    uint32_t value;                   // 32-bit immediate value
    uint32_t address;                 // 32-bit memory address
    char label[DEZ_MAX_LABEL_LENGTH]; // Label name
    char string[DEZ_MAX_LINE_LENGTH]; // String literal
  };
} dez_operand_t;

// Instruction structure
typedef struct {
  dez_instruction_type_t type;
  dez_operand_t operands[3]; // Most instructions have 0-3 operands
  int num_operands;
} dez_instruction_t;

// VM state enumeration
typedef enum {
  DEZ_VM_STATE_RUNNING = 0,
  DEZ_VM_STATE_HALTED = 1,
  DEZ_VM_STATE_ERROR = 2,
  DEZ_VM_STATE_DEBUG = 3
} dez_vm_state_t;

// CPU flags for conditional jumps
typedef enum {
  DEZ_FLAG_ZERO = 0x01,    // Zero flag (set when result is zero)
  DEZ_FLAG_LESS = 0x02,    // Less flag (set when first operand < second)
  DEZ_FLAG_GREATER = 0x04, // Greater flag (set when first operand > second)
  DEZ_FLAG_EQUAL = 0x08    // Equal flag (set when operands are equal)
} dez_cpu_flags_t;

#endif // DEZ_VM_TYPES_H