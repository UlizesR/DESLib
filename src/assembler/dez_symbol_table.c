#include "dez_symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize symbol table
void symbol_table_init(symbol_table_t *table) {
  table->count = 0;
  table->next_string_addr =
      0x100; // Start string storage at address 0x100 (fits in 12 bits)
  table->pass = 1;
  memset(table->symbols, 0, sizeof(table->symbols));

  // Initialize hash table to empty (-1)
  for (int i = 0; i < SYMBOL_HASH_SIZE; i++) {
    table->hash_table[i] = -1;
  }
}

// Add symbol to table
bool symbol_table_add(symbol_table_t *table, const char *name,
                      symbol_type_t type, uint32_t address, uint32_t value,
                      const char *string_value, int line) {
  if (table->count >= 1024) {
    printf("Error: Symbol table full\n");
    return false;
  }

  // Check if symbol already exists
  symbol_t *existing = symbol_table_find(table, name);
  if (existing != NULL) {
    printf("Error: Symbol '%s' already defined at line %d\n", name,
           existing->line);
    return false;
  }

  symbol_t *symbol = &table->symbols[table->count++];
  strncpy(symbol->name, name, sizeof(symbol->name) - 1);
  symbol->name[sizeof(symbol->name) - 1] = '\0';
  symbol->type = type;
  symbol->address = address;
  symbol->value = value;
  symbol->defined = true;
  symbol->line = line;

  if (string_value != NULL) {
    strncpy(symbol->string_value, string_value,
            sizeof(symbol->string_value) - 1);
    symbol->string_value[sizeof(symbol->string_value) - 1] = '\0';
  } else {
    symbol->string_value[0] = '\0';
  }

  return true;
}

// Simple hash function for symbol names
static uint32_t symbol_hash(const char *name) {
  uint32_t hash = 5381;
  int c;
  while ((c = *name++)) {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  }
  return hash % SYMBOL_HASH_SIZE;
}

// Find symbol by name (optimized with hash table)
symbol_t *symbol_table_find(symbol_table_t *table, const char *name) {
  uint32_t hash = symbol_hash(name);
  int index = table->hash_table[hash];

  // If hash table has an entry, check it first
  if (index != -1 && index < table->count) {
    if (strcmp(table->symbols[index].name, name) == 0) {
      return &table->symbols[index];
    }
  }

  // Fallback to linear search (for hash collisions or if hash table not
  // updated)
  for (int i = table->count - 1; i >= 0; i--) {
    if (strcmp(table->symbols[i].name, name) == 0) {
      // Update hash table for faster future lookups
      table->hash_table[hash] = i;
      return &table->symbols[i];
    }
  }
  return NULL;
}

// Define a label
bool symbol_table_define(symbol_table_t *table, const char *name,
                         uint32_t address, int line) {
  return symbol_table_add(table, name, SYMBOL_LABEL, address, 0, NULL, line);
}

// Define a string literal
bool symbol_table_define_string(symbol_table_t *table, const char *name,
                                const char *value, int line) {
  uint32_t addr = symbol_table_allocate_string(table, value);
  return symbol_table_add(table, name, SYMBOL_STRING, addr, 0, value, line);
}

// Define a constant
bool symbol_table_define_constant(symbol_table_t *table, const char *name,
                                  uint32_t value, int line) {
  return symbol_table_add(table, name, SYMBOL_CONSTANT, value, value, NULL,
                          line);
}

// Allocate space for a string
uint32_t symbol_table_allocate_string(symbol_table_t *table, const char *str) {
  uint32_t addr = table->next_string_addr;
  table->next_string_addr += strlen(str) + 1; // +1 for null terminator
  return addr;
}

// Get string value by address
const char *symbol_table_get_string(symbol_table_t *table, uint32_t address) {
  for (int i = 0; i < table->count; i++) {
    if (table->symbols[i].type == SYMBOL_STRING &&
        table->symbols[i].address == address) {
      return table->symbols[i].string_value;
    }
  }
  return NULL;
}

// Print symbol table for debugging
void symbol_table_print(symbol_table_t *table) {
  printf("\n=== Symbol Table ===\n");
  printf("%-20s %-10s %-10s %-10s %s\n", "Name", "Type", "Address", "Value",
         "String");
  printf("------------------------------------------------------------\n");

  for (int i = 0; i < table->count; i++) {
    symbol_t *sym = &table->symbols[i];
    const char *type_str = "UNKNOWN";

    switch (sym->type) {
    case SYMBOL_LABEL:
      type_str = "LABEL";
      break;
    case SYMBOL_STRING:
      type_str = "STRING";
      break;
    case SYMBOL_VARIABLE:
      type_str = "VARIABLE";
      break;
    case SYMBOL_CONSTANT:
      type_str = "CONSTANT";
      break;
    }

    printf("%-20s %-10s 0x%08X 0x%08X %s\n", sym->name, type_str, sym->address,
           sym->value, sym->string_value);
  }
  printf("\n");
}

// Validate symbol table (check for undefined symbols)
bool symbol_table_validate(symbol_table_t *table) {
  bool valid = true;

  for (int i = 0; i < table->count; i++) {
    symbol_t *sym = &table->symbols[i];
    if (!sym->defined) {
      printf("Error: Undefined symbol '%s'\n", sym->name);
      valid = false;
    }
  }

  return valid;
}
