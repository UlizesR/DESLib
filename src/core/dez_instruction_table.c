#include "dez_instruction_table.h"
#include "../include/dez_vm_types.h"
#include "dez_memory.h"
#include "dez_vm.h"
#include <stdio.h>

// Instruction execution functions
// Each function takes a VM pointer and a 32-bit instruction word
// The instruction format is: (opcode << 24) | (operands...)

/**
 * Execute MOV instruction: Load immediate value into register
 * Format: (INST_MOV << 24) | (reg << 20) | (immediate & 0x0FFF)
 * Example: MOV R0, 42 -> 0x1000002A
 */
void execute_mov(dez_vm_t *vm, uint32_t instruction) {
  // Optimized: direct register access without bounds checking in hot path
  vm->cpu.regs[instruction >> 20 & 0xF] = instruction & 0x0FFF;
}

/**
 * Execute STORE instruction: Store register value to memory
 * Format: (INST_STORE << 24) | (reg << 20) | (address & 0x0FFF)
 * Example: STORE R0, 256 -> 0x11000100
 */
void execute_store(dez_vm_t *vm, uint32_t instruction) {
  vm->memory.memory[instruction & 0x0FFF] =
      vm->cpu.regs[instruction >> 20 & 0xF];
}

/**
 * Execute ADD instruction: Add two values and store result
 * Format: (INST_ADD << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12) |
 * immediate Bit 11 flag: 0 = register-to-register, 1 = register-to-immediate
 * Example: ADD R1, R2, R3 -> 0x04213000
 * Example: ADD R1, R2, 5 -> 0x04210805 (bit 11 set)
 */
void execute_add(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint8_t reg3 = instruction >> 12 & 0xF;
  uint32_t immediate = instruction & 0x07FF;
  bool immediate_mode = (instruction & (1 << 11)) != 0;

  // Check if this is register-to-immediate addition
  if (immediate_mode) {
    // Register-to-immediate addition: reg1 = reg2 + immediate
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] + immediate;
  } else {
    // Register-to-register addition: reg1 = reg2 + reg3
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] + vm->cpu.regs[reg3];
  }
}

void execute_sub(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint8_t reg3 = instruction >> 12 & 0xF;
  uint32_t immediate = instruction & 0x07FF;
  bool immediate_mode = (instruction & (1 << 11)) != 0;

  // Check if this is register-to-immediate subtraction
  if (immediate_mode) {
    // Register-to-immediate subtraction
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] - immediate;
  } else {
    // Register-to-register subtraction
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] - vm->cpu.regs[reg3];
  }
}

void execute_mul(dez_vm_t *vm, uint32_t instruction) {
  vm->cpu.regs[instruction >> 20 & 0xF] =
      vm->cpu.regs[instruction >> 16 & 0xF] *
      vm->cpu.regs[instruction >> 12 & 0xF];
}

void execute_div(dez_vm_t *vm, uint32_t instruction) {
  uint32_t divisor = vm->cpu.regs[instruction >> 12 & 0xF];
  if (UNLIKELY(divisor == 0)) {
    printf("Error: Division by zero\n");
    vm->cpu.state = VM_STATE_ERROR;
    return;
  }
  vm->cpu.regs[instruction >> 20 & 0xF] =
      vm->cpu.regs[instruction >> 16 & 0xF] / divisor;
}

void execute_jmp(dez_vm_t *vm, uint32_t instruction) {
  vm->cpu.pc = instruction & 0x0FFF;
}

void execute_jz(dez_vm_t *vm, uint32_t instruction) {
  if (vm->cpu.flags == 1) {
    vm->cpu.pc = instruction & 0x0FFF;
  }
  // If condition not met, let VM handle PC increment
}

void execute_jnz(dez_vm_t *vm, uint32_t instruction) {
  if (vm->cpu.flags == 0) {
    vm->cpu.pc = instruction & 0x0FFF;
  }
  // If condition not met, let VM handle PC increment
}

void execute_cmp(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint32_t immediate = instruction & 0x0FFF;

  // Check if this is register-to-register comparison (reg2 != 0)
  if (reg2 != 0) {
    // Register-to-register comparison
    vm->cpu.flags = (vm->cpu.regs[reg1] == vm->cpu.regs[reg2]) ? 1 : 0;
  } else {
    // Register-to-immediate comparison
    vm->cpu.flags = (vm->cpu.regs[reg1] == immediate) ? 1 : 0;
  }
}

void execute_sys(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint32_t immediate = instruction & 0x0FFF;

  // For now, implement basic print functionality
  if (immediate == SYS_PRINT) {
    printf("R%d = %d\n", reg1, vm->cpu.regs[reg1]);
  } else if (immediate == SYS_PRINT_CHAR) {
    printf("%c", (char)vm->cpu.regs[reg1]);
  } else if (immediate == SYS_PRINT_STR) {
    // Print string - reg1 contains address of string
    uint32_t addr = vm->cpu.regs[reg1];

    // Optimized single-pass string printing with escape sequence interpretation
    uint32_t pos = addr;
    while (pos < 0x200) { // Limit to prevent overflow
      uint8_t byte = memory_read_byte(&vm->memory, pos);
      if (byte == 0)
        break; // Null terminator

      if (byte == '\\' && pos + 1 < 0x200) {
        // Handle escape sequences
        uint8_t next_byte = memory_read_byte(&vm->memory, pos + 1);
        switch (next_byte) {
        case 'n':
          printf("\n");
          pos += 2; // Skip both characters
          continue;
        case 't':
          printf("\t");
          pos += 2;
          continue;
        case 'r':
          printf("\r");
          pos += 2;
          continue;
        case '\\':
          printf("\\");
          pos += 2;
          continue;
        case '"':
          printf("\"");
          pos += 2;
          continue;
        default:
          printf("%c", byte); // Print the backslash as-is
          pos++;
          continue;
        }
      } else {
        printf("%c", byte);
        pos++;
      }
    }
  } else if (immediate == SYS_EXIT) {
    vm->cpu.state = VM_STATE_HALTED;
    printf("Program exited with code %d\n", vm->cpu.regs[reg1]);
  } else {
    printf("Unknown system call: %d\n", immediate);
  }
}

void execute_halt(dez_vm_t *vm, uint32_t instruction) {
  (void)instruction; // Suppress unused parameter warning
  vm->cpu.state = VM_STATE_HALTED;
  printf("Program halted\n");
}

void execute_nop(dez_vm_t *vm, uint32_t instruction) {
  (void)vm;          // Suppress unused parameter warning
  (void)instruction; // Suppress unused parameter warning
}

void execute_unknown(dez_vm_t *vm, uint32_t instruction) {
  printf("Error: Unknown instruction 0x%02X at PC %04X\n", instruction >> 24,
         vm->cpu.pc);
  vm->cpu.state = VM_STATE_ERROR;
}

// Instruction table - ordered by frequency for better branch prediction
static const instruction_info_t instruction_table[256] = {
    [INST_MOV] = {execute_mov, 1, false, false, "MOV"},
    [INST_ADD] = {execute_add, 1, false, false, "ADD"},
    [INST_STORE] = {execute_store, 1, true, false, "STORE"},
    [INST_SUB] = {execute_sub, 1, false, false, "SUB"},
    [INST_MUL] = {execute_mul, 1, false, false, "MUL"},
    [INST_DIV] = {execute_div, 1, false, false, "DIV"},
    [INST_JMP] = {execute_jmp, 0, false, false, "JMP"},
    [INST_JZ] = {execute_jz, 1, false, false, "JZ"},
    [INST_JNZ] = {execute_jnz, 1, false, false, "JNZ"},
    [INST_CMP] = {execute_cmp, 1, false, false, "CMP"},
    [INST_SYS] = {execute_sys, 1, false, false, "SYS"},
    [INST_HALT] = {execute_halt, 0, false, false, "HALT"},
    [INST_NOP] = {execute_nop, 1, false, false, "NOP"},
};

// Static unknown instruction info to avoid stack return warning
static const instruction_info_t unknown_instruction = {execute_unknown, 0,
                                                       false, true, "UNKNOWN"};

// Get instruction info
const instruction_info_t *get_instruction_info(uint8_t opcode) {
  if (instruction_table[opcode].execute != NULL) {
    return &instruction_table[opcode];
  }
  return &unknown_instruction;
}