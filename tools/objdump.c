#include "assembly_vm.h"
#include "vm_instruction_printer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Binary format constants
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

// Function to format hex values
void print_hex_byte(uint8_t byte) { printf("%02x", byte); }

void print_hex_32(uint32_t value) { printf("%08x", value); }

// Function to disassemble a single instruction with memory addresses
void disassemble_instruction_objdump(uint8_t opcode, uint8_t operand_count, uint8_t operand_types, int32_t *operands, int instruction_num, uint32_t base_address) {
  uint32_t instruction_address = base_address + (instruction_num * 16);

  // Print address and raw bytes
  printf("%08x: ", instruction_address);

  // Print raw instruction bytes (16 bytes total)
  printf("%02x %02x %02x %02x ", opcode, operand_count, operand_types, 0);
  for (int i = 0; i < 3; i++) {
    printf("%02x %02x %02x %02x ", (operands[i] >> 0) & 0xFF, (operands[i] >> 8) & 0xFF, (operands[i] >> 16) & 0xFF, (operands[i] >> 24) & 0xFF);
  }
  printf("  ");

  // Print disassembled instruction
  printf("%-8s ",instruction_names[opcode < sizeof(instruction_names) / sizeof(instruction_names[0]) ? opcode : sizeof(instruction_names) / sizeof(instruction_names[0]) - 1]);

  // Print operands
  int operand_printed = 0;
  for (int i = 0; i < operand_count && i < 3; i++) {
    if (operand_printed > 0)
      printf(", ");

    uint8_t op_type = (operand_types >> (i * 2)) & 0x3;

    switch (op_type) {
    case 0: // OP_REGISTER
      printf("R%d", operands[i]);
      break;
    case 1: // OP_IMMEDIATE
      printf("#%d", operands[i]);
      break;
    case 2: // OP_MEMORY
      printf("[%d]", operands[i]);
      break;
    case 3: // OP_LABEL or OP_STRING
      if (operands[i] == 0) {
        printf("\"\"");
      } else {
        printf("0x%x", operands[i]);
      }
      break;
    }
    operand_printed++;
  }

  printf("\n");
}

// Function to show memory layout and execution flow
void show_memory_layout(FILE *file, uint32_t program_size) {
  printf("\n=== Memory Layout ===\n");
  printf("Text Section (Instructions):\n");
  printf("  Start: 0x00000000\n");
  printf("  Size:  %d bytes (%d instructions × 16 bytes)\n", program_size * 16, program_size);
  printf("  End:   0x%08x\n", program_size * 16 - 1);

  printf("\nData Section (Strings):\n");
  printf("  Start: 0x%08x\n", program_size * 16);
  printf("  Size:  Variable (depends on string literals)\n");

  printf("\nStack:\n");
  printf("  Start: 0x%08x (grows downward)\n", 0x40000000);
  printf("  Size:  1024 bytes\n");

  printf("\nRegisters:\n");
  printf("  R0-R7: 0x%08x - 0x%08x\n", 0x50000000, 0x5000001C);
  printf("  PC:    0x%08x\n", 0x50000020);
  printf("  SP:    0x%08x\n", 0x50000024);
  printf("  Flags: 0x%08x\n", 0x50000028);
}

// Function to show execution flow
void show_execution_flow(FILE *file, uint32_t program_size) {
  printf("\n=== Execution Flow ===\n");
  printf("Program Entry Point: 0x00000000\n");
  printf("Execution Order:\n");

  for (uint32_t i = 0; i < program_size; i++) {
    uint32_t address = i * 16;
    printf("  0x%08x: Instruction %d\n", address, i);
  }

  printf("\nJump Targets:\n");
  printf("  (Labels resolved during assembly)\n");
}

// Function to show symbol table
void show_symbol_table(FILE *file, uint32_t program_size) {
  printf("\n=== Symbol Table ===\n");
  printf("Address    Type     Name\n");
  printf("--------   -------  ----\n");
  printf("0x00000000 FUNC     _start\n");

  for (uint32_t i = 0; i < program_size; i++) {
    uint32_t address = i * 16;
    printf("0x%08x INST     instruction_%d\n", address, i);
  }

  printf("0x%08x DATA     string_data\n", program_size * 16);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <binary_file.dez>\n", argv[0]);
    printf("Disassembles a Dez binary file with memory addresses and execution flow\n");
    printf("\nExample: %s bin/hello_world.dez\n", argv[0]);
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
    printf("Error: Invalid magic number 0x%08x (expected 0x%08x)\n", header.magic, DEZ_MAGIC);
    fclose(file);
    return 1;
  }

  printf("Dez Binary File Disassembly\n");
  printf("File: %s\n", argv[1]);
  printf("Magic: 0x%08x\n", header.magic);
  printf("Version: %d\n", header.version);
  printf("Program size: %d instructions\n", header.program_size);
  printf("Text size: %d bytes\n", header.program_size * 16);
  printf("\n");

  // Show memory layout
  // show_memory_layout(file, header.program_size);

  // // Show execution flow
  // show_execution_flow(file, header.program_size);

  // // Show symbol table
  // show_symbol_table(file, header.program_size);

  // Disassemble instructions
  printf("\n=== Disassembly of .text section ===\n");
  printf("Address    Raw Bytes                    Instruction\n");
  printf("--------   ---------------------------  -----------\n");

  for (uint32_t i = 0; i < header.program_size; i++) {
    uint8_t opcode, operand_count, operand_types, reserved;
    int32_t operands[3];

    // Read instruction header
    if (fread(&opcode, 1, 1, file) != 1 || fread(&operand_count, 1, 1, file) != 1 || fread(&operand_types, 1, 1, file) != 1 || fread(&reserved, 1, 1, file) != 1) {
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

    disassemble_instruction_objdump(opcode, operand_count, operand_types, operands, i, 0x00000000);

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
        break; 
      }
    }
  }

  // Show data section (strings)
  long data_start = ftell(file);
  if (data_start > 0) {
    printf("\n=== Disassembly of .data section ===\n");
    printf("Address    Raw Bytes                    ASCII\n");
    printf("--------   ---------------------------  -----\n");

    fseek(file, data_start, SEEK_SET);
    int byte_count = 0;
    int c;
    while ((c = fgetc(file)) != EOF) {
      if (byte_count % 16 == 0) {
        printf("0x%08lx  ", (unsigned long)(data_start + byte_count));
      }
      printf("%02x ", c);
      if (byte_count % 16 == 15) {
        printf(" ");
        // Show ASCII representation
        fseek(file, data_start + byte_count - 15, SEEK_SET);
        for (int i = 0; i < 16; i++) {
          int ch = fgetc(file);
          printf("%c", (ch >= 32 && ch <= 126) ? ch : '.');
        }
        printf("\n");
      }
      byte_count++;
    }

    // Handle remaining bytes
    if (byte_count % 16 != 0) {
      for (int i = byte_count % 16; i < 16; i++) {
        printf("   ");
      }
      printf(" ");
      fseek(file, data_start + (byte_count / 16) * 16, SEEK_SET);
      for (int i = 0; i < byte_count % 16; i++) {
        int ch = fgetc(file);
        printf("%c", (ch >= 32 && ch <= 126) ? ch : '.');
      }
      printf("\n");
    }
  }

  fclose(file);
  printf("\n=== End of Disassembly ===\n");
  return 0;
}
