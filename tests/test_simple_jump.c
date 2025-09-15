#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== Simple Jump Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program:
  // MOV R0, 5
  // JZ R0, 4     (jump if R0 is zero to line 4)
  // MOV R1, 10   (should be skipped)
  // MOV R2, 20   (target of JZ)
  // HALT
  uint32_t program[] = {
      0x10000005, // MOV R0, 5
      0x09000004, // JZ R0, 4 (should not jump since R0 = 5)
      0x1010000A, // MOV R1, 10 (should execute)
      0x10200014, // MOV R2, 20 (target of JZ)
      0x00000000  // HALT
  };

  // Load program
  for (int i = 0; i < 5; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 5;

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  printf("Program loaded. Executing...\n");
  dez_vm_run(&vm);

  // Check results
  printf("\nResults:\n");
  printf("R0: %d (expected: 5)\n", vm.cpu.regs[0]);
  printf("R1: %d (expected: 10 - JZ should not jump)\n", vm.cpu.regs[1]);
  printf("R2: %d (expected: 0 - JZ should not jump)\n", vm.cpu.regs[2]);

  // Verify results
  bool passed = (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 5 &&
                 vm.cpu.regs[1] == 10 && vm.cpu.regs[2] == 0);

  if (passed) {
    printf("\n✅ Simple jump test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ Simple jump test FAILED!\n");
    return 1;
  }
}
