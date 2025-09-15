#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== Jump Debug Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program:
  // MOV R0, 5
  // MOV R1, 0
  // JMP 5        (jump to line 5)
  // MOV R1, 1    (should be skipped)
  // MOV R2, 2    (should be skipped)
  // MOV R3, 10   (target of JMP)
  // CMP R0, R1   (compare R0 with R1, sets flags = 0)
  // JZ 5         (jump if flags == 1 to line 5)
  // MOV R4, 20   (should execute)
  // MOV R5, 30   (target of JZ)
  // HALT
  uint32_t program[] = {
      0x10000005, // MOV R0, 5
      0x10100000, // MOV R1, 0
      0x08000005, // JMP 5
      0x10100001, // MOV R1, 1 (skipped)
      0x10200002, // MOV R2, 2 (skipped)
      0x1030000A, // MOV R3, 10 (target of JMP)
      0x0E010000, // CMP R0, R1 (5 != 0, so flags = 0)
      0x09000005, // JZ 5 (should not jump since flags = 0)
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

  printf("Program loaded. Executing step by step...\n");

  // Execute step by step
  for (int step = 0; step < 15 && vm.cpu.state == VM_STATE_RUNNING; step++) {
    printf(
        "Step %d: PC=%d, R0=%d, R1=%d, R2=%d, R3=%d, R4=%d, R5=%d, Flags=%d\n",
        step, vm.cpu.pc, vm.cpu.regs[0], vm.cpu.regs[1], vm.cpu.regs[2],
        vm.cpu.regs[3], vm.cpu.regs[4], vm.cpu.regs[5], vm.cpu.flags);
    dez_vm_step(&vm);
  }

  printf("\nFinal state:\n");
  printf("  R0: %d (expected: 5)\n", vm.cpu.regs[0]);
  printf("  R1: %d (expected: 0 - JMP should skip the MOV R1, 1)\n",
         vm.cpu.regs[1]);
  printf("  R2: %d (expected: 0 - JMP should skip the MOV R2, 2)\n",
         vm.cpu.regs[2]);
  printf("  R3: %d (expected: 10 - target of JMP)\n", vm.cpu.regs[3]);
  printf("  R4: %d (expected: 20 - JZ should not jump)\n", vm.cpu.regs[4]);
  printf("  R5: %d (expected: 0 - JZ should not jump)\n", vm.cpu.regs[5]);
  printf("  Flags: %d (expected: 0)\n", vm.cpu.flags);
  printf("  PC: %d (expected: 10)\n", vm.cpu.pc);
  printf("  State: %d (expected: 1 for HALTED)\n", vm.cpu.state);

  return 0;
}
