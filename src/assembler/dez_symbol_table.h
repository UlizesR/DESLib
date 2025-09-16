#ifndef DEZ_SYMBOL_TABLE_H
#define DEZ_SYMBOL_TABLE_H

#include "../../include/dez_vm_types.h"
#include <stdbool.h>
#include <stdint.h>

// Symbol types
typedef enum {
  SYMBOL_LABEL,    // Code label
  SYMBOL_STRING,   // String literal
  SYMBOL_VARIABLE, // Variable
  SYMBOL_CONSTANT  // Named constant
} symbol_type_t;

// Symbol structure
typedef struct {
  char name[64];          // Symbol name
  symbol_type_t type;     // Symbol type
  uint32_t address;       // Memory address
  uint32_t value;         // Value (for constants)
  char string_value[256]; // String value (for strings)
  bool defined;           // Whether symbol is defined
  int line;               // Line where defined
} symbol_t;

// Symbol table with hash table optimization
typedef struct {
  symbol_t symbols[1024];           // Array of symbols
  int count;                        // Number of symbols
  int hash_table[SYMBOL_HASH_SIZE]; // Hash table for faster lookups (-1 =
                                    // empty)
  int next_string_addr;             // Next available string address
  int pass;                         // Current pass (1 or 2)
} symbol_table_t;

// Symbol table functions
void symbol_table_init(symbol_table_t *table);
bool symbol_table_add(symbol_table_t *table, const char *name,
                      symbol_type_t type, uint32_t address, uint32_t value,
                      const char *string_value, int line);
symbol_t *symbol_table_find(symbol_table_t *table, const char *name);
bool symbol_table_define(symbol_table_t *table, const char *name,
                         uint32_t address, int line);
bool symbol_table_define_string(symbol_table_t *table, const char *name,
                                const char *value, int line);
bool symbol_table_define_constant(symbol_table_t *table, const char *name,
                                  uint32_t value, int line);
void symbol_table_print(symbol_table_t *table);
bool symbol_table_validate(symbol_table_t *table);

// String management
uint32_t symbol_table_allocate_string(symbol_table_t *table, const char *str);
const char *symbol_table_get_string(symbol_table_t *table, uint32_t address);

#endif // DEZ_SYMBOL_TABLE_H
