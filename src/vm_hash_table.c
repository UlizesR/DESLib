#include "vm_hash_table.h"
#include <stdlib.h>
#include <string.h>

/**
 * Hash function for strings (djb2 algorithm)
 */
uint32_t vm_hash_string(const char *str) {
  uint32_t hash = 5381;
  int c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  }

  return hash;
}

/**
 * Create a new hash table
 */
vm_hash_table_t *vm_hash_table_create(size_t size) {
  vm_hash_table_t *table = malloc(sizeof(vm_hash_table_t));
  if (!table)
    return NULL;

  table->buckets = calloc(size, sizeof(vm_hash_entry_t *));
  if (!table->buckets) {
    free(table);
    return NULL;
  }

  table->size = size;
  table->count = 0;
  return table;
}

/**
 * Destroy hash table and free all memory
 */
void vm_hash_table_destroy(vm_hash_table_t *table) {
  if (!table)
    return;

  vm_hash_table_clear(table);
  free(table->buckets);
  free(table);
}

/**
 * Insert key-value pair into hash table
 */
int vm_hash_table_insert(vm_hash_table_t *table, const char *key, void *value) {
  if (!table || !key)
    return 0;

  uint32_t hash = vm_hash_string(key) % table->size;
  vm_hash_entry_t *entry = table->buckets[hash];

  // Check if key already exists
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      entry->value = value; // Update existing entry
      return 1;
    }
    entry = entry->next;
  }

  // Create new entry
  entry = malloc(sizeof(vm_hash_entry_t));
  if (!entry)
    return 0;

  entry->key = strdup(key);
  if (!entry->key) {
    free(entry);
    return 0;
  }

  entry->value = value;
  entry->next = table->buckets[hash];
  table->buckets[hash] = entry;
  table->count++;

  return 1;
}

/**
 * Lookup value by key
 */
void *vm_hash_table_lookup(vm_hash_table_t *table, const char *key) {
  if (!table || !key)
    return NULL;

  uint32_t hash = vm_hash_string(key) % table->size;
  vm_hash_entry_t *entry = table->buckets[hash];

  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      return entry->value;
    }
    entry = entry->next;
  }

  return NULL;
}

/**
 * Remove key-value pair from hash table
 */
int vm_hash_table_remove(vm_hash_table_t *table, const char *key) {
  if (!table || !key)
    return 0;

  uint32_t hash = vm_hash_string(key) % table->size;
  vm_hash_entry_t *entry = table->buckets[hash];
  vm_hash_entry_t *prev = NULL;

  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      if (prev) {
        prev->next = entry->next;
      } else {
        table->buckets[hash] = entry->next;
      }

      free(entry->key);
      free(entry);
      table->count--;
      return 1;
    }
    prev = entry;
    entry = entry->next;
  }

  return 0;
}

/**
 * Get number of entries in hash table
 */
size_t vm_hash_table_count(const vm_hash_table_t *table) {
  return table ? table->count : 0;
}

/**
 * Clear all entries from hash table
 */
void vm_hash_table_clear(vm_hash_table_t *table) {
  if (!table)
    return;

  for (size_t i = 0; i < table->size; i++) {
    vm_hash_entry_t *entry = table->buckets[i];
    while (entry) {
      vm_hash_entry_t *next = entry->next;
      free(entry->key);
      free(entry);
      entry = next;
    }
    table->buckets[i] = NULL;
  }

  table->count = 0;
}
