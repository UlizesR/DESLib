#ifndef VM_UTILS_H
#define VM_UTILS_H

#include "vm_errors.h"
#include <stddef.h>
#include <stdint.h>

// String utility functions
char *vm_str_trim(char *str);
int vm_str_is_empty(const char *str);
int vm_str_starts_with(const char *str, const char *prefix);
int vm_str_ends_with(const char *str, const char *suffix);
char *vm_str_duplicate(const char *str);
void vm_str_remove_comments(char *line);

// Memory utility functions
void vm_memory_zero(void *ptr, size_t size);
int vm_memory_copy(void *dest, const void *src, size_t size);
int vm_memory_compare(const void *a, const void *b, size_t size);

// Validation utility functions
int vm_is_valid_register(int reg);
int vm_is_valid_memory_address(int32_t address);
int vm_is_valid_instruction_type(int type);

// Error handling utilities
void vm_set_error_simple(vm_error_t *error, vm_error_code_t code,
                         const char *message);
void vm_print_error_simple(const vm_error_t *error);

#endif // VM_UTILS_H
