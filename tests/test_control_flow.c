#include "../include/assembly_vm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test program for control flow operations

int main() {
  vm_t *vm = vm_create();
  assert(vm != NULL);

  // Load program from assembly file
  if (vm_load_program(vm, "../tests/asm/control_flow.asm")) {
    vm_run(vm);
  } else {
    printf("Failed to load assembly file\n");
    vm_destroy(vm);
    return 1;
  }

  // Verify final register state after control flow operations
  // R0 = 10 (unconditional jump worked)
  // R1 = 15 (JZ didn't jump when condition false)
  // R2 = 0 (JZ jumped when condition true)
  // R3 = 7 (JNZ jumped when condition true)
  // R4 = 45 (JNZ didn't jump when condition false)

  assert(vm->registers[R0] == 10);
  assert(vm->registers[R1] == 15);
  assert(vm->registers[R2] == 0);
  assert(vm->registers[R3] == 7);
  assert(vm->registers[R4] == 45);

  // Verify VM stopped properly
  assert(vm->running == 0);

  printf("All control flow tests passed!\n");
  vm_destroy(vm);
  return 0;
}
