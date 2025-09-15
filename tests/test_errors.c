#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== Error Handling Test ===\n\n");

  // Test 1: Division by zero
  printf("Test 1: Division by zero\n");
  dez_vm_t vm1;
  dez_vm_init(&vm1);
  memory_set_protection(&vm1.memory, 0, false);

  uint32_t div_zero_program[] = {
      0x1000000A, // MOV R0, 10
      0x10100000, // MOV R1, 0
      0x07501000, // DIV R2, R0, R1 (10 / 0 - should cause error)
      0x00000000  // HALT
  };

  for (int i = 0; i < 4; i++) {
    memory_write_word(&vm1.memory, i, div_zero_program[i]);
  }
  vm1.program_size = 4;
  memory_set_protection(&vm1.memory, 0, true);

  printf("Executing division by zero test...\n");
  dez_vm_run(&vm1);
  bool div_zero_passed = (vm1.cpu.state == VM_STATE_ERROR);
  printf("Division by zero result: %s\n",
         div_zero_passed ? "PASSED" : "FAILED");

  // Test 2: Unknown instruction
  printf("\nTest 2: Unknown instruction\n");
  dez_vm_t vm2;
  dez_vm_init(&vm2);
  memory_set_protection(&vm2.memory, 0, false);

  uint32_t unknown_inst_program[] = {
      0x10000005, // MOV R0, 5
      0xFF000000, // Unknown instruction (0xFF)
      0x00000000  // HALT
  };

  for (int i = 0; i < 3; i++) {
    memory_write_word(&vm2.memory, i, unknown_inst_program[i]);
  }
  vm2.program_size = 3;
  memory_set_protection(&vm2.memory, 0, true);

  printf("Executing unknown instruction test...\n");
  dez_vm_run(&vm2);
  bool unknown_inst_passed = (vm2.cpu.state == VM_STATE_ERROR);
  printf("Unknown instruction result: %s\n",
         unknown_inst_passed ? "PASSED" : "FAILED");

  // Test 3: Memory bounds check (read from invalid address)
  printf("\nTest 3: Memory bounds check\n");
  dez_vm_t vm3;
  dez_vm_init(&vm3);
  memory_set_protection(&vm3.memory, 0, false);

  // Try to read from address beyond memory size
  uint32_t invalid_addr = 0x10000; // Beyond 16KB memory
  uint32_t value = memory_read_word(&vm3.memory, invalid_addr);
  bool memory_bounds_passed =
      (value == 0); // Should return 0 for invalid address
  printf("Memory bounds check result: %s\n",
         memory_bounds_passed ? "PASSED" : "FAILED");

  // Test 4: Write to read-only code segment
  printf("\nTest 4: Write to read-only code segment\n");
  dez_vm_t vm4;
  dez_vm_init(&vm4);
  // Code protection is enabled by default
  int write_result = memory_write_word(&vm4.memory, 0x0000, 0x12345678);
  bool write_protection_passed = (write_result == -1); // Should fail
  printf("Write protection result: %s\n",
         write_protection_passed ? "PASSED" : "FAILED");

  // Overall results
  bool all_passed = (div_zero_passed && unknown_inst_passed &&
                     memory_bounds_passed && write_protection_passed);

  printf("\n=== Error Handling Test Results ===\n");
  printf("Division by zero: %s\n", div_zero_passed ? "PASSED" : "FAILED");
  printf("Unknown instruction: %s\n",
         unknown_inst_passed ? "PASSED" : "FAILED");
  printf("Memory bounds: %s\n", memory_bounds_passed ? "PASSED" : "FAILED");
  printf("Write protection: %s\n",
         write_protection_passed ? "PASSED" : "FAILED");

  if (all_passed) {
    printf("\n✅ All error handling tests PASSED!\n");
    return 0;
  } else {
    printf("\n❌ Some error handling tests FAILED!\n");
    return 1;
  }
}
