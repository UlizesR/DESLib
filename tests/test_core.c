#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <assert.h>
#include <stdio.h>

// Test basic VM initialization and core functionality
int test_vm_init() {
  printf("Testing VM initialization...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Check initial state
  assert(vm.cpu.pc == 0);
  assert(vm.cpu.state == VM_STATE_RUNNING);
  assert(vm.cpu.flags == 0);

  // Check all registers are zero
  for (int i = 0; i < 16; i++) {
    assert(vm.cpu.regs[i] == 0);
  }

  printf("✅ VM initialization test passed\n");
  return 0;
}

// Test MOV instruction with various values
int test_mov_instruction() {
  printf("Testing MOV instruction...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Test program: MOV R0, 42; MOV R1, 255; MOV R15, 1000; HALT
  uint32_t program[] = {
      0x1000002A, // MOV R0, 42
      0x101000FF, // MOV R1, 255
      0x10F003E8, // MOV R15, 1000
      0x00000000  // HALT
  };

  for (int i = 0; i < 4; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 4;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);

  assert(vm.cpu.state == VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 42);
  assert(vm.cpu.regs[1] == 255);
  assert(vm.cpu.regs[15] == 1000);

  printf("✅ MOV instruction test passed\n");
  return 0;
}

// Test arithmetic operations
int test_arithmetic() {
  printf("Testing arithmetic operations...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Test program: MOV R0, 20; MOV R1, 4; ADD R2, R0, R1; SUB R3, R0, R1; MUL
  // R4, R0, R1; DIV R5, R0, R1; HALT
  uint32_t program[] = {
      0x10000014, // MOV R0, 20
      0x10100004, // MOV R1, 4
      0x04201000, // ADD R2, R0, R1 (20 + 4 = 24)
      0x05301000, // SUB R3, R0, R1 (20 - 4 = 16)
      0x06401000, // MUL R4, R0, R1 (20 * 4 = 80)
      0x07501000, // DIV R5, R0, R1 (20 / 4 = 5)
      0x00000000  // HALT
  };

  for (int i = 0; i < 7; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 7;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);

  assert(vm.cpu.state == VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 20);
  assert(vm.cpu.regs[1] == 4);
  assert(vm.cpu.regs[2] == 24); // ADD
  assert(vm.cpu.regs[3] == 16); // SUB
  assert(vm.cpu.regs[4] == 80); // MUL
  assert(vm.cpu.regs[5] == 5);  // DIV

  printf("✅ Arithmetic operations test passed\n");
  return 0;
}

// Test memory operations
int test_memory_ops() {
  printf("Testing memory operations...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Test program: MOV R0, 123; STORE R0, 0x100; MOV R1, 456; STORE R1, 0x101;
  // HALT
  uint32_t program[] = {
      0x1000007B, // MOV R0, 123
      0x03000100, // STORE R0, 0x100
      0x101001C8, // MOV R1, 456
      0x03100101, // STORE R1, 0x101
      0x00000000  // HALT
  };

  for (int i = 0; i < 5; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 5;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);

  // Check memory contents
  uint32_t val1 = memory_read_word(&vm.memory, 0x100);
  uint32_t val2 = memory_read_word(&vm.memory, 0x101);

  assert(vm.cpu.state == VM_STATE_HALTED);
  assert(val1 == 123);
  assert(val2 == 456);

  printf("✅ Memory operations test passed\n");
  return 0;
}

// Test comparison and conditional jumps
int test_conditional_jumps() {
  printf("Testing conditional jumps...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Test program: MOV R0, 5; MOV R1, 5; CMP R0, R1; JZ 5; MOV R2, 1; MOV R3,
  // 10; HALT
  uint32_t program[] = {
      0x10000005, // MOV R0, 5
      0x10100005, // MOV R1, 5
      0x0E010000, // CMP R0, R1 (sets flags = 1)
      0x09000005, // JZ 5 (should jump since flags = 1)
      0x10200001, // MOV R2, 1 (skipped)
      0x1030000A, // MOV R3, 10 (target of JZ)
      0x00000000  // HALT
  };

  for (int i = 0; i < 7; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 7;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);

  assert(vm.cpu.state == VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 5);
  assert(vm.cpu.regs[1] == 5);
  assert(vm.cpu.regs[2] == 0);  // Should be 0 (instruction skipped)
  assert(vm.cpu.regs[3] == 10); // Should be 10 (jump target)
  assert(vm.cpu.flags == 1);    // CMP should set flags

  printf("✅ Conditional jumps test passed\n");
  return 0;
}

// Test unconditional jump
int test_unconditional_jump() {
  printf("Testing unconditional jump...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Test program: MOV R0, 1; JMP 3; MOV R1, 2; MOV R2, 3; HALT
  uint32_t program[] = {
      0x10000001, // MOV R0, 1
      0x08000003, // JMP 3
      0x10100002, // MOV R1, 2 (skipped)
      0x10200003, // MOV R2, 3 (target of JMP)
      0x00000000  // HALT
  };

  for (int i = 0; i < 5; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 5;
  memory_set_protection(&vm.memory, 0, true);

  dez_vm_run(&vm);

  assert(vm.cpu.state == VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 1);
  assert(vm.cpu.regs[1] == 0); // Should be 0 (instruction skipped)
  assert(vm.cpu.regs[2] == 3); // Should be 3 (jump target)

  printf("✅ Unconditional jump test passed\n");
  return 0;
}

// Test system calls
int test_system_calls() {
  printf("Testing system calls...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Test program: MOV R0, 42; SYS R0, PRINT; MOV R1, 65; SYS R1, PRINT_CHAR;
  // HALT
  uint32_t program[] = {
      0x1000002A, // MOV R0, 42
      0x0D000001, // SYS R0, PRINT
      0x10100041, // MOV R1, 65 ('A')
      0x0D010003, // SYS R1, PRINT_CHAR
      0x00000000  // HALT
  };

  for (int i = 0; i < 5; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 5;
  memory_set_protection(&vm.memory, 0, true);

  printf("Expected output: 42A\n");
  printf("Actual output: ");
  dez_vm_run(&vm);
  printf("\n");

  assert(vm.cpu.state == VM_STATE_HALTED);
  assert(vm.cpu.regs[0] == 42);
  assert(vm.cpu.regs[1] == 65);

  printf("✅ System calls test passed\n");
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
  assert(vm1.cpu.state == VM_STATE_ERROR);

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
  assert(vm2.cpu.state == VM_STATE_ERROR);

  printf("✅ Error handling test passed\n");
  return 0;
}

int main() {
  printf("=== DEZ VM Core Functionality Tests ===\n\n");

  int result = 0;
  result += test_vm_init();
  result += test_mov_instruction();
  result += test_arithmetic();
  result += test_memory_ops();
  result += test_conditional_jumps();
  result += test_unconditional_jump();
  result += test_system_calls();
  result += test_error_handling();

  printf("\n=== Test Results ===\n");
  if (result == 0) {
    printf("✅ All core functionality tests PASSED!\n");
    return 0;
  } else {
    printf("❌ Some tests FAILED!\n");
    return 1;
  }
}
