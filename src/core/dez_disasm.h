#ifndef DEZ_DISASM_H
#define DEZ_DISASM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Disassemble a single instruction
void dez_disasm_instruction(uint32_t instruction, char *output, size_t output_size);

// Disassemble a range of memory
void dez_disasm_memory(uint32_t *memory, uint32_t start_addr, uint32_t count, bool show_addresses);

// Get instruction mnemonic
const char *dez_get_instruction_mnemonic(uint8_t opcode);

// Get register name
const char *dez_get_register_name(uint8_t reg);

// Get system call name
const char *dez_get_syscall_name(uint32_t syscall);

#endif // DEZ_DISASM_H
