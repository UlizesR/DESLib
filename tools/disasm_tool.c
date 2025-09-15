#include "core/dez_disasm.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <instruction1> [instruction2] ...\n", argv[0]);
    printf("Example: %s 0x01000005 0x04210000\n", argv[0]);
    return 1;
  }

  printf("=== DEZ Disassembler ===\n");
  printf("Disassembling %d instruction(s):\n\n", argc - 1);

  for (int i = 1; i < argc; i++) {
    // Parse hexadecimal instruction
    uint32_t instruction = (uint32_t)strtoul(argv[i], NULL, 16);

    // Disassemble instruction
    char disasm[256];
    dez_disasm_instruction(instruction, disasm, sizeof(disasm));

    printf("0x%08X  %s\n", instruction, disasm);
  }

  printf("\n=== Instruction Details ===\n");
  for (int i = 1; i < argc; i++) {
    uint32_t instruction = (uint32_t)strtoul(argv[i], NULL, 16);

    uint8_t opcode = (instruction >> 24) & 0xFF;
    uint8_t reg1 = (instruction >> 20) & 0xF;
    uint8_t reg2 = (instruction >> 16) & 0xF;
    uint8_t reg3 = (instruction >> 12) & 0xF;
    uint32_t immediate = instruction & 0x0FFF;

    printf("\nInstruction 0x%08X:\n", instruction);
    printf("  Opcode: 0x%02X (%s)\n", opcode, dez_get_instruction_mnemonic(opcode));
    printf("  Reg1:   %d (R%d)\n", reg1, reg1);
    printf("  Reg2:   %d (R%d)\n", reg2, reg2);
    printf("  Reg3:   %d (R%d)\n", reg3, reg3);
    printf("  Immediate: %d (0x%03X)\n", immediate, immediate);
  }

  return 0;
}