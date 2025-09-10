#include "vm_instruction_printer.h"
#include "assembly_vm.h"
#include <stdio.h>

// Instruction mnemonics lookup table
static const char *instruction_mnemonics[] = {
    [INST_MOV] = "MOV",        [INST_ADD] = "ADD",       [INST_SUB] = "SUB",
    [INST_MUL] = "MUL",        [INST_DIV] = "DIV",       [INST_LOAD] = "LOAD",
    [INST_STORE] = "STORE",    [INST_JMP] = "JMP",       [INST_JZ] = "JZ",
    [INST_JNZ] = "JNZ",        [INST_PUSH] = "PUSH",     [INST_POP] = "POP",
    [INST_PRINT] = "PRINT",    [INST_PRINTS] = "PRINTS", [INST_INPUT] = "INPUT",
    [INST_CMP] = "CMP",        [INST_HALT] = "HALT",     [INST_NOP] = "NOP",
    [INST_UNKNOWN] = "UNKNOWN"};

// Operand type names
static const char *operand_type_names[] = {[OP_REGISTER] = "REG",
                                           [OP_IMMEDIATE] = "IMM",
                                           [OP_MEMORY] = "MEM",
                                           [OP_LABEL] = "LABEL",
                                           [OP_STRING] = "STRING"};

const char *vm_get_instruction_mnemonic(int type) {
  if (type >= 0 && type < INST_UNKNOWN) {
    return instruction_mnemonics[type];
  }
  return instruction_mnemonics[INST_UNKNOWN];
}

const char *vm_get_operand_type_name(int type) {
  if (type >= 0 && type < 5) {
    return operand_type_names[type];
  }
  return "UNKNOWN";
}

void vm_print_operand(const void *op_ptr, int operand_index) {
  (void)operand_index; // Suppress unused parameter warning
  if (!op_ptr)
    return;

  const operand_t *op = (const operand_t *)op_ptr;

  switch (op->type) {
  case OP_REGISTER:
    printf("R%d", op->reg);
    break;
  case OP_IMMEDIATE:
    printf("#%d", op->value);
    break;
  case OP_MEMORY:
    if (op->reg >= 0 && op->reg < NUM_REGISTERS) {
      printf("[R%d]", op->reg);
    } else {
      printf("[%d]", op->value);
    }
    break;
  case OP_LABEL:
    printf("%s", op->label);
    break;
  case OP_STRING:
    printf("\"%s\"", op->string);
    break;
  default:
    printf("?");
    break;
  }
}

void vm_print_raw_bytes(const uint8_t *bytes, size_t count) {
  for (size_t i = 0; i < count; i++) {
    printf("%02x ", bytes[i]);
  }
}

void vm_print_instruction_simple(const void *inst_ptr) {
  if (!inst_ptr)
    return;

  const instruction_t *inst = (const instruction_t *)inst_ptr;
  printf("%s", vm_get_instruction_mnemonic(inst->type));

  for (int i = 0; i < inst->num_operands; i++) {
    if (i == 0) {
      printf(" ");
    } else {
      printf(", ");
    }
    vm_print_operand(&inst->operands[i], i);
  }
}

void vm_print_instruction_detailed(const void *inst_ptr, uint32_t address) {
  if (!inst_ptr)
    return;

  const instruction_t *inst = (const instruction_t *)inst_ptr;
  printf("0x%08X: %s", address, vm_get_instruction_mnemonic(inst->type));

  for (int i = 0; i < inst->num_operands; i++) {
    if (i == 0) {
      printf(" ");
    } else {
      printf(", ");
    }
    vm_print_operand(&inst->operands[i], i);
  }
}

void vm_print_instruction_objdump(const void *inst_ptr, uint32_t address, const uint8_t *raw_bytes) {
  if (!inst_ptr)
    return;

  const instruction_t *inst = (const instruction_t *)inst_ptr;

  // Print address and raw bytes
  printf("%08x: ", address);
  if (raw_bytes) {
    vm_print_raw_bytes(raw_bytes, 16);
    printf("  ");
  }

  // Print instruction
  printf("%-8s ", vm_get_instruction_mnemonic(inst->type));

  // Print operands
  for (int i = 0; i < inst->num_operands; i++) {
    if (i > 0) {
      printf(", ");
    }
    vm_print_operand(&inst->operands[i], i);
  }
}

void vm_print_instruction_step(const void *inst_ptr, int instruction_num) {
  if (!inst_ptr)
    return;

  printf("PC=%d (0x%04X): ", instruction_num, instruction_num);
  vm_print_instruction_simple(inst_ptr);
}

void vm_print_instruction(const void *inst_ptr, const vm_print_context_t *context) {
  if (!inst_ptr || !context)
    return;

  switch (context->mode) {
  case VM_PRINT_MODE_SIMPLE:
    vm_print_instruction_simple(inst_ptr);
    break;
  case VM_PRINT_MODE_DETAILED:
    vm_print_instruction_detailed(inst_ptr, context->base_address + context->instruction_number * 16);
    break;
  case VM_PRINT_MODE_OBJDUMP:
    vm_print_instruction_objdump(inst_ptr, context->base_address + context->instruction_number * 16, NULL);
    break;
  case VM_PRINT_MODE_STEP:
    vm_print_instruction_step(inst_ptr, context->instruction_number);
    break;
  }
}
