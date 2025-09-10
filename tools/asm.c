#include "assembly_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple binary format
#define DEZ_MAGIC 0xDEADBEEF
#define DEZ_VERSION 1

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t program_size;
} simple_header_t;

int simple_assemble_file(const char *input_file, const char *output_file) {
  // Create VM to parse the assembly
  vm_t *vm = vm_create();
  if (!vm) {
    printf("Error: Failed to create VM for assembly\n");
    return 0;
  }

  // Load the assembly program
  if (!vm_load_program(vm, input_file)) {
    printf("Error: Failed to load assembly file %s\n", input_file);
    vm_destroy(vm);
    return 0;
  }

  // Open output file
  FILE *out = fopen(output_file, "wb");
  if (!out) {
    printf("Error: Cannot create output file %s\n", output_file);
    vm_destroy(vm);
    return 0;
  }

  // Write header
  simple_header_t header = {.magic = DEZ_MAGIC, .version = DEZ_VERSION, .program_size = vm->program_size};

  if (fwrite(&header, sizeof(header), 1, out) != 1) {
    printf("Error: Failed to write header\n");
    fclose(out);
    vm_destroy(vm);
    return 0;
  }

  // Write instructions in a simple format
  for (int i = 0; i < vm->program_size; i++) {
    instruction_t *inst = &vm->program[i];

    // Write instruction header: opcode, operand_count, operand_types
    uint8_t opcode = (uint8_t)inst->type;
    uint8_t operand_count = inst->num_operands;
    uint8_t operand_types = 0;

    // Encode operand types (2 bits per operand)
    for (int j = 0; j < inst->num_operands && j < 3; j++) {
      uint8_t type_code = 0;
      switch (inst->operands[j].type) {
      case OP_REGISTER:
        type_code = 0;
        break;
      case OP_IMMEDIATE:
        type_code = 1;
        break;
      case OP_MEMORY:
        type_code = 2;
        break;
      case OP_LABEL:
        type_code = 3;
        break;
      case OP_STRING:
        type_code = 3;
        break; // Use same as label for now
      default:
        type_code = 0;
        break;
      }
      operand_types |= (type_code & 0x3) << (j * 2);
    }

    uint8_t reserved = 0;

    if (fwrite(&opcode, 1, 1, out) != 1 || fwrite(&operand_count, 1, 1, out) != 1 || fwrite(&operand_types, 1, 1, out) != 1 || fwrite(&reserved, 1, 1, out) != 1) {
      printf("Error: Failed to write instruction header %d\n", i);
      fclose(out);
      vm_destroy(vm);
      return 0;
    }

    // Write operands (up to 3, each as int32_t)
    for (int j = 0; j < 3; j++) {
      int32_t operand_value = 0;

      if (j < inst->num_operands) {
        switch (inst->operands[j].type) {
        case OP_REGISTER:
          operand_value = inst->operands[j].reg;
          break;
        case OP_IMMEDIATE:
          operand_value = inst->operands[j].value;
          break;
        case OP_LABEL:
          operand_value = find_label(vm, inst->operands[j].label);
          break;
        case OP_STRING:
          // For strings, write the length first
          operand_value = strlen(inst->operands[j].string);
          break;
        default:
          operand_value = 0;
          break;
        }
      }

      if (fwrite(&operand_value, sizeof(int32_t), 1, out) != 1) {
        printf("Error: Failed to write operand %d for instruction %d\n", j, i);
        fclose(out);
        vm_destroy(vm);
        return 0;
      }
    }

    // Write string data if present
    for (int j = 0; j < inst->num_operands; j++) {
      if (inst->operands[j].type == OP_STRING) {
        if (fwrite(inst->operands[j].string, 1, strlen(inst->operands[j].string), out) != strlen(inst->operands[j].string)) {
          printf("Error: Failed to write string data for instruction %d\n", i);
          fclose(out);
          vm_destroy(vm);
          return 0;
        }
      }
    }
  }

  fclose(out);
  vm_destroy(vm);

  printf("Successfully assembled %s to %s\n", input_file, output_file);
  printf("Program size: %d instructions\n", vm->program_size);

  return 1;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <input.asm> <output.dez>\n", argv[0]);
    printf("Assembles a Dez assembly file into a binary .dez file\n");
    printf("\nExample: %s hello_world.asm hello_world.dez\n", argv[0]);
    return 1;
  }

  const char *input_file = argv[1];
  const char *output_file = argv[2];

  // Check if input file exists
  FILE *test = fopen(input_file, "r");
  if (!test) {
    printf("Error: Cannot open input file %s\n", input_file);
    return 1;
  }
  fclose(test);

  // Assemble the file
  if (simple_assemble_file(input_file, output_file)) {
    printf("Assembly completed successfully!\n");
    printf("Binary file created: %s\n", output_file);
    return 0;
  } else {
    printf("Assembly failed!\n");
    return 1;
  }
}
