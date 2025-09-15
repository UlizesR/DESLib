#ifndef DEZ_ASSEMBLER_H
#define DEZ_ASSEMBLER_H

#include "dez_lexer.h"
#include "dez_parser.h"
#include "dez_symbol_table.h"
#include <stdbool.h>
#include <stdint.h>

// Assembler state
typedef struct {
  symbol_table_t symbol_table;
  uint32_t *output_buffer;
  int output_capacity;
  int output_size;
  bool verbose;
} assembler_t;

// Assembler functions
void assembler_init(assembler_t *assembler, int capacity, bool verbose);
void assembler_cleanup(assembler_t *assembler);
bool assembler_assemble_file(assembler_t *assembler, const char *input_file,
                             const char *output_file);
bool assembler_assemble_string(assembler_t *assembler, const char *input,
                               uint32_t **output, int *size);
void assembler_print_output(assembler_t *assembler);
void assembler_print_symbols(assembler_t *assembler);

// File I/O functions
char *read_file_contents(const char *filename);
bool write_binary_file(const char *filename, const uint32_t *data, int count);
bool write_binary_file_with_strings(const char *filename, const uint32_t *data,
                                    int count, symbol_table_t *symbol_table);
bool write_hex_file(const char *filename, const uint32_t *data, int count);

#endif // DEZ_ASSEMBLER_H
