#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== System Calls Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program:
  // MOV R0, 42
  // SYS R0, PRINT     (print R0 value)
  // MOV R1, 65
  // SYS R1, PRINT_CHAR (print character 'A')
  // MOV R2, 0
  // SYS R2, EXIT      (exit with code 0)
  uint32_t program[] = {
      0x1000002A, // MOV R0, 42
      0x0D000001, // SYS R0, PRINT
      0x10100041, // MOV R1, 65 ('A')
      0x0D010002, // SYS R1, PRINT_CHAR
      0x10200000, // MOV R2, 0
      0x0D020004  // SYS R2, EXIT
  };

  // Load program
  for (int i = 0; i < 6; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 6;

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  printf("Program loaded. Executing...\n");
  printf("Expected output: R0 = 42\nA\nProgram exited with code 0\n");
  printf("Actual output: ");

  dez_vm_run(&vm);

  // Check results
  printf("\n\nResults:\n");
  printf("R0: %d (expected: 42)\n", vm.cpu.regs[0]);
  printf("R1: %d (expected: 65)\n", vm.cpu.regs[1]);
  printf("R2: %d (expected: 0)\n", vm.cpu.regs[2]);
  printf("VM State: %d (expected: 1 for HALTED)\n", vm.cpu.state);

  // Verify results
  bool passed = (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 42 &&
                 vm.cpu.regs[1] == 65 && vm.cpu.regs[2] == 0);

  if (passed) {
    printf("\n✅ System calls test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ System calls test FAILED!\n");
    return 1;
  }
}
