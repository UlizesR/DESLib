#include "dez_vm.h"
#include "dez_instruction_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize the VM
void dez_vm_init(dez_vm_t *vm) {
  // Clear all registers
  memset(vm->cpu.regs, 0, sizeof(vm->cpu.regs));

  // Initialize CPU state
  vm->cpu.pc = 0;                         // Program counter starts at 0
  vm->cpu.sp = DEZ_MEMORY_SIZE_WORDS - 1; // Stack pointer at top of memory
  vm->cpu.flags = 0;                      // Clear all flags
  vm->cpu.state = DEZ_VM_STATE_RUNNING;

  // Initialize memory system
  memory_init(&vm->memory);

  // Initialize other fields
  vm->program_size = 0;
  vm->debug_mode = false;
}

// Load a program from a binary file
void dez_vm_load_program(dez_vm_t *vm, const char *filename) {
  if (!vm || !filename) {
    printf("Error: Invalid parameters for dez_vm_load_program\n");
    if (vm)
      vm->cpu.state = DEZ_VM_STATE_ERROR;
    return;
  }

  FILE *file = fopen(filename, "rb");
  if (!file) {
    printf("Error: Could not open file '%s'\n", filename);
    vm->cpu.state = DEZ_VM_STATE_ERROR;
    return;
  }

  // Read program size
  if (fread(&vm->program_size, sizeof(uint32_t), 1, file) != 1) {
    printf("Error: Could not read program size from '%s'\n", filename);
    fclose(file);
    vm->cpu.state = DEZ_VM_STATE_ERROR;
    return;
  }

  // Validate program size
  if (vm->program_size == 0 || vm->program_size > DEZ_MAX_PROGRAM_SIZE) {
    printf("Error: Invalid program size %u (max %u)\n", vm->program_size, DEZ_MAX_PROGRAM_SIZE);
    fclose(file);
    vm->cpu.state = DEZ_VM_STATE_ERROR;
    return;
  }

  // Get file size to determine how much data to load
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, sizeof(uint32_t), SEEK_SET); // Skip the size header

  // Temporarily disable code protection for program loading
  memory_set_protection(&vm->memory, 0, false);

  // Read instruction data into memory (first vm->program_size words)
  // Note: vm->program_size is the actual instruction count, not the total word
  // count
  for (uint32_t i = 0; i < vm->program_size; i++) {
    uint32_t instruction;
    if (fread(&instruction, sizeof(uint32_t), 1, file) != 1) {
      printf("Error: Could not read instruction %u from '%s'\n", i, filename);
      fclose(file);
      vm->cpu.state = DEZ_VM_STATE_ERROR;
      return;
    }

    if (memory_write_word(&vm->memory, i, instruction) != 0) {
      printf("Error: Could not write instruction %u to memory\n", i);
      fclose(file);
      vm->cpu.state = DEZ_VM_STATE_ERROR;
      return;
    }
  }

  // Load string data into data segment (starting at 0x100)
  // String data is appended after instructions in the file as raw bytes
  // We need to find where the string data actually starts in the file
  uint32_t instruction_data_size = vm->program_size * sizeof(uint32_t);
  uint32_t expected_file_size = sizeof(uint32_t) + instruction_data_size;

  // If file is larger than expected, there's string data
  if (file_size > expected_file_size) {
    // The assembler writes string data after the instruction data
    uint32_t string_data_offset = sizeof(uint32_t) + instruction_data_size;
    fseek(file, string_data_offset, SEEK_SET);

    // Read string data byte by byte and write to VM memory
    int c;
    uint32_t current_addr = 0x100; // Data segment start
    while ((c = fgetc(file)) != EOF && current_addr < 0x200) { // Limit to prevent overflow
      memory_write_byte(&vm->memory, current_addr, (uint8_t)c);
      current_addr++;
    }
  }

  // Re-enable code protection
  memory_set_protection(&vm->memory, 0, true);

  fclose(file);
}

// Execute one instruction
void dez_vm_step(dez_vm_t *vm) {
  if (UNLIKELY(vm->cpu.state != DEZ_VM_STATE_RUNNING)) {
    return;
  }

  // Check bounds
  if (UNLIKELY(vm->cpu.pc >= DEZ_MEMORY_SIZE_WORDS)) {
    printf("Error: Program counter out of bounds\n");
    vm->cpu.state = DEZ_VM_STATE_ERROR;
    return;
  }

  // Fetch instruction using fast memory access
  uint32_t instruction = vm->memory.memory[vm->cpu.pc];

  if (UNLIKELY(vm->debug_mode)) {
    printf("PC: %04X, Instruction: %08X\n", vm->cpu.pc, instruction);
  }

  // Decode opcode and dispatch to instruction handler
  uint8_t opcode = instruction >> 24;
  const instruction_info_t *info = get_instruction_info(opcode);

  // Store the current PC to check if it was modified by the instruction
  uint32_t old_pc = vm->cpu.pc;

  // Execute instruction using dispatch table
  info->execute(vm, instruction);

  // Only increment PC if the instruction didn't modify it
  if (vm->cpu.pc == old_pc) {
    vm->cpu.pc += info->pc_increment;
  }
}

// Run the VM until halt or error
void dez_vm_run(dez_vm_t *vm) {
  int step_count = 0; // Reset step count for each run
  const int MAX_STEPS =
      vm->debug_mode ? 10000 : 100000; // Higher limit for release

  while (LIKELY(vm->cpu.state == DEZ_VM_STATE_RUNNING)) {
    dez_vm_step(vm);

    // Safety check to prevent infinite loops
    if (UNLIKELY(++step_count > MAX_STEPS)) {
      printf("Error: Too many steps (%d), possible infinite loop\n", step_count);
      vm->cpu.state = DEZ_VM_STATE_ERROR;
      break;
    }
  }
}

// Print VM state for debugging
void dez_vm_print_state(dez_vm_t *vm) {
  printf("\n=== VM State ===\n");
  printf("PC: 0x%04X\n", vm->cpu.pc);
  printf("SP: 0x%04X\n", vm->cpu.sp);
  printf("Flags: 0x%08X\n", vm->cpu.flags);
  printf("State: %d\n", vm->cpu.state);

  printf("\nRegisters:\n");
  for (int i = 0; i < 16; i++) {
    printf("R%d: 0x%08X (%d)\n", i, vm->cpu.regs[i], vm->cpu.regs[i]);
  }

  printf("\nMemory (first 32 words):\n");
  for (int i = 0; i < 32 && i < (int)vm->program_size; i++) {
    printf("0x%04X: 0x%08X\n", i, memory_read_word(&vm->memory, i));
  }
}
