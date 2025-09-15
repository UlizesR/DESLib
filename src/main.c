#include "core/dez_vm.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <program.dez>\n", argv[0]);
    printf("  program.dez - Dez VM binary program file to execute\n");
    return 1;
  }

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Load the program
  dez_vm_load_program(&vm, argv[1]);

  if (vm.cpu.state == VM_STATE_ERROR) {
    printf("Failed to load program: %s\n", argv[1]);
    return 1;
  }

  // Run the program
  dez_vm_run(&vm);

  return 0;
}
