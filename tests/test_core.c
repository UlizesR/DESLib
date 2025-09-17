#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <assert.h>
#include <stdio.h>

// Test basic VM functionality
int test_basic_functionality() {
  printf("Testing basic VM functionality...\n");

  // Test VM initialization
  dez_vm_t vm;
  dez_vm_init(&vm);
  assert(vm.cpu.pc == 0);
  assert(vm.cpu.state == DEZ_VM_STATE_RUNNING);
  assert(vm.cpu.flags == 0);
  for (int i = 0; i < 16; i++) {
    assert(vm.cpu.regs[i] == 0);
  }

  // Test MOV instruction
  uint32_t mov_program[] = {
      0x1000002A, // MOV R0, 42
      0x00000000  // HALT
  };

  memory_set_protection(&vm.memory, 0, false);
  for (int i = 0; i < 2; i++) {
    memory_write_word(&vm.memory, i, mov_program[i]);
  }
  vm.program_size = 2;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);
  assert(vm.cpu.state == DEZ_VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 42);

  printf("✅ Basic functionality passed\n");
  return 0;
}

// Test arithmetic operations
int test_arithmetic_operations() {
  printf("Testing arithmetic operations...\n");

  uint32_t program[] = {
      0x10000014, // MOV R0, 20
      0x10100004, // MOV R1, 4
      0x04201000, // ADD R2, R0, R1 (20 + 4 = 24)
      0x05301000, // SUB R3, R0, R1 (20 - 4 = 16)
      0x06401000, // MUL R4, R0, R1 (20 * 4 = 80)
      0x07501000, // DIV R5, R0, R1 (20 / 4 = 5)
      0x00000000  // HALT
  };

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  for (int i = 0; i < 7; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 7;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);

  assert(vm.cpu.state == DEZ_VM_STATE_HALTED);
  assert(vm.cpu.regs[2] == 24); // ADD
  assert(vm.cpu.regs[3] == 16); // SUB
  assert(vm.cpu.regs[4] == 80); // MUL
  assert(vm.cpu.regs[5] == 5);  // DIV

  printf("✅ Arithmetic operations passed\n");
  return 0;
}

// Test memory operations
int test_memory_operations() {
  printf("Testing memory operations...\n");

  uint32_t program[] = {
      0x1000007B, // MOV R0, 123
      0x03000100, // STORE R0, 256
      0x101001C8, // MOV R1, 456
      0x03100101, // STORE R1, 257
      0x00000000  // HALT
  };

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  for (int i = 0; i < 5; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 5;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);

  assert(vm.cpu.state == DEZ_VM_STATE_HALTED);
  assert(memory_read_word(&vm.memory, 256) == 123);
  assert(memory_read_word(&vm.memory, 257) == 456);

  printf("✅ Memory operations passed\n");
  return 0;
}

// Test conditional jumps
int test_conditional_jumps() {
  printf("Testing conditional jumps...\n");

  uint32_t program[] = {
      0x10000005, // MOV R0, 5
      0x10100005, // MOV R1, 5
      0x0E010000, // CMP R0, R1
      0x09000005, // JZ 5 (should jump since equal)
      0x10200001, // MOV R2, 1 (skipped)
      0x1030000A, // MOV R3, 10 (target of JZ)
      0x00000000  // HALT
  };

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  for (int i = 0; i < 7; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 7;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);

  assert(vm.cpu.state == DEZ_VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 5);
  assert(vm.cpu.regs[1] == 5);
  assert(vm.cpu.regs[2] == 0);  // Should be 0 (instruction skipped)
  assert(vm.cpu.regs[3] == 10); // Should be 10 (jump target)

  printf("✅ Conditional jumps passed\n");
  return 0;
}

// Test system calls
int test_system_calls() {
  printf("Testing system calls...\n");

  uint32_t program[] = {
      0x1000002A, // MOV R0, 42
      0x0D000001, // SYS R0, PRINT
      0x10100041, // MOV R1, 65 ('A')
      0x0D010003, // SYS R1, PRINT_CHAR
      0x00000000  // HALT
  };

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  for (int i = 0; i < 5; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 5;
  memory_set_protection(&vm.memory, 0, true);

  printf("Expected output: 42A\n");
  printf("Actual output: ");
  dez_vm_run(&vm);
  printf("\n");

  assert(vm.cpu.state == DEZ_VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 42);
  assert(vm.cpu.regs[1] == 65);

  printf("✅ System calls passed\n");
  return 0;
}

// Test error handling
int test_error_handling() {
  printf("Testing error handling...\n");

  // Test division by zero
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

  dez_vm_run(&vm1);
  assert(vm1.cpu.state == DEZ_VM_STATE_ERROR);

  // Test unknown instruction
  dez_vm_t vm2;
  dez_vm_init(&vm2);
  memory_set_protection(&vm2.memory, 0, false);

  uint32_t unknown_program[] = {
      0x10000005, // MOV R0, 5
      0xFF000000, // Unknown instruction (0xFF)
      0x00000000  // HALT
  };

  for (int i = 0; i < 3; i++) {
    memory_write_word(&vm2.memory, i, unknown_program[i]);
  }
  vm2.program_size = 3;
  memory_set_protection(&vm2.memory, 0, true);

  dez_vm_run(&vm2);
  assert(vm2.cpu.state == DEZ_VM_STATE_ERROR);

  printf("✅ Error handling passed\n");
  return 0;
}

int main() {
  printf("=== DEZ VM Core Functionality Tests ===\n\n");

  int result = 0;
  result += test_basic_functionality();
  result += test_arithmetic_operations();
  result += test_memory_operations();
  result += test_conditional_jumps();
  result += test_system_calls();
  result += test_error_handling();

  printf("\n=== Test Results ===\n");
  if (result == 0) {
    printf("✅ All core functionality tests PASSED!\n");
    printf("Note: For comprehensive testing of new features (LOAD, bitwise "
           "ops, stack, functions, enhanced jumps), run the assembly tests.\n");
    return 0;
  } else {
    printf("❌ Some tests FAILED!\n");
    return 1;
  }
}