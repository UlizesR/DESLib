/**
 * @file dez_vm.h
 * @brief DEZ Virtual Machine Core Interface
 * 
 * This file defines the main VM structures and public API functions
 * for the DEZ virtual machine.
 */

#ifndef DEZ_VM_H
#define DEZ_VM_H

#include <stdint.h>

#include "../include/dez_vm_types.h"
#include "dez_memory.h"

/**
 * @brief CPU state structure
 * 
 * Contains all CPU registers, program counter, stack pointer,
 * status flags, and execution state.
 */
typedef struct {
    uint32_t regs[16];      ///< 16 general-purpose 32-bit registers (R0-R15)
    uint32_t pc;            ///< Program counter (instruction pointer)
    uint32_t sp;            ///< Stack pointer
    uint32_t flags;         ///< Status flags (zero, carry, overflow)
    dez_vm_state_t state;   ///< Current VM execution state
} cpu_t;

/**
 * @brief Main VM structure
 * 
 * Contains the CPU, memory system, program metadata, and debug settings.
 */
typedef struct {
    cpu_t cpu;              ///< CPU state and registers
    dez_memory_t memory;    ///< Memory management system
    uint32_t program_size;  ///< Number of instructions in loaded program
    bool debug_mode;        ///< Debug output enabled/disabled
} dez_vm_t;

/**
 * @brief Initialize a VM instance
 * @param vm Pointer to VM structure to initialize
 */
void dez_vm_init(dez_vm_t *vm);

/**
 * @brief Load a binary program into the VM
 * @param vm Pointer to VM structure
 * @param filename Path to binary program file
 */
void dez_vm_load_program(dez_vm_t *vm, const char *filename);

/**
 * @brief Execute a single instruction
 * @param vm Pointer to VM structure
 */
void dez_vm_step(dez_vm_t *vm);

/**
 * @brief Run the VM until halt or error
 * @param vm Pointer to VM structure
 */
void dez_vm_run(dez_vm_t *vm);

/**
 * @brief Print current VM state for debugging
 * @param vm Pointer to VM structure
 */
void dez_vm_print_state(dez_vm_t *vm);

#endif // DEZ_VM_H