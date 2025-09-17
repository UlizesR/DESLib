/**
 * @file dez_instruction_table.h
 * @brief DEZ Virtual Machine Instruction Table
 *
 * This file defines the instruction execution system, including
 * all instruction execution functions and the instruction table.
 */

#ifndef DEZ_INSTRUCTION_TABLE_H
#define DEZ_INSTRUCTION_TABLE_H

#include "dez_vm.h"
#include <stdbool.h>
#include <stdint.h>

// Instruction execution function type
typedef void (*instruction_executor_t)(dez_vm_t *vm, uint32_t instruction);

// Instruction metadata
typedef struct {
  instruction_executor_t execute;
  uint8_t pc_increment;
  bool needs_memory;
  bool needs_validation;
  const char *mnemonic;
} instruction_info_t;

// Get instruction info
const instruction_info_t *get_instruction_info(uint8_t opcode);

// Instruction execution functions
void execute_mov(dez_vm_t *vm, uint32_t instruction);
void execute_store(dez_vm_t *vm, uint32_t instruction);
void execute_add(dez_vm_t *vm, uint32_t instruction);
void execute_sub(dez_vm_t *vm, uint32_t instruction);
void execute_mul(dez_vm_t *vm, uint32_t instruction);
void execute_div(dez_vm_t *vm, uint32_t instruction);
void execute_jmp(dez_vm_t *vm, uint32_t instruction);
void execute_jz(dez_vm_t *vm, uint32_t instruction);
void execute_jnz(dez_vm_t *vm, uint32_t instruction);
void execute_jl(dez_vm_t *vm, uint32_t instruction);
void execute_jg(dez_vm_t *vm, uint32_t instruction);
void execute_jle(dez_vm_t *vm, uint32_t instruction);
void execute_jge(dez_vm_t *vm, uint32_t instruction);
void execute_cmp(dez_vm_t *vm, uint32_t instruction);
void execute_sys(dez_vm_t *vm, uint32_t instruction);
void execute_halt(dez_vm_t *vm, uint32_t instruction);
void execute_nop(dez_vm_t *vm, uint32_t instruction);
void execute_push(dez_vm_t *vm, uint32_t instruction);
void execute_pop(dez_vm_t *vm, uint32_t instruction);
void execute_call(dez_vm_t *vm, uint32_t instruction);
void execute_ret(dez_vm_t *vm, uint32_t instruction);
void execute_and(dez_vm_t *vm, uint32_t instruction);
void execute_or(dez_vm_t *vm, uint32_t instruction);
void execute_xor(dez_vm_t *vm, uint32_t instruction);
void execute_not(dez_vm_t *vm, uint32_t instruction);
void execute_shl(dez_vm_t *vm, uint32_t instruction);
void execute_shr(dez_vm_t *vm, uint32_t instruction);
void execute_inc(dez_vm_t *vm, uint32_t instruction);
void execute_dec(dez_vm_t *vm, uint32_t instruction);
void execute_unknown(dez_vm_t *vm, uint32_t instruction);

// ============================================================================
// OPTIMIZATION MACROS
// ============================================================================

// Register access macros for optimization
#define DEZ_REG_ACCESS(vm, reg) ((vm)->cpu.regs[(reg) & 0xF])
#define DEZ_PC_INCREMENT(vm, inc) ((vm)->cpu.pc += (inc))

// ============================================================================
// COMMON ERROR HANDLING MACROS
// ============================================================================

// Common error handling for invalid registers
#define DEZ_VALIDATE_REGISTER(reg, instruction_name)                           \
  do {                                                                         \
    if ((reg) > DEZ_MAX_REGISTER_INDEX) {                                      \
      printf("Error: Invalid register R%d in %s instruction\n", (reg),         \
             (instruction_name));                                              \
      vm->cpu.state = DEZ_VM_STATE_ERROR;                                      \
      return;                                                                  \
    }                                                                          \
  } while (0)

// Common error handling for memory bounds
#define DEZ_VALIDATE_MEMORY_ADDRESS(addr, instruction_name)                    \
  do {                                                                         \
    if ((addr) >= DEZ_MEMORY_SIZE_WORDS) {                                     \
      printf("Error: Memory address 0x%04X out of bounds in %s instruction\n", \
             (addr), (instruction_name));                                      \
      vm->cpu.state = DEZ_VM_STATE_ERROR;                                      \
      return;                                                                  \
    }                                                                          \
  } while (0)

// Common error handling for stack bounds
#define DEZ_VALIDATE_STACK_OVERFLOW(instruction_name)                          \
  do {                                                                         \
    if (vm->cpu.sp < DEZ_STACK_START) {                                        \
      printf("Error: Stack overflow in %s instruction\n", (instruction_name)); \
      vm->cpu.state = DEZ_VM_STATE_ERROR;                                      \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define DEZ_VALIDATE_STACK_UNDERFLOW(instruction_name)                         \
  do {                                                                         \
    if (vm->cpu.sp >= DEZ_STACK_END) {                                         \
      printf("Error: Stack underflow in %s instruction\n",                     \
             (instruction_name));                                              \
      vm->cpu.state = DEZ_VM_STATE_ERROR;                                      \
      return;                                                                  \
    }                                                                          \
  } while (0)

#endif // DEZ_INSTRUCTION_TABLE_H
