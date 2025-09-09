#include "../include/assembly_vm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test program for stack operations

int main() {
  vm_t *vm = vm_create();
  assert(vm != NULL);

  // Load program from assembly file
  if (vm_load_program(vm, "../tests/asm/stack.asm")) {
    vm_run(vm);
  } else {
    printf("Failed to load assembly file\n");
    vm_destroy(vm);
    return 1;
  }

  // Verify final register state after stack operations
  // R0 = 0 (cleared after arithmetic)
  // R1 = 30 (popped from stack)
  // R2 = 0 (cleared)
  // R3 = 300 (popped from stack - last in, first out)
  // R4 = 200 (popped from stack)
  // R5 = 100 (popped from stack - first in, last out)
  // R6 = 10 (first operand)
  // R7 = 20 (second operand)

  assert(vm->registers[R0] == 0);
  assert(vm->registers[R1] == 30);
  assert(vm->registers[R2] == 0);
  assert(vm->registers[R3] == 300);
  assert(vm->registers[R4] == 200);
  assert(vm->registers[R5] == 100);
  assert(vm->registers[R6] == 10);
  assert(vm->registers[R7] == 20);

  // Verify VM stopped properly
  assert(vm->running == 0);

  printf("All stack operation tests passed!\n");
  vm_destroy(vm);
  return 0;
}
