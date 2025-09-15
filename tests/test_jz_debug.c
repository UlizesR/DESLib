#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== JZ Debug Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program:
  // MOV R0, 5
  // MOV R1, 0
  // CMP R0, R1   (compare R0 with R1, sets flags = 0)
  // JZ 6         (jump if flags == 1 to line 6)
  // MOV R2, 10   (should execute)
  // MOV R3, 20   (target of JZ)
  // HALT
  uint32_t program[] = {
      0x10000005, // MOV R0, 5
      0x10100000, // MOV R1, 0
      0x0E010000, // CMP R0, R1 (5 != 0, so flags = 0)
      0x09000006, // JZ 6 (should not jump since flags = 0)
      0x1020000A, // MOV R2, 10 (should execute)
      0x10300014, // MOV R3, 20 (target of JZ)
      0x00000000  // HALT
  };

  // Load program
  for (int i = 0; i < 7; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 7;

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  printf("Program loaded. Executing step by step...\n");

  // Execute step by step
  for (int step = 0; step < 10 && vm.cpu.state == VM_STATE_RUNNING; step++) {
    printf("Step %d: PC=%d, R0=%d, R1=%d, R2=%d, R3=%d, Flags=%d\n", step,
           vm.cpu.pc, vm.cpu.regs[0], vm.cpu.regs[1], vm.cpu.regs[2],
           vm.cpu.regs[3], vm.cpu.flags);
    dez_vm_step(&vm);
  }

  printf("\nFinal state:\n");
  printf("  R0: %d (expected: 5)\n", vm.cpu.regs[0]);
  printf("  R1: %d (expected: 0)\n", vm.cpu.regs[1]);
  printf("  R2: %d (expected: 10 - JZ should not jump)\n", vm.cpu.regs[2]);
  printf("  R3: %d (expected: 20 - JZ should not jump, so R3 gets set)\n",
         vm.cpu.regs[3]);
  printf("  Flags: %d (expected: 0)\n", vm.cpu.flags);
  printf("  PC: %d (expected: 6)\n", vm.cpu.pc);
  printf("  State: %d (expected: 1 for HALTED)\n", vm.cpu.state);

  // Verify results
  bool passed = (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 5 &&
                 vm.cpu.regs[1] == 0 && vm.cpu.regs[2] == 10 &&
                 vm.cpu.regs[3] == 20 && vm.cpu.flags == 0 && vm.cpu.pc == 6);

  if (passed) {
    printf("\n✅ JZ debug test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ JZ debug test FAILED!\n");
    return 1;
  }
}
