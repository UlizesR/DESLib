#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== Division by Zero Debug Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program:
  // MOV R0, 10
  // MOV R1, 0
  // DIV R2, R0, R1   (10 / 0 - should cause error)
  // HALT
  uint32_t program[] = {
      0x1000000A, // MOV R0, 10
      0x10100000, // MOV R1, 0
      0x07501000, // DIV R2, R0, R1 (10 / 0 - should cause error)
      0x00000000  // HALT
  };

  // Load program
  for (int i = 0; i < 4; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 4;

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  printf("Program loaded. Executing step by step...\n");

  // Execute step by step
  for (int step = 0; step < 10 && vm.cpu.state == VM_STATE_RUNNING; step++) {
    printf("Step %d: PC=%d, R0=%d, R1=%d, R2=%d, State=%d\n", step, vm.cpu.pc,
           vm.cpu.regs[0], vm.cpu.regs[1], vm.cpu.regs[2], vm.cpu.state);
    dez_vm_step(&vm);
  }

  printf("\nFinal state:\n");
  printf("  R0: %d (expected: 10)\n", vm.cpu.regs[0]);
  printf("  R1: %d (expected: 0)\n", vm.cpu.regs[1]);
  printf("  R2: %d (expected: 0 - not set due to error)\n", vm.cpu.regs[2]);
  printf("  PC: %d (expected: 2 - stops at DIV)\n", vm.cpu.pc);
  printf("  State: %d (expected: 2 for ERROR)\n", vm.cpu.state);

  // Verify results
  bool passed = (vm.cpu.state == VM_STATE_ERROR && vm.cpu.regs[0] == 10 &&
                 vm.cpu.regs[1] == 0 && vm.cpu.regs[2] == 0 && vm.cpu.pc == 2);

  if (passed) {
    printf("\n✅ Division by zero debug test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ Division by zero debug test FAILED!\n");
    return 1;
  }
}
