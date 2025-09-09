#include "vm_validation.h"
#include "assembly_vm.h"
#include <stdio.h>

/**
 * Validate instruction
 */
vm_validation_result_t vm_validate_instruction(const void *inst_ptr,
                                               int32_t pc) {
  vm_validation_result_t result = {1, {0}};
  const instruction_t *inst = (const instruction_t *)inst_ptr;

  if (!inst) {
    vm_set_error(&result.error, VM_ERROR_INVALID_INSTRUCTION,
                 "Null instruction", pc, -1, NULL, NULL);
    result.valid = 0;
    return result;
  }

  // Validate instruction type
  if (inst->type < 0 || inst->type >= INST_UNKNOWN) {
    vm_set_error(&result.error, VM_ERROR_INVALID_INSTRUCTION,
                 "Invalid instruction type", pc, -1, NULL, NULL);
    result.valid = 0;
    return result;
  }

  // Validate operands
  for (int i = 0; i < inst->num_operands; i++) {
    vm_validation_result_t op_result =
        vm_validate_operand(&inst->operands[i], i);
    if (!op_result.valid) {
      result = op_result;
      return result;
    }
  }

  return result;
}

/**
 * Validate operand
 */
vm_validation_result_t vm_validate_operand(const void *op_ptr,
                                           int operand_index) {
  (void)operand_index; // Suppress unused parameter warning
  vm_validation_result_t result = {1, {0}};
  const operand_t *op = (const operand_t *)op_ptr;

  if (!op) {
    vm_set_error(&result.error, VM_ERROR_INVALID_OPERAND, "Null operand", -1,
                 -1, NULL, NULL);
    result.valid = 0;
    return result;
  }

  switch (op->type) {
  case OP_REGISTER:
    if (op->reg < 0 || op->reg >= NUM_REGISTERS) {
      vm_set_error(&result.error, VM_ERROR_INVALID_REGISTER,
                   "Invalid register number", -1, -1, NULL, NULL);
      result.valid = 0;
    }
    break;

  case OP_MEMORY:
    if (op->value < 0 || op->value >= MEMORY_SIZE - 3) {
      vm_set_error(&result.error, VM_ERROR_INVALID_MEMORY_ADDRESS,
                   "Invalid memory address", -1, -1, NULL, NULL);
      result.valid = 0;
    }
    break;

  case OP_IMMEDIATE:
    // Immediate values are always valid
    break;

  case OP_LABEL:
    // Label validation happens during assembly
    break;

  default:
    vm_set_error(&result.error, VM_ERROR_INVALID_OPERAND,
                 "Invalid operand type", -1, -1, NULL, NULL);
    result.valid = 0;
    break;
  }

  return result;
}

/**
 * Validate memory access
 */
vm_validation_result_t vm_validate_memory_access(int32_t address) {
  vm_validation_result_t result = {1, {0}};

  if (address < 0 || address >= MEMORY_SIZE - 3) {
    vm_set_error(&result.error, VM_ERROR_MEMORY_ACCESS_VIOLATION,
                 "Memory access out of bounds", -1, address, NULL, NULL);
    result.valid = 0;
  }

  return result;
}

/**
 * Validate register number
 */
vm_validation_result_t vm_validate_register(int reg_num) {
  vm_validation_result_t result = {1, {0}};

  if (reg_num < 0 || reg_num >= NUM_REGISTERS) {
    vm_set_error(&result.error, VM_ERROR_INVALID_REGISTER,
                 "Invalid register number", -1, -1, NULL, NULL);
    result.valid = 0;
  }

  return result;
}

/**
 * Validate entire program
 */
vm_validation_result_t vm_validate_program(const void *vm_ptr) {
  vm_validation_result_t result = {1, {0}};
  const vm_t *vm = (const vm_t *)vm_ptr;

  if (!vm) {
    vm_set_error(&result.error, VM_ERROR_INVALID_INSTRUCTION, "Null VM", -1, -1,
                 NULL, NULL);
    result.valid = 0;
    return result;
  }

  // Validate each instruction
  for (int i = 0; i < vm->program_size; i++) {
    vm_validation_result_t inst_result =
        vm_validate_instruction(&vm->program[i], i);
    if (!inst_result.valid) {
      result = inst_result;
      return result;
    }
  }

  return result;
}