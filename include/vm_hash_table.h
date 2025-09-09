#ifndef VM_HASH_TABLE_H
#define VM_HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>

// Hash table entry
typedef struct vm_hash_entry {
  char *key;
  void *value;
  struct vm_hash_entry *next;
} vm_hash_entry_t;

// Hash table structure
typedef struct {
  vm_hash_entry_t **buckets;
  size_t size;
  size_t count;
} vm_hash_table_t;

// Hash table functions
vm_hash_table_t *vm_hash_table_create(size_t size);
void vm_hash_table_destroy(vm_hash_table_t *table);
int vm_hash_table_insert(vm_hash_table_t *table, const char *key, void *value);
void *vm_hash_table_lookup(vm_hash_table_t *table, const char *key);
int vm_hash_table_remove(vm_hash_table_t *table, const char *key);
size_t vm_hash_table_count(const vm_hash_table_t *table);
void vm_hash_table_clear(vm_hash_table_t *table);

// Hash function
uint32_t vm_hash_string(const char *str);

#endif // VM_HASH_TABLE_H
