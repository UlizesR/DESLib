#include "dez_disasm.h"
#include "../include/dez_vm_types.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Get instruction mnemonic
const char *dez_get_instruction_mnemonic(uint8_t opcode) {
  switch (opcode) {
  case INST_LOAD:
    return "LOAD";
  case INST_STORE:
    return "STORE";
  case INST_ADD:
    return "ADD";
  case INST_SUB:
    return "SUB";
  case INST_MUL:
    return "MUL";
  case INST_DIV:
    return "DIV";
  case INST_JMP:
    return "JMP";
  case INST_JZ:
    return "JZ";
  case INST_JNZ:
    return "JNZ";
  case INST_CMP:
    return "CMP";
  case INST_SYS:
    return "SYS";
  case INST_HALT:
    return "HALT";
  case INST_NOP:
    return "NOP";
  default:
    return "UNKNOWN";
  }
}

// Get register name
const char *dez_get_register_name(uint8_t reg) {
  static const char *reg_names[] = {"R0",  "R1",  "R2",  "R3", "R4",  "R5",
                                    "R6",  "R7",  "R8",  "R9", "R10", "R11",
                                    "R12", "R13", "R14", "R15"};

  if (reg < 16) {
    return reg_names[reg];
  }
  return "R?";
}

// Get system call name
const char *dez_get_syscall_name(uint32_t syscall) {
  switch (syscall) {
  case SYS_PRINT:
    return "PRINT";
  case SYS_PRINT_STR:
    return "PRINT_STR";
  case SYS_PRINT_CHAR:
    return "PRINT_CHAR";
  case SYS_READ:
    return "READ";
  case SYS_READ_STR:
    return "READ_STR";
  case SYS_EXIT:
    return "EXIT";
  case SYS_DEBUG:
    return "DEBUG";
  default:
    return "UNKNOWN_SYS";
  }
}

// Helper function to safely format strings
static void safe_snprintf(char *output, size_t output_size, const char *format,
                          ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(output, output_size, format, args);
  va_end(args);
  output[output_size - 1] = '\0'; // Ensure null termination
}

// Disassemble a single instruction
void dez_disasm_instruction(uint32_t instruction, char *output,
                            size_t output_size) {
  if (!output || output_size == 0)
    return;

  // Decode instruction
  uint8_t opcode = (instruction >> 24) & 0xFF;
  uint8_t reg1 = (instruction >> 20) & 0xF;
  uint8_t reg2 = (instruction >> 16) & 0xF;
  uint8_t reg3 = (instruction >> 12) & 0xF;
  uint32_t immediate = instruction & 0x0FFF;

  const char *mnemonic = dez_get_instruction_mnemonic(opcode);

  switch (opcode) {
  case INST_LOAD: // LOAD immediate
    safe_snprintf(output, output_size, "%s %s, #%d", mnemonic,
                  dez_get_register_name(reg1), immediate);
    break;

  case INST_STORE: // STORE to memory
    safe_snprintf(output, output_size, "%s %s, [%d]", mnemonic,
                  dez_get_register_name(reg1), immediate);
    break;

  case INST_ADD: // ADD
    safe_snprintf(output, output_size, "%s %s, %s, %s", mnemonic,
                  dez_get_register_name(reg1), dez_get_register_name(reg2),
                  dez_get_register_name(reg3));
    break;

  case INST_SUB: // SUB
    safe_snprintf(output, output_size, "%s %s, %s, %s", mnemonic,
                  dez_get_register_name(reg1), dez_get_register_name(reg2),
                  dez_get_register_name(reg3));
    break;

  case INST_MUL: // MUL
    safe_snprintf(output, output_size, "%s %s, %s, %s", mnemonic,
                  dez_get_register_name(reg1), dez_get_register_name(reg2),
                  dez_get_register_name(reg3));
    break;

  case INST_DIV: // DIV
    safe_snprintf(output, output_size, "%s %s, %s, %s", mnemonic,
                  dez_get_register_name(reg1), dez_get_register_name(reg2),
                  dez_get_register_name(reg3));
    break;

  case INST_JMP: // JUMP
    safe_snprintf(output, output_size, "%s #%d", mnemonic, immediate);
    break;

  case INST_JZ: // JUMP if zero
    safe_snprintf(output, output_size, "%s %s, #%d", mnemonic,
                  dez_get_register_name(reg1), immediate);
    break;

  case INST_JNZ: // JUMP if not zero
    safe_snprintf(output, output_size, "%s %s, #%d", mnemonic,
                  dez_get_register_name(reg1), immediate);
    break;

  case INST_CMP: // COMPARE
    safe_snprintf(output, output_size, "%s %s, %s", mnemonic,
                  dez_get_register_name(reg1), dez_get_register_name(reg2));
    break;

  case INST_SYS: // SYSTEM CALL
    safe_snprintf(output, output_size, "%s %s, %s", mnemonic,
                  dez_get_register_name(reg1), dez_get_syscall_name(immediate));
    break;

  case INST_HALT: // HALT
    safe_snprintf(output, output_size, "%s", mnemonic);
    break;

  case INST_NOP: // NO OPERATION
    safe_snprintf(output, output_size, "%s", mnemonic);
    break;

  default:
    safe_snprintf(output, output_size, "%s 0x%02X (0x%08X)", mnemonic, opcode,
                  instruction);
    break;
  }
}

// Disassemble a range of memory
void dez_disasm_memory(uint32_t *memory, uint32_t start_addr, uint32_t count,
                       bool show_addresses) {
  if (!memory)
    return;

  printf("\n=== Disassembly ===\n");
  for (uint32_t i = 0; i < count; i++) {
    uint32_t addr = start_addr + i;
    uint32_t instruction = memory[addr];
    char disasm[256];

    dez_disasm_instruction(instruction, disasm, sizeof(disasm));

    if (show_addresses) {
      printf("0x%04X: 0x%08X  %s\n", addr, instruction, disasm);
    } else {
      printf("0x%08X  %s\n", instruction, disasm);
    }
  }
  printf("\n");
}

// Disassemble memory using memory system
void dez_disasm_memory_system(void *mem_system, uint32_t start_addr,
                              uint32_t count, bool show_addresses) {
  (void)mem_system;     // Suppress unused parameter warning
  (void)show_addresses; // Suppress unused parameter warning

  // This would be used with the memory system
  // For now, we'll use the direct memory access
  printf("\n=== Disassembly (Memory System) ===\n");
  for (uint32_t i = 0; i < count; i++) {
    uint32_t addr = start_addr + i;

    // Note: This would need to be adapted to work with the memory system
    // For now, we'll show the address and a placeholder
    printf("0x%04X: [Memory System Access]\n", addr);
  }
  printf("\n");
}
