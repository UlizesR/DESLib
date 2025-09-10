#include "../include/assembly_vm.h"
#include <assert.h>
#include <stdio.h>

int main() {
  vm_t *vm = vm_create();
  assert(vm != NULL);

  if (vm_load_program(vm, "../tests/asm/strings.asm")) {
    vm_run(vm);
  } else {
    printf("Failed to load assembly file\n");
    vm_destroy(vm);
    return 1;
  }

  assert(vm->running == 0);

  printf("All string printing tests passed!\n");
  vm_destroy(vm);
  return 0;
}
