#include "core/dez_disasm.h"
#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>
#include <time.h>

int main() {
  printf("=== DEZ VM Performance Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Create a performance test program
  printf("Loading performance test program...\n");

  // Temporarily disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Load a program with many instructions
  uint32_t program[] = {
      0x10000001, // MOV R0, 1
      0x10100002, // MOV R1, 2
      0x10200003, // MOV R2, 3
      0x10300004, // MOV R3, 4
      0x10400005, // MOV R4, 5
      0x10500006, // MOV R5, 6
      0x10600007, // MOV R6, 7
      0x10700008, // MOV R7, 8
      0x10800009, // MOV R8, 9
      0x1090000A, // MOV R9, 10

      // Add operations
      0x04210000, // ADD R2, R1, R0
      0x04320000, // ADD R3, R2, R0
      0x04430000, // ADD R4, R3, R0
      0x04540000, // ADD R5, R4, R0
      0x04650000, // ADD R6, R5, R0
      0x04760000, // ADD R7, R6, R0
      0x04870000, // ADD R8, R7, R0
      0x04980000, // ADD R9, R8, R0

      // Multiply operations
      0x06210000, // MUL R2, R1, R0
      0x06320000, // MUL R3, R2, R0
      0x06430000, // MUL R4, R3, R0
      0x06540000, // MUL R5, R4, R0

      // Store operations
      0x03000010, // STORE R0, 16
      0x03100011, // STORE R1, 17
      0x03200012, // STORE R2, 18
      0x03300013, // STORE R3, 19
      0x03400014, // STORE R4, 20

      // Print results
      0x0D200001, // SYS R2, PRINT
      0x0D300001, // SYS R3, PRINT
      0x0D400001, // SYS R4, PRINT
      0x0D500001, // SYS R5, PRINT

      0x00000000 // HALT
  };

  // Load program into memory
  for (int i = 0; i < 25; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  vm.program_size = 25;

  printf("Program loaded. Executing performance test...\n\n");

  // Time the execution
  clock_t start = clock();

  // Run the VM
  dez_vm_run(&vm);

  clock_t end = clock();
  double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("\n=== Performance Results ===\n");
  printf("Execution time: %.6f seconds\n", cpu_time_used);
  printf("Instructions executed: %d\n", vm.cpu.pc);
  printf("Instructions per second: %.0f\n", vm.cpu.pc / cpu_time_used);

  // Print final state
  printf("\n=== Final VM State ===\n");
  dez_vm_print_state(&vm);

  // Print memory statistics
  printf("\n=== Memory Statistics ===\n");
  memory_print_stats(&vm.memory);

  return 0;
}