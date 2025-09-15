#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>
#include <string.h>

int main() {
  printf("=== String Handling Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Store test string in memory manually
  const char *test_string = "Hello, World!\n";
  uint32_t string_addr = 0x0100; // Valid memory address

  // Write string to memory byte by byte
  for (int i = 0; test_string[i] != '\0'; i++) {
    memory_write_byte(&vm.memory, string_addr + i, test_string[i]);
  }
  memory_write_byte(&vm.memory, string_addr + strlen(test_string),
                    '\0'); // Null terminator

  // Test program:
  // MOV R0, 0x0100  (string address)
  // SYS R0, PRINT_STR
  // MOV R1, 65      (ASCII 'A')
  // SYS R1, PRINT_CHAR
  // MOV R2, 10      (ASCII newline)
  // SYS R2, PRINT_CHAR
  // HALT
  uint32_t program[] = {
      0x10000100, // MOV R0, 0x0100
      0x0D000003, // SYS R0, PRINT_STR
      0x10100041, // MOV R1, 65 ('A')
      0x0D010002, // SYS R1, PRINT_CHAR
      0x1020000A, // MOV R2, 10 ('\n')
      0x0D020002, // SYS R2, PRINT_CHAR
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
  printf("Expected output: Hello, World!\nA\n");
  printf("Actual output: ");

  dez_vm_run(&vm);

  // Check results
  printf("\n\nResults:\n");
  printf("R0: %d (expected: 0x0100)\n", vm.cpu.regs[0]);
  printf("R1: %d (expected: 65)\n", vm.cpu.regs[1]);
  printf("R2: %d (expected: 10)\n", vm.cpu.regs[2]);

  // Verify results
  bool passed = (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 0x0100 &&
                 vm.cpu.regs[1] == 65 && vm.cpu.regs[2] == 10);

  if (passed) {
    printf("\n✅ String handling test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ String handling test FAILED!\n");
    return 1;
  }
}
