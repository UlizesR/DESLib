#include "dez_instruction_table.h"
#include "../include/dez_vm_types.h"
#include "dez_memory.h"
#include "dez_vm.h"
#include <stdio.h>

// Instruction execution functions
// Each function takes a VM pointer and a 32-bit instruction word
// The instruction format is: (opcode << 24) | (operands...)

/**
 * Execute MOV instruction: Load immediate value into register
 * Format: (INST_MOV << 24) | (reg << 20) | (immediate & 0x0FFF)
 * Example: MOV R0, 42 -> 0x1000002A
 */
void execute_mov(dez_vm_t *vm, uint32_t instruction) {
  // Optimized: direct register access without bounds checking in hot path
  vm->cpu.regs[instruction >> 20 & 0xF] = instruction & 0x0FFF;
}

/**
 * Execute LOAD instruction: Load value from memory to register
 * Format: (INST_LOAD << 24) | (reg << 20) | (address & 0x0FFF)
 * Example: LOAD R0, 256 -> 0x01000100
 */
void execute_load(dez_vm_t *vm, uint32_t instruction) {
  uint32_t address = instruction & 0x0FFF;
  uint8_t reg = instruction >> 20 & 0xF;
  vm->cpu.regs[reg] = memory_read_word(&vm->memory, address);
}

/**
 * Execute STORE instruction: Store register value to memory
 * Format: (INST_STORE << 24) | (reg << 20) | (address & 0x0FFF)
 * Example: STORE R0, 256 -> 0x11000100
 */
void execute_store(dez_vm_t *vm, uint32_t instruction) {
  vm->memory.memory[instruction & 0x0FFF] =
      vm->cpu.regs[instruction >> 20 & 0xF];
}

/**
 * Execute ADD instruction: Add two values and store result
 * Format: (INST_ADD << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12) |
 * immediate Bit 11 flag: 0 = register-to-register, 1 = register-to-immediate
 * Example: ADD R1, R2, R3 -> 0x04213000
 * Example: ADD R1, R2, 5 -> 0x04210805 (bit 11 set)
 */
void execute_add(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint8_t reg3 = instruction >> 12 & 0xF;
  uint32_t immediate = instruction & 0x07FF;
  bool immediate_mode = (instruction & (1 << 11)) != 0;

  // Check if this is register-to-immediate addition
  if (immediate_mode) {
    // Register-to-immediate addition: reg1 = reg2 + immediate
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] + immediate;
  } else {
    // Register-to-register addition: reg1 = reg2 + reg3
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] + vm->cpu.regs[reg3];
  }
}

void execute_sub(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint8_t reg3 = instruction >> 12 & 0xF;
  uint32_t immediate = instruction & 0x07FF;
  bool immediate_mode = (instruction & (1 << 11)) != 0;

  // Check if this is register-to-immediate subtraction
  if (immediate_mode) {
    // Register-to-immediate subtraction
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] - immediate;
  } else {
    // Register-to-register subtraction
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] - vm->cpu.regs[reg3];
  }
}

void execute_mul(dez_vm_t *vm, uint32_t instruction) {
  vm->cpu.regs[instruction >> 20 & 0xF] =
      vm->cpu.regs[instruction >> 16 & 0xF] *
      vm->cpu.regs[instruction >> 12 & 0xF];
}

void execute_div(dez_vm_t *vm, uint32_t instruction) {
  uint32_t divisor = vm->cpu.regs[instruction >> 12 & 0xF];
  if (UNLIKELY(divisor == 0)) {
    printf("Error: Division by zero\n");
    vm->cpu.state = VM_STATE_ERROR;
    return;
  }
  vm->cpu.regs[instruction >> 20 & 0xF] =
      vm->cpu.regs[instruction >> 16 & 0xF] / divisor;
}

void execute_jmp(dez_vm_t *vm, uint32_t instruction) {
  vm->cpu.pc = instruction & 0x0FFF;
}

void execute_jz(dez_vm_t *vm, uint32_t instruction) {
  if (vm->cpu.flags & FLAG_ZERO) {
    vm->cpu.pc = instruction & 0x0FFF;
  }
  // If condition not met, let VM handle PC increment
}

void execute_jnz(dez_vm_t *vm, uint32_t instruction) {
  if (!(vm->cpu.flags & FLAG_ZERO)) {
    vm->cpu.pc = instruction & 0x0FFF;
  }
  // If condition not met, let VM handle PC increment
}

void execute_cmp(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint32_t immediate = instruction & 0x0FFF;
  uint32_t val1 = vm->cpu.regs[reg1];
  uint32_t val2;

  // Check if this is register-to-register comparison (reg2 != 0)
  if (reg2 != 0) {
    // Register-to-register comparison
    val2 = vm->cpu.regs[reg2];
  } else {
    // Register-to-immediate comparison
    val2 = immediate;
  }

  // Clear all flags first
  vm->cpu.flags = 0;

  // Set flags based on comparison
  if (val1 == val2) {
    vm->cpu.flags |= FLAG_EQUAL | FLAG_ZERO;
  } else if (val1 < val2) {
    vm->cpu.flags |= FLAG_LESS;
  } else {
    vm->cpu.flags |= FLAG_GREATER;
  }
}

void execute_sys(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint32_t immediate = instruction & 0x0FFF;

  // For now, implement basic print functionality
  if (immediate == SYS_PRINT) {
    printf("R%d = %d\n", reg1, vm->cpu.regs[reg1]);
  } else if (immediate == SYS_PRINT_CHAR) {
    printf("%c", (char)vm->cpu.regs[reg1]);
  } else if (immediate == SYS_PRINT_STR) {
    // Print string - reg1 contains address of string
    uint32_t addr = vm->cpu.regs[reg1];

    // Optimized single-pass string printing with escape sequence interpretation
    uint32_t pos = addr;
    while (pos < 0x200) { // Limit to prevent overflow
      uint8_t byte = memory_read_byte(&vm->memory, pos);
      if (byte == 0)
        break; // Null terminator

      if (byte == '\\' && pos + 1 < 0x200) {
        // Handle escape sequences
        uint8_t next_byte = memory_read_byte(&vm->memory, pos + 1);
        switch (next_byte) {
        case 'n':
          printf("\n");
          pos += 2; // Skip both characters
          continue;
        case 't':
          printf("\t");
          pos += 2;
          continue;
        case 'r':
          printf("\r");
          pos += 2;
          continue;
        case '\\':
          printf("\\");
          pos += 2;
          continue;
        case '"':
          printf("\"");
          pos += 2;
          continue;
        default:
          printf("%c", byte); // Print the backslash as-is
          pos++;
          continue;
        }
      } else {
        printf("%c", byte);
        pos++;
      }
    }
  } else if (immediate == SYS_EXIT) {
    vm->cpu.state = VM_STATE_HALTED;
    printf("Program exited with code %d\n", vm->cpu.regs[reg1]);
  } else {
    printf("Unknown system call: %d\n", immediate);
  }
}

void execute_halt(dez_vm_t *vm, uint32_t instruction) {
  (void)instruction; // Suppress unused parameter warning
  vm->cpu.state = VM_STATE_HALTED;
  printf("Program halted\n");
}

void execute_nop(dez_vm_t *vm, uint32_t instruction) {
  (void)vm;          // Suppress unused parameter warning
  (void)instruction; // Suppress unused parameter warning
}

/**
 * Execute AND instruction: Bitwise AND operation
 * Format: (INST_AND << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)
 * Example: AND R1, R2, R3 -> R1 = R2 & R3
 */
void execute_and(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint8_t reg3 = instruction >> 12 & 0xF;
  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] & vm->cpu.regs[reg3];
}

/**
 * Execute OR instruction: Bitwise OR operation
 * Format: (INST_OR << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)
 * Example: OR R1, R2, R3 -> R1 = R2 | R3
 */
void execute_or(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint8_t reg3 = instruction >> 12 & 0xF;
  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] | vm->cpu.regs[reg3];
}

/**
 * Execute XOR instruction: Bitwise XOR operation
 * Format: (INST_XOR << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)
 * Example: XOR R1, R2, R3 -> R1 = R2 ^ R3
 */
void execute_xor(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint8_t reg3 = instruction >> 12 & 0xF;
  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] ^ vm->cpu.regs[reg3];
}

/**
 * Execute NOT instruction: Bitwise NOT operation
 * Format: (INST_NOT << 24) | (reg1 << 20) | (reg2 << 16)
 * Example: NOT R1, R2 -> R1 = ~R2
 */
void execute_not(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  vm->cpu.regs[reg1] = ~vm->cpu.regs[reg2];
}

/**
 * Execute SHL instruction: Shift left operation
 * Format: (INST_SHL << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)
 * Example: SHL R1, R2, R3 -> R1 = R2 << R3
 */
void execute_shl(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint8_t reg3 = instruction >> 12 & 0xF;
  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] << vm->cpu.regs[reg3];
}

/**
 * Execute SHR instruction: Shift right operation
 * Format: (INST_SHR << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)
 * Example: SHR R1, R2, R3 -> R1 = R2 >> R3
 */
void execute_shr(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = instruction >> 20 & 0xF;
  uint8_t reg2 = instruction >> 16 & 0xF;
  uint8_t reg3 = instruction >> 12 & 0xF;
  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] >> vm->cpu.regs[reg3];
}

/**
 * Execute PUSH instruction: Push register value to stack
 * Format: (INST_PUSH << 24) | (reg << 20)
 * Example: PUSH R0 -> Push R0 to stack
 */
void execute_push(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = instruction >> 20 & 0xF;

  // Check stack bounds
  if (vm->cpu.sp < STACK_START) {
    printf("Error: Stack overflow\n");
    vm->cpu.state = VM_STATE_ERROR;
    return;
  }

  // Push value to stack
  memory_write_word(&vm->memory, vm->cpu.sp, vm->cpu.regs[reg]);
  vm->cpu.sp--;
}

/**
 * Execute POP instruction: Pop value from stack to register
 * Format: (INST_POP << 24) | (reg << 20)
 * Example: POP R0 -> Pop from stack to R0
 */
void execute_pop(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = instruction >> 20 & 0xF;

  // Check stack bounds
  if (vm->cpu.sp >= STACK_END) {
    printf("Error: Stack underflow\n");
    vm->cpu.state = VM_STATE_ERROR;
    return;
  }

  // Pop value from stack
  vm->cpu.sp++;
  vm->cpu.regs[reg] = memory_read_word(&vm->memory, vm->cpu.sp);
}

/**
 * Execute CALL instruction: Call a function (push return address and jump)
 * Format: (INST_CALL << 24) | (address & 0x0FFF)
 * Example: CALL function -> Push PC+1 and jump to function
 */
void execute_call(dez_vm_t *vm, uint32_t instruction) {
  uint32_t target_address = instruction & 0x0FFF;

  // Check stack bounds
  if (vm->cpu.sp < STACK_START) {
    printf("Error: Stack overflow during call\n");
    vm->cpu.state = VM_STATE_ERROR;
    return;
  }

  // Push return address (current PC + 1) to stack
  memory_write_word(&vm->memory, vm->cpu.sp, vm->cpu.pc + 1);
  vm->cpu.sp--;

  // Jump to target address
  vm->cpu.pc = target_address;
}

/**
 * Execute RET instruction: Return from function (pop return address and jump)
 * Format: (INST_RET << 24)
 * Example: RET -> Pop return address and jump back
 */
void execute_ret(dez_vm_t *vm, uint32_t instruction) {
  (void)instruction; // Suppress unused parameter warning

  // Check stack bounds
  if (vm->cpu.sp >= STACK_END) {
    printf("Error: Stack underflow during return\n");
    vm->cpu.state = VM_STATE_ERROR;
    return;
  }

  // Pop return address from stack
  vm->cpu.sp++;
  uint32_t return_address = memory_read_word(&vm->memory, vm->cpu.sp);

  // Jump back to return address
  vm->cpu.pc = return_address;
}

/**
 * Execute JL instruction: Jump if less
 * Format: (INST_JL << 24) | (address & 0x0FFF)
 * Example: JL target -> Jump if first operand < second operand
 */
void execute_jl(dez_vm_t *vm, uint32_t instruction) {
  if (vm->cpu.flags & FLAG_LESS) {
    vm->cpu.pc = instruction & 0x0FFF;
  }
  // If condition not met, let VM handle PC increment
}

/**
 * Execute JG instruction: Jump if greater
 * Format: (INST_JG << 24) | (address & 0x0FFF)
 * Example: JG target -> Jump if first operand > second operand
 */
void execute_jg(dez_vm_t *vm, uint32_t instruction) {
  if (vm->cpu.flags & FLAG_GREATER) {
    vm->cpu.pc = instruction & 0x0FFF;
  }
  // If condition not met, let VM handle PC increment
}

/**
 * Execute JLE instruction: Jump if less or equal
 * Format: (INST_JLE << 24) | (address & 0x0FFF)
 * Example: JLE target -> Jump if first operand <= second operand
 */
void execute_jle(dez_vm_t *vm, uint32_t instruction) {
  if (vm->cpu.flags & (FLAG_LESS | FLAG_EQUAL)) {
    vm->cpu.pc = instruction & 0x0FFF;
  }
  // If condition not met, let VM handle PC increment
}

/**
 * Execute JGE instruction: Jump if greater or equal
 * Format: (INST_JGE << 24) | (address & 0x0FFF)
 * Example: JGE target -> Jump if first operand >= second operand
 */
void execute_jge(dez_vm_t *vm, uint32_t instruction) {
  if (vm->cpu.flags & (FLAG_GREATER | FLAG_EQUAL)) {
    vm->cpu.pc = instruction & 0x0FFF;
  }
  // If condition not met, let VM handle PC increment
}

void execute_unknown(dez_vm_t *vm, uint32_t instruction) {
  printf("Error: Unknown instruction 0x%02X at PC %04X\n", instruction >> 24,
         vm->cpu.pc);
  vm->cpu.state = VM_STATE_ERROR;
}

// Instruction table - ordered by frequency for better branch prediction
static const instruction_info_t instruction_table[256] = {
    [INST_MOV] = {execute_mov, 1, false, false, "MOV"},
    [INST_LOAD] = {execute_load, 1, true, false, "LOAD"},
    [INST_ADD] = {execute_add, 1, false, false, "ADD"},
    [INST_STORE] = {execute_store, 1, true, false, "STORE"},
    [INST_SUB] = {execute_sub, 1, false, false, "SUB"},
    [INST_MUL] = {execute_mul, 1, false, false, "MUL"},
    [INST_DIV] = {execute_div, 1, false, false, "DIV"},
    [INST_JMP] = {execute_jmp, 0, false, false, "JMP"},
    [INST_JZ] = {execute_jz, 1, false, false, "JZ"},
    [INST_JNZ] = {execute_jnz, 1, false, false, "JNZ"},
    [INST_JL] = {execute_jl, 1, false, false, "JL"},
    [INST_JG] = {execute_jg, 1, false, false, "JG"},
    [INST_JLE] = {execute_jle, 1, false, false, "JLE"},
    [INST_JGE] = {execute_jge, 1, false, false, "JGE"},
    [INST_PUSH] = {execute_push, 1, false, false, "PUSH"},
    [INST_POP] = {execute_pop, 1, false, false, "POP"},
    [INST_CMP] = {execute_cmp, 1, false, false, "CMP"},
    [INST_SYS] = {execute_sys, 1, false, false, "SYS"},
    [INST_AND] = {execute_and, 1, false, false, "AND"},
    [INST_OR] = {execute_or, 1, false, false, "OR"},
    [INST_XOR] = {execute_xor, 1, false, false, "XOR"},
    [INST_NOT] = {execute_not, 1, false, false, "NOT"},
    [INST_SHL] = {execute_shl, 1, false, false, "SHL"},
    [INST_SHR] = {execute_shr, 1, false, false, "SHR"},
    [INST_CALL] = {execute_call, 0, false, false, "CALL"},
    [INST_RET] = {execute_ret, 0, false, false, "RET"},
    [INST_HALT] = {execute_halt, 0, false, false, "HALT"},
    [INST_NOP] = {execute_nop, 1, false, false, "NOP"},
};

// Static unknown instruction info to avoid stack return warning
static const instruction_info_t unknown_instruction = {execute_unknown, 0,
                                                       false, true, "UNKNOWN"};

// Get instruction info
const instruction_info_t *get_instruction_info(uint8_t opcode) {
  if (instruction_table[opcode].execute != NULL) {
    return &instruction_table[opcode];
  }
  return &unknown_instruction;
}