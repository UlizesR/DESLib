#include "../include/assembly_vm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test program for memory operations

int main() {
  vm_t *vm = vm_create();
  assert(vm != NULL);

  // Load program from assembly file
  if (vm_load_program(vm, "../tests/asm/memory.asm")) {
    vm_run(vm);
  } else {
    printf("Failed to load assembly file\n");
    vm_destroy(vm);
    return 1;
  }

  // Verify final register state after memory operations
  // R0 = 100 (stored value)
  // R1 = 200 (stored value)
  // R2 = 100 (loaded from address 10)
  // R3 = 200 (loaded from address 20)
  // R4 = 30 (address)
  // R5 = 300 (stored value)
  // R6 = 300 (loaded from address in R4)

  assert(vm->registers[R0] == 100);
  assert(vm->registers[R1] == 200);
  assert(vm->registers[R2] == 100);
  assert(vm->registers[R3] == 200);
  assert(vm->registers[R4] == 30);
  assert(vm->registers[R5] == 300);
  assert(vm->registers[R6] == 300);

  // Verify VM stopped properly
  assert(vm->running == 0);

  printf("All memory operation tests passed!\n");
  vm_destroy(vm);
  return 0;
}
