/**
 * @file dez_instruction_table.c
 * @brief DEZ VM Instruction Table Implementation
 *
 * This file implements the instruction execution system, including
 * all instruction execution functions and the instruction table.
 */

#include "dez_instruction_table.h"
#include "../include/dez_vm_types.h"
#include "dez_memory.h"
#include "dez_vm.h"
#include <stdio.h>

// Instruction execution functions
// Each function takes a VM pointer and a 32-bit instruction word
// The instruction format is: (opcode << 24) | (operands...)

/**
 * @brief Execute MOV instruction: Load immediate value into register
 *
 * Format: (DEZ_INST_MOV << 24) | (reg << 20) | (immediate & DEZ_IMMEDIATE_MASK)
 * Example: MOV R0, 42 -> 0x1000002A
 *
 * @param vm Pointer to VM instance
 * @param instruction 32-bit instruction word
 */
void execute_mov(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = DEZ_DECODE_REG1(instruction);
  uint32_t immediate = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_REGISTER(reg, "MOV");
  vm->cpu.regs[reg] = immediate;
}

/**
 * @brief Execute LOAD instruction: Load value from memory to register
 *
 * Format: (DEZ_INST_LOAD << 24) | (reg << 20) | (address & DEZ_IMMEDIATE_MASK)
 * Example: LOAD R0, 256 -> 0x01000100
 *
 * @param vm Pointer to VM instance
 * @param instruction 32-bit instruction word
 */
void execute_load(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = DEZ_DECODE_REG1(instruction);
  uint32_t address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_REGISTER(reg, "LOAD");
  DEZ_VALIDATE_MEMORY_ADDRESS(address, "LOAD");
  vm->cpu.regs[reg] = memory_read_word(&vm->memory, address);
}

/**
 * @brief Execute STORE instruction: Store register value to memory
 *
 * Format: (DEZ_INST_STORE << 24) | (reg << 20) | (address & DEZ_IMMEDIATE_MASK)
 * Example: STORE R0, 256 -> 0x11000100
 *
 * @param vm Pointer to VM instance
 * @param instruction 32-bit instruction word
 */
void execute_store(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = DEZ_DECODE_REG1(instruction);
  uint32_t address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_REGISTER(reg, "STORE");
  DEZ_VALIDATE_MEMORY_ADDRESS(address, "STORE");
  vm->memory.memory[address] = vm->cpu.regs[reg];
}

/**
 * @brief Execute ADD instruction: Add two values and store result
 *
 * Format: (DEZ_INST_ADD << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12) |
 * immediate Bit 11 flag: 0 = register-to-register, 1 = register-to-immediate
 * Example: ADD R1, R2, R3 -> 0x04213000
 * Example: ADD R1, R2, 5 -> 0x04210805 (bit 11 set)
 *
 * @param vm Pointer to VM instance
 * @param instruction 32-bit instruction word
 */
void execute_add(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint8_t reg3 = DEZ_DECODE_REG3(instruction);
  uint32_t immediate = DEZ_DECODE_EXT_IMMEDIATE(instruction);
  bool immediate_mode = DEZ_IS_IMMEDIATE_MODE(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "ADD");
  DEZ_VALIDATE_REGISTER(reg2, "ADD");
  if (!immediate_mode) {
    DEZ_VALIDATE_REGISTER(reg3, "ADD");
  }

  // Check if this is register-to-immediate addition
  if (immediate_mode) {
    // Register-to-immediate addition: reg1 = reg2 + immediate
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] + immediate;
  } else {
    // Register-to-register addition: reg1 = reg2 + reg3
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] + vm->cpu.regs[reg3];
  }
}

/**
 * @brief Execute SUB instruction: Subtract two values and store result
 *
 * Format: (DEZ_INST_SUB << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12) |
 * immediate Bit 11 flag: 0 = register-to-register, 1 = register-to-immediate
 * Example: SUB R1, R2, R3 -> 0x05213000
 * Example: SUB R1, R2, 5 -> 0x05210805 (bit 11 set)
 *
 * @param vm Pointer to VM instance
 * @param instruction 32-bit instruction word
 */
void execute_sub(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint8_t reg3 = DEZ_DECODE_REG3(instruction);
  uint32_t immediate = DEZ_DECODE_EXT_IMMEDIATE(instruction);
  bool immediate_mode = DEZ_IS_IMMEDIATE_MODE(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "SUB");
  DEZ_VALIDATE_REGISTER(reg2, "SUB");
  if (!immediate_mode) {
    DEZ_VALIDATE_REGISTER(reg3, "SUB");
  }

  // Check if this is register-to-immediate subtraction
  if (immediate_mode) {
    // Register-to-immediate subtraction: reg1 = reg2 - immediate
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] - immediate;
  } else {
    // Register-to-register subtraction: reg1 = reg2 - reg3
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] - vm->cpu.regs[reg3];
  }
}

/**
 * @brief Execute MUL instruction: Multiply two values and store result
 *
 * Format: (DEZ_INST_MUL << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12) |
 * immediate Bit 11 flag: 0 = register-to-register, 1 = register-to-immediate
 * Example: MUL R1, R2, R3 -> 0x06213000
 * Example: MUL R1, R2, 5 -> 0x06210805 (bit 11 set)
 *
 * @param vm Pointer to VM instance
 * @param instruction 32-bit instruction word
 */
void execute_mul(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint8_t reg3 = DEZ_DECODE_REG3(instruction);
  uint32_t immediate = DEZ_DECODE_EXT_IMMEDIATE(instruction);
  bool immediate_mode = DEZ_IS_IMMEDIATE_MODE(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "MUL");
  DEZ_VALIDATE_REGISTER(reg2, "MUL");
  if (!immediate_mode) {
    DEZ_VALIDATE_REGISTER(reg3, "MUL");
  }

  // Check if this is register-to-immediate multiplication
  if (immediate_mode) {
    // Register-to-immediate multiplication: reg1 = reg2 * immediate
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] * immediate;
  } else {
    // Register-to-register multiplication: reg1 = reg2 * reg3
    vm->cpu.regs[reg1] = vm->cpu.regs[reg2] * vm->cpu.regs[reg3];
  }
}

/**
 * @brief Execute DIV instruction: Divide two values and store result
 *
 * Format: (DEZ_INST_DIV << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12) |
 * immediate Bit 11 flag: 0 = register-to-register, 1 = register-to-immediate
 * Example: DIV R1, R2, R3 -> 0x07213000
 * Example: DIV R1, R2, 5 -> 0x07210805 (bit 11 set)
 *
 * @param vm Pointer to VM instance
 * @param instruction 32-bit instruction word
 */
void execute_div(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint8_t reg3 = DEZ_DECODE_REG3(instruction);
  uint32_t immediate = DEZ_DECODE_EXT_IMMEDIATE(instruction);
  bool immediate_mode = DEZ_IS_IMMEDIATE_MODE(instruction);
  uint32_t divisor;

  DEZ_VALIDATE_REGISTER(reg1, "DIV");
  DEZ_VALIDATE_REGISTER(reg2, "DIV");
  if (!immediate_mode) {
    DEZ_VALIDATE_REGISTER(reg3, "DIV");
  }

  // Check if this is register-to-immediate division
  if (immediate_mode) {
    // Register-to-immediate division: reg1 = reg2 / immediate
    divisor = immediate;
  } else {
    // Register-to-register division: reg1 = reg2 / reg3
    divisor = vm->cpu.regs[reg3];
  }

  if (UNLIKELY(divisor == 0)) {
    printf("Error: Division by zero\n");
    vm->cpu.state = DEZ_VM_STATE_ERROR;
    return;
  }

  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] / divisor;
}

void execute_jmp(dez_vm_t *vm, uint32_t instruction) {
  uint32_t target_address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_MEMORY_ADDRESS(target_address, "JMP");
  vm->cpu.pc = target_address;
}

void execute_jz(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = DEZ_DECODE_REG1(instruction);
  uint32_t target_address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_REGISTER(reg, "JZ");
  DEZ_VALIDATE_MEMORY_ADDRESS(target_address, "JZ");

  if (vm->cpu.flags & DEZ_FLAG_ZERO) {
    vm->cpu.pc = target_address;
  }
  // If condition not met, let VM handle PC increment
}

void execute_jnz(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = DEZ_DECODE_REG1(instruction);
  uint32_t target_address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_REGISTER(reg, "JNZ");
  DEZ_VALIDATE_MEMORY_ADDRESS(target_address, "JNZ");

  if (!(vm->cpu.flags & DEZ_FLAG_ZERO)) {
    vm->cpu.pc = target_address;
  }
  // If condition not met, let VM handle PC increment
}

void execute_cmp(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint32_t immediate = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "CMP");
  DEZ_VALIDATE_REGISTER(reg2, "CMP");
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
    vm->cpu.flags |= DEZ_FLAG_EQUAL | DEZ_FLAG_ZERO;
  } else if (val1 < val2) {
    vm->cpu.flags |= DEZ_FLAG_LESS;
  } else {
    vm->cpu.flags |= DEZ_FLAG_GREATER;
  }
}

void execute_sys(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint32_t immediate = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "SYS");

  // For now, implement basic print functionality
  if (immediate == DEZ_SYS_PRINT) {
    printf("R%d = %d\n", reg1, vm->cpu.regs[reg1]);
  } else if (immediate == DEZ_SYS_PRINT_CHAR) {
    printf("%c", (char)vm->cpu.regs[reg1]);
  } else if (immediate == DEZ_SYS_PRINT_STR) {
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
  } else if (immediate == DEZ_SYS_EXIT) {
    vm->cpu.state = DEZ_VM_STATE_HALTED;
    printf("Program exited with code %d\n", vm->cpu.regs[reg1]);
  } else {
    printf("Unknown system call: %d\n", immediate);
  }
}

void execute_halt(dez_vm_t *vm, uint32_t instruction) {
  (void)instruction; // Suppress unused parameter warning
  vm->cpu.state = DEZ_VM_STATE_HALTED;
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
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint8_t reg3 = DEZ_DECODE_REG3(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "AND");
  DEZ_VALIDATE_REGISTER(reg2, "AND");
  DEZ_VALIDATE_REGISTER(reg3, "AND");

  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] & vm->cpu.regs[reg3];
}

/**
 * Execute OR instruction: Bitwise OR operation
 * Format: (INST_OR << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)
 * Example: OR R1, R2, R3 -> R1 = R2 | R3
 */
void execute_or(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint8_t reg3 = DEZ_DECODE_REG3(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "OR");
  DEZ_VALIDATE_REGISTER(reg2, "OR");
  DEZ_VALIDATE_REGISTER(reg3, "OR");

  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] | vm->cpu.regs[reg3];
}

/**
 * Execute XOR instruction: Bitwise XOR operation
 * Format: (INST_XOR << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)
 * Example: XOR R1, R2, R3 -> R1 = R2 ^ R3
 */
void execute_xor(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint8_t reg3 = DEZ_DECODE_REG3(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "XOR");
  DEZ_VALIDATE_REGISTER(reg2, "XOR");
  DEZ_VALIDATE_REGISTER(reg3, "XOR");

  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] ^ vm->cpu.regs[reg3];
}

/**
 * Execute NOT instruction: Bitwise NOT operation
 * Format: (INST_NOT << 24) | (reg1 << 20) | (reg2 << 16)
 * Example: NOT R1, R2 -> R1 = ~R2
 */
void execute_not(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "NOT");
  DEZ_VALIDATE_REGISTER(reg2, "NOT");

  vm->cpu.regs[reg1] = ~vm->cpu.regs[reg2];
}

/**
 * Execute SHL instruction: Shift left operation
 * Format: (INST_SHL << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)
 * Example: SHL R1, R2, R3 -> R1 = R2 << R3
 */
void execute_shl(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint8_t reg3 = DEZ_DECODE_REG3(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "SHL");
  DEZ_VALIDATE_REGISTER(reg2, "SHL");
  DEZ_VALIDATE_REGISTER(reg3, "SHL");

  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] << vm->cpu.regs[reg3];
}

/**
 * Execute SHR instruction: Shift right operation
 * Format: (INST_SHR << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)
 * Example: SHR R1, R2, R3 -> R1 = R2 >> R3
 */
void execute_shr(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg1 = DEZ_DECODE_REG1(instruction);
  uint8_t reg2 = DEZ_DECODE_REG2(instruction);
  uint8_t reg3 = DEZ_DECODE_REG3(instruction);

  DEZ_VALIDATE_REGISTER(reg1, "SHR");
  DEZ_VALIDATE_REGISTER(reg2, "SHR");
  DEZ_VALIDATE_REGISTER(reg3, "SHR");

  vm->cpu.regs[reg1] = vm->cpu.regs[reg2] >> vm->cpu.regs[reg3];
}

/**
 * @brief Execute INC instruction: Increment register value by 1
 *
 * Format: (DEZ_INST_INC << 24) | (reg << 20)
 * Example: INC R0 -> R0 = R0 + 1
 *
 * @param vm Pointer to VM instance
 * @param instruction 32-bit instruction word
 */
void execute_inc(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = DEZ_DECODE_REG1(instruction);

  DEZ_VALIDATE_REGISTER(reg, "INC");

  vm->cpu.regs[reg]++;
}

/**
 * @brief Execute DEC instruction: Decrement register value by 1
 *
 * Format: (DEZ_INST_DEC << 24) | (reg << 20)
 * Example: DEC R0 -> R0 = R0 - 1
 *
 * @param vm Pointer to VM instance
 * @param instruction 32-bit instruction word
 */
void execute_dec(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = DEZ_DECODE_REG1(instruction);

  DEZ_VALIDATE_REGISTER(reg, "DEC");

  vm->cpu.regs[reg]--;
}

/**
 * Execute PUSH instruction: Push register value to stack
 * Format: (INST_PUSH << 24) | (reg << 20)
 * Example: PUSH R0 -> Push R0 to stack
 */
void execute_push(dez_vm_t *vm, uint32_t instruction) {
  uint8_t reg = DEZ_DECODE_REG1(instruction);

  DEZ_VALIDATE_REGISTER(reg, "PUSH");
  DEZ_VALIDATE_STACK_OVERFLOW("PUSH");

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
  uint8_t reg = DEZ_DECODE_REG1(instruction);

  DEZ_VALIDATE_REGISTER(reg, "POP");
  DEZ_VALIDATE_STACK_UNDERFLOW("POP");

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
  uint32_t target_address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_MEMORY_ADDRESS(target_address, "CALL");
  DEZ_VALIDATE_STACK_OVERFLOW("CALL");

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

  DEZ_VALIDATE_STACK_UNDERFLOW("RET");

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
  uint32_t target_address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_MEMORY_ADDRESS(target_address, "JL");

  if (vm->cpu.flags & DEZ_FLAG_LESS) {
    vm->cpu.pc = target_address;
  }
  // If condition not met, let VM handle PC increment
}

/**
 * Execute JG instruction: Jump if greater
 * Format: (DEZ_INST_JG << 24) | (address & 0x0FFF)
 * Example: JG target -> Jump if first operand > second operand
 */
void execute_jg(dez_vm_t *vm, uint32_t instruction) {
  uint32_t target_address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_MEMORY_ADDRESS(target_address, "JG");

  if (vm->cpu.flags & DEZ_FLAG_GREATER) {
    vm->cpu.pc = target_address;
  }
  // If condition not met, let VM handle PC increment
}

/**
 * Execute JLE instruction: Jump if less or equal
 * Format: (DEZ_INST_JLE << 24) | (address & 0x0FFF)
 * Example: JLE target -> Jump if first operand <= second operand
 */
void execute_jle(dez_vm_t *vm, uint32_t instruction) {
  uint32_t target_address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_MEMORY_ADDRESS(target_address, "JLE");

  if (vm->cpu.flags & (DEZ_FLAG_LESS | DEZ_FLAG_EQUAL)) {
    vm->cpu.pc = target_address;
  }
  // If condition not met, let VM handle PC increment
}

/**
 * Execute JGE instruction: Jump if greater or equal
 * Format: (DEZ_INST_JGE << 24) | (address & 0x0FFF)
 * Example: JGE target -> Jump if first operand >= second operand
 */
void execute_jge(dez_vm_t *vm, uint32_t instruction) {
  uint32_t target_address = DEZ_DECODE_IMMEDIATE(instruction);

  DEZ_VALIDATE_MEMORY_ADDRESS(target_address, "JGE");

  if (vm->cpu.flags & (DEZ_FLAG_GREATER | DEZ_FLAG_EQUAL)) {
    vm->cpu.pc = target_address;
  }
  // If condition not met, let VM handle PC increment
}

void execute_unknown(dez_vm_t *vm, uint32_t instruction) {
  printf("Error: Unknown instruction 0x%02X at PC %04X\n", instruction >> 24,
         vm->cpu.pc);
  vm->cpu.state = DEZ_VM_STATE_ERROR;
}

// ============================================================================
// INSTRUCTION TABLE
// ============================================================================

// Instruction table - ordered by frequency for better branch prediction
static const instruction_info_t instruction_table[256] = {
    [DEZ_INST_MOV] = {execute_mov, 1, false, false, "MOV"},
    [DEZ_INST_LOAD] = {execute_load, 1, true, false, "LOAD"},
    [DEZ_INST_ADD] = {execute_add, 1, false, false, "ADD"},
    [DEZ_INST_STORE] = {execute_store, 1, true, false, "STORE"},
    [DEZ_INST_SUB] = {execute_sub, 1, false, false, "SUB"},
    [DEZ_INST_MUL] = {execute_mul, 1, false, false, "MUL"},
    [DEZ_INST_DIV] = {execute_div, 1, false, false, "DIV"},
    [DEZ_INST_JMP] = {execute_jmp, 0, false, false, "JMP"},
    [DEZ_INST_JZ] = {execute_jz, 1, false, false, "JZ"},
    [DEZ_INST_JNZ] = {execute_jnz, 1, false, false, "JNZ"},
    [DEZ_INST_JL] = {execute_jl, 1, false, false, "JL"},
    [DEZ_INST_JG] = {execute_jg, 1, false, false, "JG"},
    [DEZ_INST_JLE] = {execute_jle, 1, false, false, "JLE"},
    [DEZ_INST_JGE] = {execute_jge, 1, false, false, "JGE"},
    [DEZ_INST_PUSH] = {execute_push, 1, false, false, "PUSH"},
    [DEZ_INST_POP] = {execute_pop, 1, false, false, "POP"},
    [DEZ_INST_CMP] = {execute_cmp, 1, false, false, "CMP"},
    [DEZ_INST_SYS] = {execute_sys, 1, false, false, "SYS"},
    [DEZ_INST_AND] = {execute_and, 1, false, false, "AND"},
    [DEZ_INST_OR] = {execute_or, 1, false, false, "OR"},
    [DEZ_INST_XOR] = {execute_xor, 1, false, false, "XOR"},
    [DEZ_INST_NOT] = {execute_not, 1, false, false, "NOT"},
    [DEZ_INST_SHL] = {execute_shl, 1, false, false, "SHL"},
    [DEZ_INST_SHR] = {execute_shr, 1, false, false, "SHR"},
    [DEZ_INST_INC] = {execute_inc, 1, false, false, "INC"},
    [DEZ_INST_DEC] = {execute_dec, 1, false, false, "DEC"},
    [DEZ_INST_CALL] = {execute_call, 0, false, false, "CALL"},
    [DEZ_INST_RET] = {execute_ret, 0, false, false, "RET"},
    [DEZ_INST_HALT] = {execute_halt, 0, false, false, "HALT"},
    [DEZ_INST_NOP] = {execute_nop, 1, false, false, "NOP"},
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