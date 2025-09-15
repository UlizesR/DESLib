#include "core/dez_memory.h"
#include "core/dez_vm.h"
#include <stdio.h>

int main() {
  printf("=== Memory Operations Test ===\n\n");

  dez_vm_t vm;
  dez_vm_init(&vm);

  // Disable code protection for program loading
  memory_set_protection(&vm.memory, 0, false);

  // Test program:
  // MOV R0, 42
  // STORE R0, 0x0100  (store R0 at address 0x0100)
  // STORE R0, 0x0101  (store R0 at address 0x0101)
  // HALT
  uint32_t program[] = {
      0x1000002A, // MOV R0, 42
      0x03000100, // STORE R0, 0x0100 (store 42 at 0x0100)
      0x03000101, // STORE R0, 0x0101 (store 42 at 0x0101)
      0x00000000  // HALT
  };

  // Load program
  for (int i = 0; i < 4; i++) {
    memory_write_word(&vm.memory, i, program[i]);
  }
  vm.program_size = 4;

  // Re-enable code protection
  memory_set_protection(&vm.memory, 0, true);

  printf("Program loaded. Executing...\n");
  dez_vm_run(&vm);

  // Check results
  printf("\nResults:\n");
  printf("R0: %d (expected: 42)\n", vm.cpu.regs[0]);

  // Verify memory contents directly
  uint32_t mem_0100 = memory_read_word(&vm.memory, 0x0100);
  uint32_t mem_0101 = memory_read_word(&vm.memory, 0x0101);
  printf("Memory[0x0100]: %d (expected: 42)\n", mem_0100);
  printf("Memory[0x0101]: %d (expected: 42)\n", mem_0101);

  // Verify results
  bool passed = (vm.cpu.state == VM_STATE_HALTED && vm.cpu.regs[0] == 42 &&
                 mem_0100 == 42 && mem_0101 == 42);

  if (passed) {
    printf("\n✅ Memory operations test PASSED!\n");
    return 0;
  } else {
    printf("\n❌ Memory operations test FAILED!\n");
    return 1;
  }
}
