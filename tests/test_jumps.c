#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== Jump and Branch Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program:
  // MOV R0, 5
  // MOV R1, 1
  // JMP 5        (jump to line 5)
  // MOV R1, 0    (should be skipped)
  // MOV R2, 2    (should be skipped)
  // MOV R3, 10   (target of JMP)
  // JZ R1, 9     (jump if R1 is zero to line 9)
  // MOV R4, 20   (should execute)
  // MOV R5, 30   (target of JZ)
  // HALT
  uint32_t program[] = {
      0x10000005, // MOV R0, 5
      0x10100001, // MOV R1, 1
      0x08000005, // JMP 5
      0x10100000, // MOV R1, 0 (skipped)
      0x10200002, // MOV R2, 2 (skipped)
      0x1030000A, // MOV R3, 10 (target of JMP)
      0x09010009, // JZ R1, 9 (should not jump since R1 = 1)
      0x10400014, // MOV R4, 20 (should execute)
      0x1050001E, // MOV R5, 30 (target of JZ)
      0x00000000  // HALT
  };

  // Load program
  for (int i = 0; i < 11; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 11;

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  printf("Program loaded. Executing...\n");
  dez_vm_run(&vm);

  // Check results
  printf("\nResults:\n");
  printf("R0: %d (expected: 5)\n", vm.cpu.regs[0]);
  printf("R1: %d (expected: 1 - JMP should skip the MOV R1, 0)\n",
         vm.cpu.regs[1]);
  printf("R2: %d (expected: 0 - JMP should skip the MOV R2, 2)\n",
         vm.cpu.regs[2]);
  printf("R3: %d (expected: 10 - target of JMP)\n", vm.cpu.regs[3]);
  printf("R4: %d (expected: 20 - JZ should not jump)\n", vm.cpu.regs[4]);
  printf("R5: %d (expected: 0 - JZ should not jump)\n", vm.cpu.regs[5]);
  // Verify results
  bool passed =
      (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 5 &&
       vm.cpu.regs[1] == 1 && vm.cpu.regs[2] == 0 && vm.cpu.regs[3] == 10 &&
       vm.cpu.regs[4] == 20 && vm.cpu.regs[5] == 0);

  if (passed) {
    printf("\n✅ Jump and branch test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ Jump and branch test FAILED!\n");
    return 1;
  }
}
