#ifndef DEZ_MEMORY_H
#define DEZ_MEMORY_H

#include <stdbool.h>
#include <stdint.h>

// Memory size constants for 16KB VM
#define MEMORY_SIZE_BYTES 16384
#define MEMORY_SIZE_WORDS 4096

// Segment boundaries for 16KB (4096 words)
#define CODE_START 0x0000
#define CODE_END 0x03FF // First 1KB (256 words)
#define DATA_START 0x0400
#define DATA_END 0x07FF // Second 1KB (256 words)
#define STACK_START 0x0800
#define STACK_END 0x0FFF // Last 2KB (512 words)

// Segment sizes
#define CODE_SIZE 0x0400  // 1KB (256 words)
#define DATA_SIZE 0x0400  // 1KB (256 words)
#define STACK_SIZE 0x0800 // 2KB (512 words)

typedef struct {
  uint32_t memory[MEMORY_SIZE_WORDS];

  // Memory segment boundaries
  uint32_t code_start;
  uint32_t code_end;
  uint32_t data_start;
  uint32_t data_end;
  uint32_t stack_start;
  uint32_t stack_end;

  // Memory protection flags
  bool code_readonly;
  bool data_writable;
  bool stack_writable;

  // Memory statistics (for debugging)
  uint32_t code_usage;
  uint32_t data_usage;
  uint32_t stack_usage;
  uint32_t max_stack_usage;

  // Memory access tracking (optional)
  uint32_t access_count;
  uint32_t read_count;
  uint32_t write_count;
} dez_memory_t;

void memory_init(dez_memory_t *mem);

uint32_t memory_read_word(dez_memory_t *mem, uint32_t address);
int memory_write_word(dez_memory_t *mem, uint32_t address, uint32_t value);
uint8_t memory_read_byte(dez_memory_t *mem, uint32_t address);
int memory_write_byte(dez_memory_t *mem, uint32_t address, uint8_t value);

// Additional utility functions
void memory_print_stats(dez_memory_t *mem);
uint32_t memory_get_usage(dez_memory_t *mem, uint32_t segment);
void memory_set_protection(dez_memory_t *mem, uint32_t segment, bool readonly);

// Address validation functions
bool memory_is_valid_address_public(dez_memory_t *mem, uint32_t address);
bool memory_is_code_segment_public(dez_memory_t *mem, uint32_t address);
bool memory_is_data_segment_public(dez_memory_t *mem, uint32_t address);
bool memory_is_stack_segment_public(dez_memory_t *mem, uint32_t address);
bool memory_can_write_public(dez_memory_t *mem, uint32_t address);

// Fast path memory access (no validation, no statistics)
uint32_t memory_read_word_fast(dez_memory_t *mem, uint32_t address);
int memory_write_word_fast(dez_memory_t *mem, uint32_t address, uint32_t value);

// Compiler optimization hints
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif // DEZ_MEMORY_H