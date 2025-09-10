#include "vm_utils.h"
#include "assembly_vm.h"
#include "vm_errors.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *vm_str_trim(char *str) {
  if (!str)
    return NULL;

  // Trim leading whitespace
  char *start = str;
  while (isspace(*start)) {
    start++;
  }

  // Trim trailing whitespace
  char *end = start + strlen(start) - 1;
  while (end > start && isspace(*end)) {
    end--;
  }
  *(end + 1) = '\0';

  return start;
}

int vm_str_is_empty(const char *str) {
  if (!str)
    return 1;

  while (*str) {
    if (!isspace(*str)) {
      return 0;
    }
    str++;
  }
  return 1;
}

int vm_str_starts_with(const char *str, const char *prefix) {
  if (!str || !prefix)
    return 0;

  size_t prefix_len = strlen(prefix);
  return strncmp(str, prefix, prefix_len) == 0;
}

int vm_str_ends_with(const char *str, const char *suffix) {
  if (!str || !suffix)
    return 0;

  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  if (suffix_len > str_len)
    return 0;

  return strcmp(str + str_len - suffix_len, suffix) == 0;
}

char *vm_str_duplicate(const char *str) {
  if (!str)
    return NULL;

  size_t len = strlen(str) + 1;
  char *dup = malloc(len);
  if (dup) {
    strcpy(dup, str);
  }
  return dup;
}

void vm_str_remove_comments(char *line) {
  if (!line)
    return;

  char *comment = strchr(line, ';');
  if (comment) {
    *comment = '\0';
  }
}

void vm_memory_zero(void *ptr, size_t size) {
  if (ptr) {
    memset(ptr, 0, size);
  }
}

int vm_memory_copy(void *dest, const void *src, size_t size) {
  if (!dest || !src)
    return 0;

  memcpy(dest, src, size);
  return 1;
}

int vm_memory_compare(const void *a, const void *b, size_t size) {
  if (!a || !b)
    return -1;

  return memcmp(a, b, size);
}

int vm_is_valid_register(int reg) { return reg >= 0 && reg < NUM_REGISTERS; }

int vm_is_valid_memory_address(int32_t address) {
  return address >= 0 && address < MEMORY_SIZE - 3;
}

int vm_is_valid_instruction_type(int type) {
  return type >= 0 && type < INST_UNKNOWN;
}

void vm_set_error_simple(vm_error_t *error, vm_error_code_t code,
                         const char *message) {
  if (!error)
    return;

  error->code = code;
  if (message) {
    strncpy(error->message, message, sizeof(error->message) - 1);
    error->message[sizeof(error->message) - 1] = '\0';
  }
}

void vm_print_error_simple(const vm_error_t *error) {
  if (!error || error->code == VM_ERROR_NONE)
    return;

  printf("Error: %s", vm_error_code_to_string(error->code));
  if (strlen(error->message) > 0) {
    printf(" - %s", error->message);
  }
  printf("\n");
}
