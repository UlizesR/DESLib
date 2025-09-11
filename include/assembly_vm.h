#ifndef ASSEMBLY_VM_H
#define ASSEMBLY_VM_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include module headers
#include "vm_errors.h"
#include "vm_hash_table.h"
#include "vm_instruction_printer.h"
#include "vm_validation.h"

// Constants
#define NUM_REGISTERS 8
#define MEMORY_SIZE 8192
#define MAX_INSTRUCTIONS 2048
#define MAX_LABEL_LENGTH 32
#define MAX_LINE_LENGTH 128
#define MAX_LABELS 256

// Register indices
#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7

// Status flags
#define FLAG_ZERO 0x01
#define FLAG_CARRY 0x02
#define FLAG_OVERFLOW 0x04

// Instruction metadata flags
#define INST_FLAG_ARITHMETIC 0x01 // Sets status flags
#define INST_FLAG_JUMP 0x02       // Modifies program counter
#define INST_FLAG_MEMORY 0x04     // Accesses memory
#define INST_FLAG_IO 0x08         // Performs I/O operations

// Instruction types
typedef enum {
  INST_MOV,    // MOV R1, R2 or MOV R1, #123
  INST_ADD,    // ADD R1, R2, R3
  INST_SUB,    // SUB R1, R2, R3
  INST_MUL,    // MUL R1, R2, R3
  INST_DIV,    // DIV R1, R2, R3
  INST_LOAD,   // LOAD R1, [R2] or LOAD R1, [123]
  INST_STORE,  // STORE R1, [R2] or STORE R1, [123]
  INST_JMP,    // JMP label or JMP 123
  INST_JZ,     // JZ label or JZ 123
  INST_JNZ,    // JNZ label or JNZ 123
  INST_PUSH,   // PUSH R1
  INST_POP,    // POP R1
  INST_PRINT,  // PRINT R1 or PRINT #123
  INST_PRINTS, // PRINTS "string" or PRINTS R1 (address)
  INST_INPUT,  // INPUT R1
  INST_CMP,    // CMP R1, R2 or CMP R1, #123
  INST_CALL,   // CALL label or CALL 123 - call subroutine
  INST_RET,    // RET - return from subroutine
  INST_HALT,   // HALT
  INST_NOP,    // NOP
  INST_UNKNOWN
} instruction_type_t;

// Operand types
typedef enum {
  OP_REGISTER,
  OP_IMMEDIATE,
  OP_MEMORY,
  OP_LABEL,
  OP_STRING
} operand_type_t;

// Label structure
typedef struct {
  char name[MAX_LABEL_LENGTH];
  int address;
} label_t;

// Instruction metadata structure
typedef struct {
  const char *mnemonic; // Instruction name (e.g., "MOV", "ADD")
  int num_operands;     // Expected number of operands
  uint8_t flags;        // Instruction behavior flags
} instruction_metadata_t;

// Operand structure
typedef struct {
  operand_type_t type;
  union {
    int reg;                      // Register number
    int32_t value;                // Immediate value or memory address
    char label[MAX_LABEL_LENGTH]; // Label name
    char string[MAX_LINE_LENGTH]; // String literal
  };
} operand_t;

// Instruction structure
typedef struct {
  instruction_type_t type;
  operand_t operands[3]; // Most instructions have 0-3 operands
  int num_operands;
} instruction_t;

// Virtual Machine state
typedef struct {
  // Core VM state
  int32_t registers[NUM_REGISTERS];        // General purpose registers
  uint8_t memory[MEMORY_SIZE];             // RAM
  int32_t stack_pointer;                   // Stack pointer
  int32_t program_counter;                 // Program counter
  uint8_t status_flags;                    // Status register
  instruction_t program[MAX_INSTRUCTIONS]; // Loaded program
  int program_size;                        // Number of instructions
  int running;                             // VM running state
  label_t labels[MAX_LABELS];              // Label table
  int num_labels;                          // Number of labels
  int verbose;                             // Verbose output flag

  // Enhanced features
  vm_error_t last_error;                   // Last error information
  vm_hash_table_t *instruction_hash_table; // Hash table for instruction lookup
} vm_t;

// Function prototypes
vm_t *vm_create(void);
void vm_destroy(vm_t *vm);
void vm_reset(vm_t *vm);
void vm_cleanup_global_resources(void);
int vm_load_program(vm_t *vm, const char *filename);
int vm_load_program_from_string(vm_t *vm, const char *program_string);
int vm_execute_instruction(vm_t *vm, instruction_t *inst);
void vm_run(vm_t *vm);
void vm_step(vm_t *vm);
void vm_print_state(vm_t *vm);
void vm_print_registers(vm_t *vm);
void vm_print_memory(vm_t *vm, int start, int end);
void vm_print_program(vm_t *vm);
void vm_set_verbose(vm_t *vm, int verbose);

// Assembler functions
instruction_type_t parse_instruction(const char *mnemonic);
operand_t parse_operand(const char *str);
int assemble_file(const char *input_file, const char *output_file);
int parse_assembly_line(const char *line, instruction_t *inst);

// Label functions
int add_label(vm_t *vm, const char *name, int address);
int find_label(vm_t *vm, const char *name);
int resolve_labels(vm_t *vm);

// Helper functions
int32_t get_operand_value(vm_t *vm, const operand_t *op);
int32_t get_memory_address(vm_t *vm, const operand_t *op);
int validate_memory_access(int32_t address);
int validate_stack_operation(vm_t *vm, int is_push);
void set_status_flags(vm_t *vm, int32_t result);
const instruction_metadata_t *get_instruction_metadata(instruction_type_t type);

// Memory helper functions
int32_t read_memory_32(vm_t *vm, int32_t address);
void write_memory_32(vm_t *vm, int32_t address, int32_t value);

// Instruction handler functions
int handle_mov(vm_t *vm, const instruction_t *inst);
int handle_arithmetic(vm_t *vm, const instruction_t *inst,
                      int32_t (*op)(int32_t, int32_t));
int handle_load(vm_t *vm, const instruction_t *inst);
int handle_store(vm_t *vm, const instruction_t *inst);
int handle_jump(vm_t *vm, const instruction_t *inst, int condition);
int handle_push(vm_t *vm, const instruction_t *inst);
int handle_pop(vm_t *vm, const instruction_t *inst);
int handle_print(vm_t *vm, const instruction_t *inst);
int handle_prints(vm_t *vm, const instruction_t *inst);
int handle_input(vm_t *vm, const instruction_t *inst);
int handle_cmp(vm_t *vm, const instruction_t *inst);
int handle_call(vm_t *vm, const instruction_t *inst);
int handle_ret(vm_t *vm, const instruction_t *inst);

#endif // ASSEMBLY_VM_H
