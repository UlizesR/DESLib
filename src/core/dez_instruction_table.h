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
void execute_cmp(dez_vm_t *vm, uint32_t instruction);
void execute_sys(dez_vm_t *vm, uint32_t instruction);
void execute_halt(dez_vm_t *vm, uint32_t instruction);
void execute_nop(dez_vm_t *vm, uint32_t instruction);
void execute_unknown(dez_vm_t *vm, uint32_t instruction);

// Register access macros for optimization
#define REG_ACCESS(vm, reg) ((vm)->cpu.regs[(reg) & 0xF])
#define PC_INCREMENT(vm, inc) ((vm)->cpu.pc += (inc))

// Instruction decoding macros for lazy evaluation
#define DECODE_REG1(inst) ((inst) >> 20) & 0xF
#define DECODE_REG2(inst) ((inst) >> 16) & 0xF
#define DECODE_REG3(inst) ((inst) >> 12) & 0xF
#define DECODE_IMMEDIATE(inst) ((inst) & 0x0FFF)
#define DECODE_OPCODE(inst) ((inst) >> 24) & 0xFF

#endif // DEZ_INSTRUCTION_TABLE_H
