#include "core/dez_memory.h"
#include <stdio.h>

int main() {
  printf("=== Memory System Test ===\n\n");

  dez_memory_t mem;

  // Initialize memory
  printf("Initializing memory...\n");
  memory_init(&mem);

  // Test basic memory operations
  printf("Testing memory operations...\n");

  // Test reading from code segment
  uint32_t code_value = memory_read_word(&mem, 0x0000);
  printf("Code segment (0x0000): 0x%08X\n", code_value);

  // Test writing to data segment
  int result = memory_write_word(&mem, 0x1000, 0x12345678);
  printf("Write to data segment (0x1000): %s\n", result == 0 ? "SUCCESS" : "FAILED");

  // Test reading from data segment
  uint32_t data_value = memory_read_word(&mem, 0x1000);
  printf("Data segment (0x1000): 0x%08X\n", data_value);

  // Test segment detection
  printf("\nSegment detection:\n");
  printf("Address 0x0000 is code: %s\n", memory_is_code_segment_public(&mem, 0x0000) ? "Yes" : "No");
  printf("Address 0x1000 is data: %s\n", memory_is_data_segment_public(&mem, 0x1000) ? "Yes" : "No");
  printf("Address 0x2000 is stack: %s\n", memory_is_stack_segment_public(&mem, 0x2000) ? "Yes" : "No");

  // Print memory statistics
  printf("\nMemory statistics:\n");
  memory_print_stats(&mem);

  printf("\n✅ Memory system test completed!\n");
  return 0;
}
