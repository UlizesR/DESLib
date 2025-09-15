#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== Arithmetic Operations Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program: MOV R0, 10; MOV R1, 3; ADD R2, R0, R1; SUB R3, R0, R1; MUL
  // R4, R0, R1; DIV R5, R0, R1; HALT
  uint32_t program[] = {
      0x1000000A, // MOV R0, 10
      0x10100003, // MOV R1, 3
      0x04201000, // ADD R2, R0, R1 (R2 = R0 + R1 = 10 + 3 = 13)
      0x05301000, // SUB R3, R0, R1 (R3 = R0 - R1 = 10 - 3 = 7)
      0x06401000, // MUL R4, R0, R1 (R4 = R0 * R1 = 10 * 3 = 30)
      0x07501000, // DIV R5, R0, R1 (R5 = R0 / R1 = 10 / 3 = 3)
      0x00000000  // HALT
  };

  // Load program
  for (int i = 0; i < 7; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 7;

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  printf("Program loaded. Executing...\n");
  dez_vm_run(&vm);

  // Check results
  printf("\nResults:\n");
  printf("R0: %d (expected: 10)\n", vm.cpu.regs[0]);
  printf("R1: %d (expected: 3)\n", vm.cpu.regs[1]);
  printf("R2 (ADD): %d (expected: 13)\n", vm.cpu.regs[2]);
  printf("R3 (SUB): %d (expected: 7)\n", vm.cpu.regs[3]);
  printf("R4 (MUL): %d (expected: 30)\n", vm.cpu.regs[4]);
  printf("R5 (DIV): %d (expected: 3)\n", vm.cpu.regs[5]);

  // Verify results
  bool passed =
      (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 10 &&
       vm.cpu.regs[1] == 3 && vm.cpu.regs[2] == 13 && vm.cpu.regs[3] == 7 &&
       vm.cpu.regs[4] == 30 && vm.cpu.regs[5] == 3);

  if (passed) {
    printf("\n✅ Arithmetic operations test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ Arithmetic operations test FAILED!\n");
    return 1;
  }
}
