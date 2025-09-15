#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== MOV Instruction Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program: MOV R0, 42; MOV R1, 0x123; MOV R2, 999; MOV R3, 0; HALT
  uint32_t program[] = {
      0x1000002A, // MOV R0, 42 (0x2A = 42)
      0x10100123, // MOV R1, 0x123 (291)
      0x102003E7, // MOV R2, 999 (0x3E7 = 999)
      0x10300000, // MOV R3, 0
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
  printf("R0: %d (expected: 42)\n", vm.cpu.regs[0]);
  printf("R1: %d (expected: 291)\n", vm.cpu.regs[1]);
  printf("R2: %d (expected: 999)\n", vm.cpu.regs[2]);
  printf("R3: %d (expected: 0)\n", vm.cpu.regs[3]);

  // Verify results
  bool passed =
      (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 42 &&
       vm.cpu.regs[1] == 291 && vm.cpu.regs[2] == 999 && vm.cpu.regs[3] == 0);

  if (passed) {
    printf("\n✅ MOV instruction test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ MOV instruction test FAILED!\n");
    return 1;
  }
}
