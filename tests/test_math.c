#include "../include/assembly_vm.h"
#include <assert.h>
#include <stdio.h>

int main() {
  vm_t *vm = vm_create();
  assert(vm != NULL);

  if (vm_load_program(vm, "../tests/asm/math.asm")) {
    vm_run(vm);
  } else {
    printf("Failed to load assembly file\n");
    vm_destroy(vm);
    return 1;
  }

  assert(vm->registers[R0] == 3);
  assert(vm->registers[R1] == 5);
  assert(vm->registers[R2] == 4);
  assert(vm->registers[R3] == 21);
  assert(vm->registers[R4] == 10);
  assert(vm->registers[R5] == 20);
  assert(vm->registers[R6] == -10);
  assert(vm->registers[R7] == 17);
  assert(vm->running == 0);

  printf("All arithmetic tests passed!\n");
  vm_destroy(vm);
  return 0;
}