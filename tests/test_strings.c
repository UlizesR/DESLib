#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Test string printing with escape sequences
int test_string_printing() {
  printf("Testing string printing with escape sequences...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Store string "Hello\\nWorld\\t!" at address 0x100
  const char *test_string = "Hello\\nWorld\\t!";
  uint32_t addr = 0x100;

  // Write string to memory
  for (int i = 0; i < strlen(test_string); i++) {
    memory_write_byte(&vm.memory, addr + i, test_string[i]);
  }
  memory_write_byte(&vm.memory, addr + strlen(test_string),
                    0); // Null terminator

  // Test program: MOV R0, 0x100; SYS R0, PRINT_STR; HALT
  uint32_t program[] = {
      0x10000100, // MOV R0, 0x100
      0x0D000002, // SYS R0, PRINT_STR
      0x00000000  // HALT
  };

  for (int i = 0; i < 3; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 3;
  memory_set_protection(&vm.memory, 0, true);

  printf("Expected output: Hello\\nWorld\\t! (with actual newline and tab)\n");
  printf("Actual output: ");
  dez_vm_run(&vm);
  printf("\n");

  assert(vm.cpu.state == VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 0x100);

  printf("✅ String printing test passed\n");
  return 0;
}

// Test character printing
int test_character_printing() {
  printf("Testing character printing...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Test program: MOV R0, 65; SYS R0, PRINT_CHAR; MOV R1, 66; SYS R1,
  // PRINT_CHAR; HALT
  uint32_t program[] = {
      0x10000041, // MOV R0, 65 ('A')
      0x0D000003, // SYS R0, PRINT_CHAR
      0x10100042, // MOV R1, 66 ('B')
      0x0D010003, // SYS R1, PRINT_CHAR
      0x00000000  // HALT
  };

  for (int i = 0; i < 5; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 5;
  memory_set_protection(&vm.memory, 0, true);

  printf("Expected output: AB\n");
  printf("Actual output: ");
  dez_vm_run(&vm);
  printf("\n");

  assert(vm.cpu.state == VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 65);
  assert(vm.cpu.regs[1] == 66);

  printf("✅ Character printing test passed\n");
  return 0;
}

// Test multiple string operations
int test_multiple_strings() {
  printf("Testing multiple string operations...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Store multiple strings
  const char *str1 = "First: ";
  const char *str2 = "Second: ";
  const char *str3 = "Done!";

  uint32_t addr1 = 0x100;
  uint32_t addr2 = 0x110;
  uint32_t addr3 = 0x120;

  // Write strings to memory
  for (int i = 0; i < strlen(str1); i++) {
    memory_write_byte(&vm.memory, addr1 + i, str1[i]);
  }
  memory_write_byte(&vm.memory, addr1 + strlen(str1), 0);

  for (int i = 0; i < strlen(str2); i++) {
    memory_write_byte(&vm.memory, addr2 + i, str2[i]);
  }
  memory_write_byte(&vm.memory, addr2 + strlen(str2), 0);

  for (int i = 0; i < strlen(str3); i++) {
    memory_write_byte(&vm.memory, addr3 + i, str3[i]);
  }
  memory_write_byte(&vm.memory, addr3 + strlen(str3), 0);

  // Test program: Print all three strings
  uint32_t program[] = {
      0x10000100, // MOV R0, 0x100
      0x0D000002, // SYS R0, PRINT_STR
      0x10000110, // MOV R0, 0x110
      0x0D000002, // SYS R0, PRINT_STR
      0x10000120, // MOV R0, 0x120
      0x0D000002, // SYS R0, PRINT_STR
      0x00000000  // HALT
  };

  for (int i = 0; i < 7; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 7;
  memory_set_protection(&vm.memory, 0, true);

  printf("Expected output: First: Second: Done!\n");
  printf("Actual output: ");
  dez_vm_run(&vm);
  printf("\n");

  assert(vm.cpu.state == VM_STATE_HALTED);

  printf("✅ Multiple strings test passed\n");
  return 0;
}

// Test string with special characters
int test_special_characters() {
  printf("Testing special characters in strings...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Store string with various escape sequences
  const char *test_string =
      "Line1\\nLine2\\tTab\\rCarriage\\\"Quote\\\\Backslash";
  uint32_t addr = 0x100;

  // Write string to memory
  for (int i = 0; i < strlen(test_string); i++) {
    memory_write_byte(&vm.memory, addr + i, test_string[i]);
  }
  memory_write_byte(&vm.memory, addr + strlen(test_string), 0);

  // Test program: MOV R0, 0x100; SYS R0, PRINT_STR; HALT
  uint32_t program[] = {
      0x10000100, // MOV R0, 0x100
      0x0D000002, // SYS R0, PRINT_STR
      0x00000000  // HALT
  };

  for (int i = 0; i < 3; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 3;
  memory_set_protection(&vm.memory, 0, true);

  printf(
      "Expected output: Line1\\nLine2\\tTab\\rCarriage\\\"Quote\\\\Backslash "
      "(with actual escapes)\n");
  printf("Actual output: ");
  dez_vm_run(&vm);
  printf("\n");

  assert(vm.cpu.state == VM_STATE_HALTED);

  printf("✅ Special characters test passed\n");
  return 0;
}

int main() {
  printf("=== DEZ VM String Handling Tests ===\n\n");

  int result = 0;
  result += test_string_printing();
  result += test_character_printing();
  result += test_multiple_strings();
  result += test_special_characters();

  printf("\n=== Test Results ===\n");
  if (result == 0) {
    printf("✅ All string handling tests PASSED!\n");
    return 0;
  } else {
    printf("❌ Some tests FAILED!\n");
    return 1;
  }
}