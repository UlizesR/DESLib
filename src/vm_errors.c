#include "vm_errors.h"
#include <stdio.h>
#include <string.h>

/**
 * Clear error state
 */
void vm_clear_error(vm_error_t *error) {
  if (!error)
    return;

  memset(error, 0, sizeof(vm_error_t));
  error->code = VM_ERROR_NONE;
  error->severity = VM_ERROR_SEVERITY_INFO;
}

/**
 * Set error information
 */
void vm_set_error(vm_error_t *error, vm_error_code_t code, const char *message, int32_t pc, int32_t addr, const char *filename, const char *mnemonic) {
  if (!error)
    return;

  error->code = code;
  error->program_counter = pc;
  error->instruction_address = addr;

  // Set severity based on error code
  switch (code) {
  case VM_ERROR_NONE:
    error->severity = VM_ERROR_SEVERITY_INFO;
    break;
  case VM_ERROR_INVALID_INPUT:
    error->severity = VM_ERROR_SEVERITY_WARNING;
    break;
  case VM_ERROR_MEMORY_ACCESS_VIOLATION:
  case VM_ERROR_STACK_OVERFLOW:
  case VM_ERROR_STACK_UNDERFLOW:
  case VM_ERROR_DIVISION_BY_ZERO:
  case VM_ERROR_EXECUTION_LIMIT_EXCEEDED:
    error->severity = VM_ERROR_SEVERITY_ERROR;
    break;
  case VM_ERROR_INVALID_INSTRUCTION:
  case VM_ERROR_FILE_NOT_FOUND:
  case VM_ERROR_INVALID_FILE_FORMAT:
    error->severity = VM_ERROR_SEVERITY_FATAL;
    break;
  default:
    error->severity = VM_ERROR_SEVERITY_ERROR;
    break;
  }

  // Copy message
  if (message) {
    strncpy(error->message, message, sizeof(error->message) - 1);
    error->message[sizeof(error->message) - 1] = '\0';
  }

  // Copy filename
  if (filename) {
    strncpy(error->filename, filename, sizeof(error->filename) - 1);
    error->filename[sizeof(error->filename) - 1] = '\0';
  }

  // Copy instruction mnemonic
  if (mnemonic) {
    strncpy(error->instruction_mnemonic, mnemonic,
            sizeof(error->instruction_mnemonic) - 1);
    error->instruction_mnemonic[sizeof(error->instruction_mnemonic) - 1] = '\0';
  }
}

/**
 * Print error information
 */
void vm_print_error(const vm_error_t *error) {
  if (!error || error->code == VM_ERROR_NONE)
    return;

  printf("VM Error [%s]: %s\n", vm_error_severity_to_string(error->severity), vm_error_code_to_string(error->code));

  if (strlen(error->message) > 0) {
    printf("  Message: %s\n", error->message);
  }

  if (error->program_counter >= 0) {
    printf("  Program Counter: %d\n", error->program_counter);
  }

  if (error->instruction_address >= 0) {
    printf("  Instruction Address: 0x%04X\n", error->instruction_address);
  }

  if (strlen(error->instruction_mnemonic) > 0) {
    printf("  Instruction: %s\n", error->instruction_mnemonic);
  }

  if (strlen(error->filename) > 0) {
    printf("  File: %s", error->filename);
    if (error->line_number > 0) {
      printf(":%d", error->line_number);
    }
    printf("\n");
  }
}

/**
 * Convert error code to string
 */
const char *vm_error_code_to_string(vm_error_code_t code) {
  switch (code) {
  case VM_ERROR_NONE:
    return "No Error";
  case VM_ERROR_INVALID_INSTRUCTION:
    return "Invalid Instruction";
  case VM_ERROR_INVALID_OPERAND:
    return "Invalid Operand";
  case VM_ERROR_MEMORY_ACCESS_VIOLATION:
    return "Memory Access Violation";
  case VM_ERROR_STACK_OVERFLOW:
    return "Stack Overflow";
  case VM_ERROR_STACK_UNDERFLOW:
    return "Stack Underflow";
  case VM_ERROR_DIVISION_BY_ZERO:
    return "Division by Zero";
  case VM_ERROR_INVALID_INPUT:
    return "Invalid Input";
  case VM_ERROR_FILE_NOT_FOUND:
    return "File Not Found";
  case VM_ERROR_INVALID_FILE_FORMAT:
    return "Invalid File Format";
  case VM_ERROR_LABEL_NOT_FOUND:
    return "Label Not Found";
  case VM_ERROR_DUPLICATE_LABEL:
    return "Duplicate Label";
  case VM_ERROR_EXECUTION_LIMIT_EXCEEDED:
    return "Execution Limit Exceeded";
  case VM_ERROR_INVALID_MEMORY_ADDRESS:
    return "Invalid Memory Address";
  case VM_ERROR_INVALID_REGISTER:
    return "Invalid Register";
  case VM_ERROR_UNKNOWN:
    return "Unknown Error";
  default:
    return "Invalid Error Code";
  }
}

/**
 * Convert error severity to string
 */
const char *vm_error_severity_to_string(vm_error_severity_t severity) {
  switch (severity) {
  case VM_ERROR_SEVERITY_INFO:
    return "INFO";
  case VM_ERROR_SEVERITY_WARNING:
    return "WARNING";
  case VM_ERROR_SEVERITY_ERROR:
    return "ERROR";
  case VM_ERROR_SEVERITY_FATAL:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}
