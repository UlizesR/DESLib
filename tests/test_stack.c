#include "../include/assembly_vm.h"
#include <assert.h>
#include <stdio.h>

int main() {
  vm_t *vm = vm_create();
  assert(vm != NULL);

  if (vm_load_program(vm, "../tests/asm/stack.asm")) {
    vm_run(vm);
  } else {
    printf("Failed to load assembly file\n");
    vm_destroy(vm);
    return 1;
  }

  assert(vm->registers[R0] == 0);
  assert(vm->registers[R1] == 30);
  assert(vm->registers[R2] == 0);
  assert(vm->registers[R3] == 300);
  assert(vm->registers[R4] == 200);
  assert(vm->registers[R5] == 100);
  assert(vm->registers[R6] == 10);
  assert(vm->registers[R7] == 20);
  assert(vm->running == 0);

  printf("All stack operation tests passed!\n");
  vm_destroy(vm);
  return 0;
}