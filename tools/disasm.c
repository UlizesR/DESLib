#include "assembly_vm.h"
#include "vm_instruction_printer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple binary format (same as simple_asm.c)
#define DEZ_MAGIC 0xDEADBEEF
#define DEZ_VERSION 1

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t program_size;
} simple_header_t;

// Instruction names for disassembly
static const char *instruction_names[] = {
    "MOV",   "ADD", "SUB",  "MUL",  "DIV",    "LOAD",  "STORE",
    "JMP",   "JZ",  "JNZ",  "PUSH", "POP",    "PRINT", "PRINTS",
    "INPUT", "CMP", "HALT", "NOP",  "UNKNOWN"};

// Operand type names
static const char *operand_type_names[] = {"REG", "IMM", "MEM", "LABEL/STR"};

void print_hex_dump(const void *data, size_t size) {
  const unsigned char *bytes = (const unsigned char *)data;
  for (size_t i = 0; i < size; i++) {
    if (i % 16 == 0) {
      printf("%08zx: ", i);
    }
    printf("%02x ", bytes[i]);
    if (i % 16 == 15) {
      printf("\n");
    }
  }
  if (size % 16 != 0) {
    printf("\n");
  }
}

void disassemble_instruction(uint8_t opcode, uint8_t operand_count,
                             uint8_t operand_types, int32_t *operands,
                             int instruction_num) {
  printf("Instruction %d:\n", instruction_num);
  printf("  Opcode: %d (%s)\n", opcode,
         opcode < sizeof(instruction_names) / sizeof(instruction_names[0])
             ? instruction_names[opcode]
             : "UNKNOWN");
  printf("  Operand count: %d\n", operand_count);
  printf("  Operand types: 0x%02x\n", operand_types);

  // Decode operand types
  for (int i = 0; i < operand_count && i < 3; i++) {
    uint8_t op_type = (operand_types >> (i * 2)) & 0x3;
    printf("  Operand %d: type=%s ", i,
           op_type < sizeof(operand_type_names) / sizeof(operand_type_names[0])
               ? operand_type_names[op_type]
               : "UNKNOWN");

    switch (op_type) {
    case 0: // OP_REGISTER
      printf("(R%d)", operands[i]);
      break;
    case 1: // OP_IMMEDIATE
      printf("(#%d)", operands[i]);
      break;
    case 2: // OP_MEMORY
      printf("([%d])", operands[i]);
      break;
    case 3: // OP_LABEL or OP_STRING
      printf("(label/string: %d)", operands[i]);
      break;
    }
    printf("\n");
  }

  // Show raw bytes for this instruction
  printf("  Raw instruction bytes: ");
  printf("%02x %02x %02x %02x ", opcode, operand_count, operand_types, 0);
  for (int i = 0; i < 3; i++) {
    printf("%02x %02x %02x %02x ", (operands[i] >> 0) & 0xFF,
           (operands[i] >> 8) & 0xFF, (operands[i] >> 16) & 0xFF,
           (operands[i] >> 24) & 0xFF);
  }
  printf("\n\n");
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <binary_file.dez>\n", argv[0]);
    printf(
        "Disassembles a Dez binary file and shows the binary representation\n");
    return 1;
  }

  FILE *file = fopen(argv[1], "rb");
  if (!file) {
    printf("Error: Cannot open file %s\n", argv[1]);
    return 1;
  }

  // Read header
  simple_header_t header;
  if (fread(&header, sizeof(header), 1, file) != 1) {
    printf("Error: Failed to read header\n");
    fclose(file);
    return 1;
  }

  // Validate magic number
  if (header.magic != DEZ_MAGIC) {
    printf("Error: Invalid magic number 0x%08x (expected 0x%08x)\n",
           header.magic, DEZ_MAGIC);
    fclose(file);
    return 1;
  }

  printf("=== Dez Binary File Disassembly ===\n");
  printf("Magic: 0x%08x\n", header.magic);
  printf("Version: %d\n", header.version);
  printf("Program size: %d instructions\n", header.program_size);
  printf("\n");

  // Read and disassemble instructions
  printf("=== Instructions ===\n");
  for (uint32_t i = 0; i < header.program_size; i++) {
    uint8_t opcode, operand_count, operand_types, reserved;
    int32_t operands[3];

    // Read instruction header
    if (fread(&opcode, 1, 1, file) != 1 ||
        fread(&operand_count, 1, 1, file) != 1 ||
        fread(&operand_types, 1, 1, file) != 1 ||
        fread(&reserved, 1, 1, file) != 1) {
      printf("Error: Failed to read instruction header %d\n", i);
      fclose(file);
      return 1;
    }

    // Read operands
    for (int j = 0; j < 3; j++) {
      if (fread(&operands[j], sizeof(int32_t), 1, file) != 1) {
        printf("Error: Failed to read operand %d for instruction %d\n", j, i);
        fclose(file);
        return 1;
      }
    }

    disassemble_instruction(opcode, operand_count, operand_types, operands, i);

    // Skip string data if present
    for (int j = 0; j < operand_count; j++) {
      uint8_t op_type = (operand_types >> (j * 2)) & 0x3;
      if (op_type == 3) { // OP_STRING
        int32_t string_length = operands[j];
        if (fseek(file, string_length, SEEK_CUR) != 0) {
          printf("Error: Failed to skip string data for instruction %d\n", i);
          fclose(file);
          return 1;
        }
        break; // Only one string per instruction in our format
      }
    }
  }

  fclose(file);
  printf("=== End of Disassembly ===\n");
  return 0;
}
