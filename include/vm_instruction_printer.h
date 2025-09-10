#ifndef VM_INSTRUCTION_PRINTER_H
#define VM_INSTRUCTION_PRINTER_H

#include <stddef.h>
#include <stdint.h>

// Instruction printing modes
typedef enum {
  VM_PRINT_MODE_SIMPLE,   // Simple format: "MOV R1, R2"
  VM_PRINT_MODE_DETAILED, // Detailed format with addresses
  VM_PRINT_MODE_OBJDUMP,  // Objdump-style format
  VM_PRINT_MODE_STEP      // Step-by-step execution format
} vm_print_mode_t;

// Instruction printing context
typedef struct {
  vm_print_mode_t mode;
  int show_address;
  int show_raw_bytes;
  uint32_t base_address;
  int instruction_number;
} vm_print_context_t;

// Function prototypes
void vm_print_instruction(const void *inst, const vm_print_context_t *context);
void vm_print_instruction_simple(const void *inst);
void vm_print_instruction_detailed(const void *inst, uint32_t address);
void vm_print_instruction_objdump(const void *inst, uint32_t address,
                                  const uint8_t *raw_bytes);
void vm_print_instruction_step(const void *inst, int instruction_num);

// Helper functions
const char *vm_get_instruction_mnemonic(int type);
const char *vm_get_operand_type_name(int type);
void vm_print_operand(const void *op, int operand_index);
void vm_print_raw_bytes(const uint8_t *bytes, size_t count);

#endif // VM_INSTRUCTION_PRINTER_H
