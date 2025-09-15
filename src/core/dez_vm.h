#ifndef DEZ_VM_H
#define DEZ_VM_H

#include <stdint.h>

#include "../include/dez_vm_types.h"
#include "dez_memory.h"

typedef struct {
    uint32_t regs[16];
    uint32_t pc;
    uint32_t sp;
    uint32_t flags;
    dez_vm_state_t state;
} cpu_t;

typedef struct {
    cpu_t cpu;
    dez_memory_t memory;
    uint32_t program_size;
    bool debug_mode;
} dez_vm_t;

void dez_vm_init(dez_vm_t *vm);

void dez_vm_load_program(dez_vm_t *vm, const char *filename);

void dez_vm_step(dez_vm_t *vm);

void dez_vm_run(dez_vm_t *vm);

void dez_vm_print_state(dez_vm_t *vm);

#endif // DEZ_VM_H