#include "dez_memory.h"
#include <stdio.h>
#include <string.h>

// Initialize the memory system
void memory_init(dez_memory_t *mem) {
  if (!mem)
    return;

  // Clear all memory
  memset(mem->memory, 0, sizeof(mem->memory));

  // Set up segment boundaries
  mem->code_start = CODE_START;
  mem->code_end = CODE_END;
  mem->data_start = DATA_START;
  mem->data_end = DATA_END;
  mem->stack_start = STACK_START;
  mem->stack_end = STACK_END;

  // Set up memory protection flags
  mem->code_readonly = true;  // Code segment is read-only
  mem->data_writable = true;  // Data segment is writable
  mem->stack_writable = true; // Stack segment is writable

  // Initialize statistics
  mem->code_usage = 0;
  mem->data_usage = 0;
  mem->stack_usage = 0;
  mem->max_stack_usage = 0;

  // Initialize access tracking
  mem->access_count = 0;
  mem->read_count = 0;
  mem->write_count = 0;
}

// Check if an address is within valid bounds
static bool memory_is_valid_address(dez_memory_t *mem, uint32_t address) {
  return (address < MEMORY_SIZE_WORDS);
}

// Check if an address is in the code segment
static bool memory_is_code_segment(dez_memory_t *mem, uint32_t address) {
  return (address >= mem->code_start && address <= mem->code_end);
}

// Check if an address is in the data segment
static bool memory_is_data_segment(dez_memory_t *mem, uint32_t address) {
  return (address >= mem->data_start && address <= mem->data_end);
}

// Check if an address is in the stack segment
static bool memory_is_stack_segment(dez_memory_t *mem, uint32_t address) {
  return (address >= mem->stack_start && address <= mem->stack_end);
}

// Check if an address can be written to
static bool memory_can_write(dez_memory_t *mem, uint32_t address) {
  if (memory_is_code_segment(mem, address)) {
    return !mem->code_readonly;
  } else if (memory_is_data_segment(mem, address)) {
    return mem->data_writable;
  } else if (memory_is_stack_segment(mem, address)) {
    return mem->stack_writable;
  }
  return false;
}

// Fast path memory read (no validation, no statistics)
uint32_t memory_read_word_fast(dez_memory_t *mem, uint32_t address) {
  return mem->memory[address];
}

// Fast path memory write (no validation, no statistics)
int memory_write_word_fast(dez_memory_t *mem, uint32_t address,
                           uint32_t value) {
  mem->memory[address] = value;
  return 0;
}

// Read a 32-bit word from memory
uint32_t memory_read_word(dez_memory_t *mem, uint32_t address) {
  if (UNLIKELY(!mem)) {
    printf("Error: NULL memory pointer\n");
    return 0;
  }

  // Fast path for common case
  if (LIKELY(address < MEMORY_SIZE_WORDS)) {
    // Update statistics only in debug mode
#ifdef DEBUG_MEMORY_STATS
    mem->access_count++;
    mem->read_count++;

    // Update usage statistics
    if (memory_is_code_segment(mem, address)) {
      mem->code_usage++;
    } else if (memory_is_data_segment(mem, address)) {
      mem->data_usage++;
    } else if (memory_is_stack_segment(mem, address)) {
      mem->stack_usage++;
      if (mem->stack_usage > mem->max_stack_usage) {
        mem->max_stack_usage = mem->stack_usage;
      }
    }
#endif
    return mem->memory[address];
  }

  // Slow path for validation
  if (!memory_is_valid_address(mem, address)) {
    printf("Error: Memory read out of bounds at address 0x%04X\n", address);
    return 0;
  }

  // Update statistics
  mem->access_count++;
  mem->read_count++;

  // Update usage statistics
  if (memory_is_code_segment(mem, address)) {
    mem->code_usage++;
  } else if (memory_is_data_segment(mem, address)) {
    mem->data_usage++;
  } else if (memory_is_stack_segment(mem, address)) {
    mem->stack_usage++;
    if (mem->stack_usage > mem->max_stack_usage) {
      mem->max_stack_usage = mem->stack_usage;
    }
  }

  return mem->memory[address >> 2];
}

// Write a 32-bit word to memory
int memory_write_word(dez_memory_t *mem, uint32_t address, uint32_t value) {
  if (UNLIKELY(!mem)) {
    printf("Error: NULL memory pointer\n");
    return -1;
  }

  // Fast path for common case
  if (LIKELY(address < MEMORY_SIZE_WORDS)) {
    // Check write permissions only for code segment
    if (UNLIKELY(memory_is_code_segment(mem, address) && mem->code_readonly)) {
      printf("Error: Write to read-only memory at address 0x%04X\n", address);
      return -1;
    }

    // Update statistics only in debug mode
#ifdef DEBUG_MEMORY_STATS
    mem->access_count++;
    mem->write_count++;

    // Update usage statistics
    if (memory_is_code_segment(mem, address)) {
      mem->code_usage++;
    } else if (memory_is_data_segment(mem, address)) {
      mem->data_usage++;
    } else if (memory_is_stack_segment(mem, address)) {
      mem->stack_usage++;
      if (mem->stack_usage > mem->max_stack_usage) {
        mem->max_stack_usage = mem->stack_usage;
      }
    }
#endif
    mem->memory[address] = value;
    return 0;
  }

  // Slow path for validation
  if (!memory_is_valid_address(mem, address)) {
    printf("Error: Memory write out of bounds at address 0x%04X\n", address);
    return -1;
  }

  // Check write permissions
  if (!memory_can_write(mem, address)) {
    printf("Error: Write to read-only memory at address 0x%04X\n", address);
    return -1;
  }

  // Update statistics
  mem->access_count++;
  mem->write_count++;

  // Update usage statistics
  if (memory_is_code_segment(mem, address)) {
    mem->code_usage++;
  } else if (memory_is_data_segment(mem, address)) {
    mem->data_usage++;
  } else if (memory_is_stack_segment(mem, address)) {
    mem->stack_usage++;
    if (mem->stack_usage > mem->max_stack_usage) {
      mem->max_stack_usage = mem->stack_usage;
    }
  }

  // Perform the write
  mem->memory[address >> 2] = value;
  return 0; // Success
}

// Read a single byte from memory
uint8_t memory_read_byte(dez_memory_t *mem, uint32_t address) {
  if (UNLIKELY(!mem)) {
    printf("Error: NULL memory pointer\n");
    return 0;
  }

  // Convert byte address to word address and byte offset
  uint32_t word_addr = address >> 2;  // Optimized division by 4
  uint32_t byte_offset = address & 3; // Optimized modulo 4

  if (UNLIKELY(word_addr >= MEMORY_SIZE_WORDS)) {
    printf("Error: Memory read out of bounds at address 0x%04X\n", address);
    return 0;
  }

  uint32_t word = mem->memory[word_addr];

  // Extract the specific byte based on offset using bit shifts
  return (word >> (24 - (byte_offset << 3))) & 0xFF;
}

// Write a single byte to memory
int memory_write_byte(dez_memory_t *mem, uint32_t address, uint8_t value) {
  if (UNLIKELY(!mem)) {
    printf("Error: NULL memory pointer\n");
    return -1;
  }

  // Convert byte address to word address and byte offset
  uint32_t word_addr = address >> 2;  // Optimized division by 4
  uint32_t byte_offset = address & 3; // Optimized modulo 4

  if (UNLIKELY(word_addr >= MEMORY_SIZE_WORDS)) {
    printf("Error: Memory write out of bounds at address 0x%04X\n", address);
    return -1;
  }

  // Check write permissions for code segment
  if (UNLIKELY(memory_is_code_segment(mem, word_addr) && mem->code_readonly)) {
    printf("Error: Write to read-only memory at address 0x%04X\n", address);
    return -1;
  }

  uint32_t word = mem->memory[word_addr];
  uint32_t shift = 24 - (byte_offset << 3);
  uint32_t mask = 0xFF << shift;

  // Update the specific byte using optimized bit operations
  word = (word & ~mask) | ((uint32_t)value << shift);
  mem->memory[word_addr] = word;
  return 0; // Success
}

// Additional utility functions

// Print memory statistics
void memory_print_stats(dez_memory_t *mem) {
  if (!mem) {
    printf("Error: NULL memory pointer\n");
    return;
  }

  printf("\n=== Memory Statistics ===\n");
  printf("Total Memory: %d words (%d bytes)\n", MEMORY_SIZE_WORDS,
         MEMORY_SIZE_BYTES);
  printf("Code Segment: 0x%04X - 0x%04X (%d words)\n", mem->code_start,
         mem->code_end, CODE_SIZE);
  printf("Data Segment: 0x%04X - 0x%04X (%d words)\n", mem->data_start,
         mem->data_end, DATA_SIZE);
  printf("Stack Segment: 0x%04X - 0x%04X (%d words)\n", mem->stack_start,
         mem->stack_end, STACK_SIZE);

  printf("\nProtection Flags:\n");
  printf("  Code Read-Only: %s\n", mem->code_readonly ? "Yes" : "No");
  printf("  Data Writable: %s\n", mem->data_writable ? "Yes" : "No");
  printf("  Stack Writable: %s\n", mem->stack_writable ? "Yes" : "No");

  printf("\nUsage Statistics:\n");
  printf("  Code Usage: %d accesses\n", mem->code_usage);
  printf("  Data Usage: %d accesses\n", mem->data_usage);
  printf("  Stack Usage: %d accesses (Max: %d)\n", mem->stack_usage,
         mem->max_stack_usage);

  printf("\nAccess Statistics:\n");
  printf("  Total Accesses: %d\n", mem->access_count);
  printf("  Reads: %d\n", mem->read_count);
  printf("  Writes: %d\n", mem->write_count);
}

// Get memory usage for a specific segment
uint32_t memory_get_usage(dez_memory_t *mem, uint32_t segment) {
  if (!mem)
    return 0;

  switch (segment) {
  case 0:
    return mem->code_usage; // Code segment
  case 1:
    return mem->data_usage; // Data segment
  case 2:
    return mem->stack_usage; // Stack segment
  default:
    return 0;
  }
}

// Set memory protection for a segment
void memory_set_protection(dez_memory_t *mem, uint32_t segment, bool readonly) {
  if (!mem)
    return;

  switch (segment) {
  case 0: // Code segment
    mem->code_readonly = readonly;
    break;
  case 1: // Data segment
    mem->data_writable = !readonly;
    break;
  case 2: // Stack segment
    mem->stack_writable = !readonly;
    break;
  }
}

// Check if an address is valid (public interface)
bool memory_is_valid_address_public(dez_memory_t *mem, uint32_t address) {
  return memory_is_valid_address(mem, address);
}

// Check if an address is in code segment (public interface)
bool memory_is_code_segment_public(dez_memory_t *mem, uint32_t address) {
  return memory_is_code_segment(mem, address);
}

// Check if an address is in data segment (public interface)
bool memory_is_data_segment_public(dez_memory_t *mem, uint32_t address) {
  return memory_is_data_segment(mem, address);
}

// Check if an address is in stack segment (public interface)
bool memory_is_stack_segment_public(dez_memory_t *mem, uint32_t address) {
  return memory_is_stack_segment(mem, address);
}

// Check if an address can be written to (public interface)
bool memory_can_write_public(dez_memory_t *mem, uint32_t address) {
  return memory_can_write(mem, address);
}
