#include "../include/assembly_vm.h"
#include <assert.h>
#include <stdio.h>

int main() {
  vm_t *vm = vm_create();
  assert(vm != NULL);

  if (vm_load_program(vm, "../tests/asm/memory.asm")) {
    vm_run(vm);
  } else {
    printf("Failed to load assembly file\n");
    vm_destroy(vm);
    return 1;
  }

  assert(vm->registers[R0] == 100);
  assert(vm->registers[R1] == 200);
  assert(vm->registers[R2] == 100);
  assert(vm->registers[R3] == 200);
  assert(vm->registers[R4] == 30);
  assert(vm->registers[R5] == 300);
  assert(vm->registers[R6] == 300);
  assert(vm->running == 0);

  printf("All memory operation tests passed!\n");
  vm_destroy(vm);
  return 0;
}