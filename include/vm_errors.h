#ifndef VM_ERRORS_H
#define VM_ERRORS_H

#include <stdint.h>

// Error codes
typedef enum {
  VM_ERROR_NONE = 0,
  VM_ERROR_INVALID_INSTRUCTION,
  VM_ERROR_INVALID_OPERAND,
  VM_ERROR_MEMORY_ACCESS_VIOLATION,
  VM_ERROR_STACK_OVERFLOW,
  VM_ERROR_STACK_UNDERFLOW,
  VM_ERROR_DIVISION_BY_ZERO,
  VM_ERROR_INVALID_INPUT,
  VM_ERROR_FILE_NOT_FOUND,
  VM_ERROR_INVALID_FILE_FORMAT,
  VM_ERROR_LABEL_NOT_FOUND,
  VM_ERROR_DUPLICATE_LABEL,
  VM_ERROR_EXECUTION_LIMIT_EXCEEDED,
  VM_ERROR_INVALID_MEMORY_ADDRESS,
  VM_ERROR_INVALID_REGISTER,
  VM_ERROR_UNKNOWN
} vm_error_code_t;

// Error severity levels
typedef enum {
  VM_ERROR_SEVERITY_INFO,
  VM_ERROR_SEVERITY_WARNING,
  VM_ERROR_SEVERITY_ERROR,
  VM_ERROR_SEVERITY_FATAL
} vm_error_severity_t;

// Error context structure
typedef struct {
  vm_error_code_t code;
  vm_error_severity_t severity;
  char message[256];
  int32_t program_counter;
  int32_t instruction_address;
  char instruction_mnemonic[32];
  char filename[128];
  int line_number;
} vm_error_t;

// Error handling functions
void vm_clear_error(vm_error_t *error);
void vm_set_error(vm_error_t *error, vm_error_code_t code, const char *message,
                  int32_t pc, int32_t addr, const char *filename,
                  const char *mnemonic);
void vm_print_error(const vm_error_t *error);
const char *vm_error_code_to_string(vm_error_code_t code);
const char *vm_error_severity_to_string(vm_error_severity_t severity);

#endif // VM_ERRORS_H
