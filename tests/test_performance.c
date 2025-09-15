#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

// Test basic performance with simple operations
int test_basic_performance() {
  printf("Testing basic VM performance...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Create a simple program with many operations
  uint32_t program[100];
  int program_size = 0;

  // Initialize values
  program[program_size++] = 0x10000001; // MOV R0, 1
  program[program_size++] = 0x10100002; // MOV R1, 2
  program[program_size++] = 0x10200003; // MOV R2, 3

  // Add many arithmetic operations
  for (int i = 0; i < 30; i++) {
    program[program_size++] = 0x04201000; // ADD R2, R0, R1
    program[program_size++] = 0x05302000; // SUB R3, R2, R0
    program[program_size++] = 0x06403000; // MUL R4, R3, R1
  }

  program[program_size++] = 0x00000000; // HALT

  for (int i = 0; i < program_size; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = program_size;
  memory_set_protection(&vm.memory, 0, true);

  clock_t start = clock();
  dez_vm_run(&vm);
  clock_t end = clock();

  double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
  int instructions = program_size - 1; // Exclude HALT

  assert(vm.cpu.state == VM_STATE_HALTED);

  printf("  Executed %d instructions in %.6f seconds\n", instructions,
         time_taken);
  printf("  Instructions per second: %.0f\n", instructions / time_taken);

  printf("✅ Basic performance test passed\n");
  return 0;
}

// Test arithmetic performance
int test_arithmetic_performance() {
  printf("Testing arithmetic performance...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Create program with many arithmetic operations
  uint32_t program[1000];
  int program_size = 0;

  // MOV R0, 1; MOV R1, 2; MOV R2, 3; MOV R3, 4
  program[program_size++] = 0x10000001; // MOV R0, 1
  program[program_size++] = 0x10100002; // MOV R1, 2
  program[program_size++] = 0x10200003; // MOV R2, 3
  program[program_size++] = 0x10300004; // MOV R3, 4

  // Add many arithmetic operations
  for (int i = 0; i < 200; i++) {
    program[program_size++] = 0x04201000; // ADD R2, R0, R1
    program[program_size++] = 0x05302000; // SUB R3, R2, R0
    program[program_size++] = 0x06403000; // MUL R4, R3, R1
    program[program_size++] = 0x07504000; // DIV R5, R4, R2
  }

  program[program_size++] = 0x00000000; // HALT

  for (int i = 0; i < program_size; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = program_size;
  memory_set_protection(&vm.memory, 0, true);

  clock_t start = clock();
  dez_vm_run(&vm);
  clock_t end = clock();

  double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
  int instructions = program_size - 1; // Exclude HALT

  assert(vm.cpu.state == VM_STATE_HALTED);

  printf("  Executed %d arithmetic instructions in %.6f seconds\n",
         instructions, time_taken);
  printf("  Instructions per second: %.0f\n", instructions / time_taken);

  printf("✅ Arithmetic performance test passed\n");
  return 0;
}

// Test memory access performance
int test_memory_performance() {
  printf("Testing memory access performance...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Create program with many memory operations
  uint32_t program[1000];
  int program_size = 0;

  // Initialize some values
  program[program_size++] = 0x10000001; // MOV R0, 1
  program[program_size++] = 0x10100002; // MOV R1, 2
  program[program_size++] = 0x10200003; // MOV R2, 3

  // Add many memory operations
  for (int i = 0; i < 200; i++) {
    uint32_t addr = 0x1000 + (i % 100);
    program[program_size++] = 0x03000000 | (addr & 0xFFF); // STORE R0, addr
    program[program_size++] = 0x03100000 | (addr & 0xFFF); // STORE R1, addr+1
    program[program_size++] = 0x03200000 | (addr & 0xFFF); // STORE R2, addr+2
  }

  program[program_size++] = 0x00000000; // HALT

  for (int i = 0; i < program_size; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = program_size;
  memory_set_protection(&vm.memory, 0, true);

  clock_t start = clock();
  dez_vm_run(&vm);
  clock_t end = clock();

  double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
  int instructions = program_size - 1; // Exclude HALT

  assert(vm.cpu.state == VM_STATE_HALTED);

  printf("  Executed %d memory instructions in %.6f seconds\n", instructions,
         time_taken);
  printf("  Instructions per second: %.0f\n", instructions / time_taken);

  printf("✅ Memory performance test passed\n");
  return 0;
}

// Test jump performance
int test_jump_performance() {
  printf("Testing jump performance...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Create program with many jumps
  uint32_t program[1000];
  int program_size = 0;

  // Initialize values
  program[program_size++] = 0x10000001; // MOV R0, 1
  program[program_size++] = 0x10100002; // MOV R1, 2
  program[program_size++] = 0x10200003; // MOV R2, 3

  // Add many jump operations (no loops)
  for (int i = 0; i < 100; i++) {
    program[program_size++] =
        0x08000000 | (program_size + 2);  // JMP to next instruction
    program[program_size++] = 0x04201000; // ADD R2, R0, R1
    program[program_size++] = 0x05302000; // SUB R3, R2, R0
  }

  program[program_size++] = 0x00000000; // HALT

  for (int i = 0; i < program_size; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = program_size;
  memory_set_protection(&vm.memory, 0, true);

  clock_t start = clock();
  dez_vm_run(&vm);
  clock_t end = clock();

  double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
  int instructions = program_size - 1; // Exclude HALT

  assert(vm.cpu.state == VM_STATE_HALTED);

  printf("  Executed %d jump instructions in %.6f seconds\n", instructions,
         time_taken);
  printf("  Instructions per second: %.0f\n", instructions / time_taken);

  printf("✅ Jump performance test passed\n");
  return 0;
}

// Test system call performance
int test_syscall_performance() {
  printf("Testing system call performance...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Create program with many system calls
  uint32_t program[1000];
  int program_size = 0;

  // Initialize values
  program[program_size++] = 0x10000041; // MOV R0, 65 ('A')
  program[program_size++] = 0x10100042; // MOV R1, 66 ('B')
  program[program_size++] = 0x10200043; // MOV R2, 67 ('C')

  // Add many system calls
  for (int i = 0; i < 100; i++) {
    program[program_size++] = 0x0D000001; // SYS R0, PRINT
    program[program_size++] = 0x0D010003; // SYS R1, PRINT_CHAR
    program[program_size++] = 0x0D020001; // SYS R2, PRINT
  }

  program[program_size++] = 0x00000000; // HALT

  for (int i = 0; i < program_size; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = program_size;
  memory_set_protection(&vm.memory, 0, true);

  clock_t start = clock();
  dez_vm_run(&vm);
  clock_t end = clock();

  double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
  int instructions = program_size - 1; // Exclude HALT

  assert(vm.cpu.state == VM_STATE_HALTED);

  printf("  Executed %d system call instructions in %.6f seconds\n",
         instructions, time_taken);
  printf("  Instructions per second: %.0f\n", instructions / time_taken);

  printf("✅ System call performance test passed\n");
  return 0;
}

// Test overall VM performance with mixed operations
int test_mixed_performance() {
  printf("Testing mixed operations performance...\n");

  dez_vm_t vm;
  dez_vm_init(&vm);
  memory_set_protection(&vm.memory, 0, false);

  // Create program with mixed operations
  uint32_t program[1000];
  int program_size = 0;

  // Initialize values
  program[program_size++] = 0x10000001; // MOV R0, 1
  program[program_size++] = 0x10100002; // MOV R1, 2
  program[program_size++] = 0x10200003; // MOV R2, 3
  program[program_size++] = 0x10300004; // MOV R3, 4

  // Add mixed operations
  for (int i = 0; i < 100; i++) {
    program[program_size++] = 0x04201000; // ADD R2, R0, R1
    program[program_size++] = 0x03001000; // STORE R0, 0x1000
    program[program_size++] = 0x05302000; // SUB R3, R2, R0
    program[program_size++] = 0x03101001; // STORE R1, 0x1001
    program[program_size++] = 0x06403000; // MUL R4, R3, R1
    program[program_size++] = 0x03201002; // STORE R2, 0x1002
    program[program_size++] = 0x0E010000; // CMP R0, R1
    program[program_size++] = 0x09000001; // JZ 1 (jump back)
  }

  program[program_size++] = 0x00000000; // HALT

  for (int i = 0; i < program_size; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = program_size;
  memory_set_protection(&vm.memory, 0, true);

  clock_t start = clock();
  dez_vm_run(&vm);
  clock_t end = clock();

  double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
  int instructions = program_size - 1; // Exclude HALT

  assert(vm.cpu.state == VM_STATE_HALTED);

  printf("  Executed %d mixed instructions in %.6f seconds\n", instructions,
         time_taken);
  printf("  Instructions per second: %.0f\n", instructions / time_taken);

  printf("✅ Mixed operations performance test passed\n");
  return 0;
}

int main() {
  printf("=== DEZ VM Performance Tests ===\n\n");

  int result = 0;
  result += test_basic_performance();
  result += test_arithmetic_performance();
  result += test_memory_performance();
  result += test_jump_performance();
  result += test_syscall_performance();
  result += test_mixed_performance();

  printf("\n=== Test Results ===\n");
  if (result == 0) {
    printf("✅ All performance tests PASSED!\n");
    return 0;
  } else {
    printf("❌ Some tests FAILED!\n");
    return 1;
  }
}
