#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== Debug CMP Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program:
  // MOV R0, 5
  // MOV R1, 0
  // CMP R0, R1   (compare R0 with R1)
  // HALT
  uint32_t program[] = {
      0x10000005, // MOV R0, 5
      0x10100000, // MOV R1, 0
      0x0E010000, // CMP R0, R1 (5 != 0, so flags should be 0)
      0x00000000  // HALT
  };

  // Load program
  for (int i = 0; i < 4; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 4;

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  printf("Program loaded. Executing...\n");
  dez_vm_run(&vm);

  // Check results
  printf("\nResults:\n");
  printf("R0: %d (expected: 5)\n", vm.cpu.regs[0]);
  printf("R1: %d (expected: 0)\n", vm.cpu.regs[1]);
  printf("Flags: %d (expected: 0 - 5 != 0)\n", vm.cpu.flags);

  // Verify results
  bool passed = (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 5 &&
                 vm.cpu.regs[1] == 0 && vm.cpu.flags == 0);

  if (passed) {
    printf("\n✅ Debug CMP test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ Debug CMP test FAILED!\n");
    return 1;
  }
}
