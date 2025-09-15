#include "core/dez_disasm.h"
#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== DEZ VM Test ===\n\n");

  // Initialize VM
  dez_vm_t vm;
  dez_vm_init(&vm);

  printf("VM initialized successfully\n");

  // Load a simple test program
  printf("Loading test program...\n");

  // Temporarily disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Simple program: MOV R0, 5; MOV R1, 3; ADD R2, R0, R1; SYS R2, PRINT; HALT
  memory_write_word(&vm.memory, 0, 0x10000005); // MOV R0, 5
  memory_write_word(&vm.memory, 1, 0x10100003); // MOV R1, 3
  memory_write_word(&vm.memory, 2, 0x04201000); // ADD R2, R0, R1
  memory_write_word(&vm.memory, 3, 0x0D200001); // SYS R2, PRINT
  memory_write_word(&vm.memory, 4, 0x00000000); // HALT

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  vm.program_size = 5;

  printf("Program loaded. Executing...\n");

  // Run the VM
  dez_vm_run(&vm);

  // Check final state
  printf("\nFinal VM state:\n");
  printf("PC: 0x%04X\n", vm.cpu.pc);
  printf("State: %d\n", vm.cpu.state);
  printf("R0: %d, R1: %d, R2: %d\n", vm.cpu.regs[0], vm.cpu.regs[1],
         vm.cpu.regs[2]);

  // Verify expected results
  if (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[2] == 8) {
    printf("\n✅ VM test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ VM test FAILED!\n");
    return 1;
  }
}