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
  // MOV R1, 5
  // CMP R0, R1   (compare R0 with R1, sets flags = 1)
  // JZ 5         (jump if flags == 1 to line 5)
  // MOV R2, 10   (should be skipped)
  // MOV R3, 20   (target of JZ)
  // HALT
  uint32_t program[] = {
      0x10000005, // MOV R0, 5
      0x10100005, // MOV R1, 5
      0x0E010000, // CMP R0, R1 (5 == 5, so flags = 1)
      0x09000005, // JZ 5 (should jump since flags = 1)
      0x1020000A, // MOV R2, 10 (should be skipped)
      0x10300014, // MOV R3, 20 (target of JZ)
      0x00000000  // HALT
  };

  // Load program
  for (int i = 0; i < 6; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 6;

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  printf("Program loaded. Executing...\n");
  dez_vm_run(&vm);

  // Check results
  printf("\nResults:\n");
  printf("R0: %d (expected: 5)\n", vm.cpu.regs[0]);
  printf("R1: %d (expected: 5)\n", vm.cpu.regs[1]);
  printf("R2: %d (expected: 0 - JZ should jump)\n", vm.cpu.regs[2]);
  printf("R3: %d (expected: 20 - target of JZ)\n", vm.cpu.regs[3]);

  // Verify results
  bool passed =
      (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 5 &&
       vm.cpu.regs[1] == 5 && vm.cpu.regs[2] == 0 && vm.cpu.regs[3] == 20);

  if (passed) {
    printf("\n✅ Simple jump test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ Simple jump test FAILED!\n");
    return 1;
  }
}
