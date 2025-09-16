// dez_vm_types.h
#ifndef DEZ_VM_TYPES_H
#define DEZ_VM_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Memory constants for 32-bit 16kB VM
#define MEMORY_SIZE_BYTES 16384
#define MEMORY_SIZE_WORDS 4096
#define MAX_MEMORY_ADDRESS 0x3FFF
#define MAX_32BIT_VALUE 0xFFFFFFFF
#define MAX_PROGRAM_SIZE 1024 // Maximum number of instructions

#define MAX_LABEL_LENGTH 64
#define MAX_LINE_LENGTH 256
#define MAX_LABELS 1024
#define SYMBOL_HASH_SIZE 256 // Hash table size for faster lookups

// Register index
typedef enum {
  R0 = 0,
  R1 = 1,
  R2 = 2,
  R3 = 3,
  R4 = 4,
  R5 = 5,
  R6 = 6,
  R7 = 7,
  R8 = 8,
  R9 = 9,
  R10 = 10,
  R11 = 11,
  R12 = 12,
  R13 = 13,
  R14 = 14,
  R15 = 15
} dez_register_t;

// system call indez
typedef enum {
  SYS_PRINT = 1,
  SYS_PRINT_STR = 2,
  SYS_PRINT_CHAR = 3,
  SYS_READ = 4,
  SYS_READ_STR = 5,
  SYS_EXIT = 6,
  SYS_DEBUG = 7
} dez_syscall_t;

// Instruction types with explicit opcode values
typedef enum {
  INST_MOV = 0x10,   // move instruction
  INST_ADD = 0x04,   // add instruction
  INST_SUB = 0x05,   // subtraction instruction
  INST_MUL = 0x06,   // multiplication instruction
  INST_DIV = 0x07,   // division instruction
  INST_LOAD = 0x01,  // load immediate instruction
  INST_STORE = 0x03, // store instruction
  INST_JMP = 0x08,   // jump instruction
  INST_JZ = 0x09,    // jump if zero instruction
  INST_JNZ = 0x0A,   // jump if not zero instruction
  INST_PUSH = 0x0B,  // push to stack instruction
  INST_POP = 0x0C,   // pop from stack instruction
  INST_SYS = 0x0D,   // syscall instruction
  INST_CMP = 0x0E,   // compare instruction
  INST_CALL = 0x0F,  // call label instruction
  INST_RET = 0x11,   // return instruction
  INST_HALT = 0x00,  // halt instruction
  INST_NOP = 0x12,   // no operation
  INST_UNKNOWN = 0xFF
} dez_instruction_type_t;

// Operand types
typedef enum {
  OP_REGISTER,
  OP_IMMEDIATE,
  OP_MEMORY,
  OP_LABEL,
  OP_STRING
} dez_operand_type_t;

// Operand structure
typedef struct {
  dez_operand_type_t type;
  union {
    uint8_t reg;                  // Register number (0-15)
    uint32_t value;               // 32-bit immediate value
    uint32_t address;             // 32-bit memory address
    char label[MAX_LABEL_LENGTH]; // Label name
    char string[MAX_LINE_LENGTH]; // String literal
  };
} dez_operand_t;

// Instruction structure
typedef struct {
  dez_instruction_type_t type;
  dez_operand_t operands[3]; // Most instructions have 0-3 operands
  int num_operands;
} dez_instruction_t;

typedef enum {
  VM_STATE_RUNNING = 0,
  VM_STATE_HALTED = 1,
  VM_STATE_ERROR = 2,
  VM_STATE_DEBUG = 3
} dez_vm_state_t;

#endif // DEZ_VM_TYPES_H