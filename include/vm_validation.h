#ifndef VM_VALIDATION_H
#define VM_VALIDATION_H

#include "vm_errors.h"

// Validation result structure
typedef struct {
  int valid;
  vm_error_t error;
} vm_validation_result_t;

// Validation functions
vm_validation_result_t vm_validate_instruction(const void *inst, int32_t pc);
vm_validation_result_t vm_validate_operand(const void *op, int operand_index);
vm_validation_result_t vm_validate_memory_access(int32_t address);
vm_validation_result_t vm_validate_register(int reg_num);
vm_validation_result_t vm_validate_program(const void *vm);

#endif // VM_VALIDATION_H