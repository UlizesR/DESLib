#include "dez_assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize assembler
void assembler_init(assembler_t *assembler, int capacity, bool verbose) {
  symbol_table_init(&assembler->symbol_table);
  assembler->output_buffer = malloc(capacity * sizeof(uint32_t));
  assembler->output_capacity = capacity;
  assembler->output_size = 0;
  assembler->verbose = verbose;
}

// Cleanup assembler
void assembler_cleanup(assembler_t *assembler) {
  if (assembler->output_buffer) {
    free(assembler->output_buffer);
    assembler->output_buffer = NULL;
  }
}

// Assemble from file
bool assembler_assemble_file(assembler_t *assembler, const char *input_file,
                             const char *output_file) {
  char *input = read_file_contents(input_file);
  if (!input) {
    printf("Error: Could not read input file '%s'\n", input_file);
    return false;
  }

  if (assembler->verbose) {
    printf("Assembling file: %s\n", input_file);
  }

  bool success = assembler_assemble_string(assembler, input, NULL, NULL);

  if (success) {
    // Write output file
    if (strstr(output_file, ".hex")) {
      success = write_hex_file(output_file, assembler->output_buffer,
                               assembler->output_size);
    } else {
      success = write_binary_file_with_strings(
          output_file, assembler->output_buffer, assembler->output_size,
          &assembler->symbol_table);
    }

    if (success) {
      printf("Successfully assembled %d instructions to %s\n",
             assembler->output_size, output_file);
    } else {
      printf("Error: Could not write output file '%s'\n", output_file);
    }
  }

  free(input);
  return success;
}

// Assemble from string
bool assembler_assemble_string(assembler_t *assembler, const char *input,
                               uint32_t **output, int *size) {
  parser_t parser;
  parser_init(&parser, input, &assembler->symbol_table,
              assembler->output_buffer, assembler->output_capacity);

  if (assembler->verbose) {
    printf("Starting assembly...\n");
  }

  bool success = parser_parse(&parser);

  if (success) {
    assembler->output_size = parser.output_size;

    // Append string data to output
    for (int i = 0; i < assembler->symbol_table.count; i++) {
      symbol_t *sym = &assembler->symbol_table.symbols[i];
      if (sym->type == SYMBOL_STRING && sym->defined) {
        // Store string data at the allocated address
        uint32_t addr = sym->address;
        const char *str = sym->string_value;
        int len = strlen(str) + 1; // +1 for null terminator

        // Ensure we have enough space (convert to word count)
        if (addr + len > assembler->output_capacity * sizeof(uint32_t)) {
          printf("Error: String data exceeds output capacity\n");
          success = false;
          break;
        }

        // Copy string data to output buffer
        memcpy((char *)&assembler->output_buffer[addr], str, len);

        // Update output size to include string data
        uint32_t end_addr = addr + len;
        uint32_t end_word =
            (end_addr + sizeof(uint32_t) - 1) / sizeof(uint32_t);
        if (end_word > (uint32_t)assembler->output_size) {
          assembler->output_size = (int)end_word;
        }

        if (assembler->verbose) {
          printf("Stored string '%s' at address 0x%04X\n", str, addr);
        }
      }
    }

    if (output) {
      *output = assembler->output_buffer;
    }
    if (size) {
      *size = assembler->output_size;
    }

    if (assembler->verbose) {
      printf("Assembly completed successfully\n");
      assembler_print_symbols(assembler);
      assembler_print_output(assembler);
    }
  } else {
    printf("Assembly failed\n");
  }

  return success;
}

// Print output buffer
void assembler_print_output(assembler_t *assembler) {
  printf("\n=== Generated Code ===\n");
  for (int i = 0; i < assembler->output_size; i++) {
    printf("0x%04X: 0x%08X\n", i, assembler->output_buffer[i]);
  }
  printf("\n");
}

// Print symbol table
void assembler_print_symbols(assembler_t *assembler) {
  symbol_table_print(&assembler->symbol_table);
}

// Read file contents
char *read_file_contents(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    return NULL;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocate buffer
  char *buffer = malloc(size + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }

  // Read file
  size_t bytes_read = fread(buffer, 1, size, file);
  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}

// Write binary file
bool write_binary_file(const char *filename, const uint32_t *data, int count) {
  FILE *file = fopen(filename, "wb");
  if (!file) {
    return false;
  }

  // Write count first
  fwrite(&count, sizeof(uint32_t), 1, file);

  // Write data (including string data)
  fwrite(data, sizeof(uint32_t), count, file);

  fclose(file);
  return true;
}

// Write binary file with string data appended
bool write_binary_file_with_strings(const char *filename, const uint32_t *data,
                                    int count, symbol_table_t *symbol_table) {
  FILE *file = fopen(filename, "wb");
  if (!file) {
    return false;
  }

  // Calculate total size including string data (for future use)
  (void)count; // Suppress unused parameter warning
  for (int i = 0; i < symbol_table->count; i++) {
    symbol_t *sym = &symbol_table->symbols[i];
    if (sym->type == SYMBOL_STRING && sym->defined) {
      // String data will be written below
      (void)sym; // Suppress unused variable warning
    }
  }

  // Write actual instruction count first (not including string data)
  fwrite(&count, sizeof(uint32_t), 1, file);

  // Write instruction data
  fwrite(data, sizeof(uint32_t), count, file);

  // Write string data
  for (int i = 0; i < symbol_table->count; i++) {
    symbol_t *sym = &symbol_table->symbols[i];
    if (sym->type == SYMBOL_STRING && sym->defined) {
      const char *str = sym->string_value;
      int len = (int)strlen(str) + 1; // +1 for null terminator
      int words = (len + (int)sizeof(uint32_t) - 1) / (int)sizeof(uint32_t);

      // Write string data padded to word boundary
      fwrite(str, 1, len, file);
      for (int j = len; j < words * (int)sizeof(uint32_t); j++) {
        fputc(0, file);
      }
    }
  }

  fclose(file);
  return true;
}

// Write hex file
bool write_hex_file(const char *filename, const uint32_t *data, int count) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    return false;
  }

  fprintf(file, "v2.0 raw\n");
  for (int i = 0; i < count; i++) {
    fprintf(file, "%08X\n", data[i]);
  }

  fclose(file);
  return true;
}
