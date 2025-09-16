#include "assembler/dez_assembler.h"
#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Helper function to get the correct path to assembly files
const char *get_asm_path(const char *filename) {
  static char path[512];

  // Check if we're in the build directory
  if (access("tests/asm", F_OK) == 0) {
    // We're in the project root
    snprintf(path, sizeof(path), "tests/asm/%s", filename);
  } else if (access("../tests/asm", F_OK) == 0) {
    // We're in the build directory
    snprintf(path, sizeof(path), "../tests/asm/%s", filename);
  } else {
    // Fallback - assume we're in project root
    snprintf(path, sizeof(path), "tests/asm/%s", filename);
  }

  return path;
}

// Helper function to test assembly file
int test_assembly_file(const char *filename, const char *test_name) {
  printf("Testing %s...\n", test_name);

  assembler_t assembler;
  assembler_init(&assembler, 100, false);

  const char *full_path = get_asm_path(filename);
  char output_file[256];
  snprintf(output_file, sizeof(output_file), "%s.bin", full_path);

  bool success = assembler_assemble_file(&assembler, full_path, output_file);
  if (!success) {
    printf("❌ %s failed - could not assemble file\n", test_name);
    assembler_cleanup(&assembler);
    return 1;
  }

  dez_vm_t vm;
  dez_vm_init(&vm);
  dez_vm_load_program(&vm, output_file);
  dez_vm_run(&vm);

  if (vm.cpu.state == VM_STATE_HALTED) {
    printf("✅ %s passed\n", test_name);
    assembler_cleanup(&assembler);
    remove(output_file);
    return 0;
  } else {
    printf("❌ %s failed - VM state: %d\n", test_name, vm.cpu.state);
    assembler_cleanup(&assembler);
    remove(output_file);
    return 1;
  }
}

// Test basic assembly functionality
int test_basic_assembly() {
  printf("Testing basic assembly functionality...\n");

  assembler_t assembler;
  assembler_init(&assembler, 100, false);

  // Test assembly string: MOV R0, 42; MOV R1, 10; ADD R2, R0, R1; HALT
  const char *assembly_code = "MOV R0, 42\n"
                              "MOV R1, 10\n"
                              "ADD R2, R0, R1\n"
                              "HALT\n";

  uint32_t *output = NULL;
  int size = 0;
  bool success =
      assembler_assemble_string(&assembler, assembly_code, &output, &size);

  assert(success);
  assert(output != NULL);
  assert(size > 0);

  // Initialize VM and load the assembled program
  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  for (int i = 0; i < size; i++) {
    memory_write_word(&vm.memory, i, output[i]);
  }
  vm.program_size = size;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);

  assert(vm.cpu.state == VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 42);
  assert(vm.cpu.regs[1] == 10);
  assert(vm.cpu.regs[2] == 52); // 42 + 10

  assembler_cleanup(&assembler);
  printf("✅ Basic assembly test passed\n");
  return 0;
}

// Test error handling in assembly
int test_assembly_errors() {
  printf("Testing assembly error handling...\n");

  assembler_t assembler;
  assembler_init(&assembler, 100, false);

  // Test invalid instruction
  const char *invalid_code = "INVALID R0, 42\nHALT\n";
  uint32_t *output = NULL;
  int size = 0;
  bool success =
      assembler_assemble_string(&assembler, invalid_code, &output, &size);

  // Should fail to assemble
  assert(!success);

  assembler_cleanup(&assembler);
  printf("✅ Assembly error handling test passed\n");
  return 0;
}

int main() {
  printf("=== DEZ Assembly and VM Integration Tests ===\n\n");

  int result = 0;
  result += test_basic_assembly();
  result += test_assembly_errors();

  // Test all assembly files
  result += test_assembly_file("test_basic.s", "Basic assembly file");
  result += test_assembly_file("test_memory.s", "Memory operations");
  result += test_assembly_file("test_jumps.s", "Jump operations");
  result += test_assembly_file("test_conditional.s", "Conditional jumps");
  result += test_assembly_file("test_system.s", "System calls");
  result += test_assembly_file("test_loop.s", "Loop operations");
  result += test_assembly_file("test_labels.s", "Label functionality");
  result += test_assembly_file("test_comments.s", "Comment functionality");
  result += test_assembly_file("test_mixed.s", "Mixed features");
  result += test_assembly_file("test_load.s", "LOAD instruction");
  result += test_assembly_file("test_bitwise.s", "Bitwise operations");
  result += test_assembly_file("test_stack.s", "Stack operations");
  result += test_assembly_file("test_functions.s", "Function calls");
  result += test_assembly_file("test_comprehensive.s", "Comprehensive features test");

  printf("\n=== Test Results ===\n");
  if (result == 0) {
    printf("✅ All assembly integration tests PASSED!\n");
    return 0;
  } else {
    printf("❌ Some assembly tests FAILED!\n");
    return 1;
  }
}