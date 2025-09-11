#include "assembly_vm.h"
#include "vm_utils.h"

static const instruction_metadata_t instruction_table[] = {
    [INST_MOV] = {"MOV", 2, 0},
    [INST_ADD] = {"ADD", 3, INST_FLAG_ARITHMETIC},
    [INST_SUB] = {"SUB", 3, INST_FLAG_ARITHMETIC},
    [INST_MUL] = {"MUL", 3, INST_FLAG_ARITHMETIC},
    [INST_DIV] = {"DIV", 3, INST_FLAG_ARITHMETIC},
    [INST_LOAD] = {"LOAD", 2, INST_FLAG_MEMORY},
    [INST_STORE] = {"STORE", 2, INST_FLAG_MEMORY},
    [INST_JMP] = {"JMP", 1, INST_FLAG_JUMP},
    [INST_JZ] = {"JZ", 1, INST_FLAG_JUMP},
    [INST_JNZ] = {"JNZ", 1, INST_FLAG_JUMP},
    [INST_PUSH] = {"PUSH", 1, INST_FLAG_MEMORY},
    [INST_POP] = {"POP", 1, INST_FLAG_MEMORY},
    [INST_PRINT] = {"PRINT", 1, INST_FLAG_IO},
    [INST_PRINTS] = {"PRINTS", 1, INST_FLAG_IO},
    [INST_INPUT] = {"INPUT", 1, INST_FLAG_IO},
    [INST_CMP] = {"CMP", 2, INST_FLAG_ARITHMETIC},
    [INST_CALL] = {"CALL", 1, INST_FLAG_JUMP | INST_FLAG_MEMORY},
    [INST_RET] = {"RET", 0, INST_FLAG_JUMP},
    [INST_HALT] = {"HALT", 0, 0},
    [INST_NOP] = {"NOP", 0, 0},
    [INST_UNKNOWN] = {NULL, 0, 0}};

static int32_t add_op(int32_t a, int32_t b) { return a + b; }
static int32_t sub_op(int32_t a, int32_t b) { return a - b; }
static int32_t mul_op(int32_t a, int32_t b) { return a * b; }
static int32_t div_op(int32_t a, int32_t b) { return (b != 0) ? a / b : 0; }

static vm_hash_table_t *instruction_hash_table = NULL;

static void init_instruction_hash_table(void) {
  if (instruction_hash_table)
    return;

  instruction_hash_table = vm_hash_table_create(32);
  if (!instruction_hash_table)
    return;

  for (int i = 0; i < INST_UNKNOWN; i++) {
    if (instruction_table[i].mnemonic) {
      vm_hash_table_insert(instruction_hash_table,
                           instruction_table[i].mnemonic, (void *)(intptr_t)i);
    }
  }
}

vm_t *vm_create(void) {
  vm_t *vm = (vm_t *)malloc(sizeof(vm_t));
  if (!vm)
    return NULL;

  init_instruction_hash_table();
  vm_clear_error(&vm->last_error);
  vm->instruction_hash_table = instruction_hash_table;
  vm_reset(vm);
  return vm;
}

void vm_destroy(vm_t *vm) {
  if (vm) {
    free(vm);
  }
}

void vm_cleanup_global_resources(void) {
  if (instruction_hash_table) {
    vm_hash_table_destroy(instruction_hash_table);
    instruction_hash_table = NULL;
  }
}

void vm_reset(vm_t *vm) {
  memset(vm->registers, 0, sizeof(vm->registers));
  memset(vm->memory, 0, sizeof(vm->memory));
  vm->stack_pointer = MEMORY_SIZE - 1;
  vm->program_counter = 0;
  vm->status_flags = 0;
  vm->program_size = 0;
  vm->running = 0;
  vm->num_labels = 0;
  vm->verbose = 0;
  memset(vm->labels, 0, sizeof(vm->labels));
  vm_clear_error(&vm->last_error);
}

const instruction_metadata_t *
get_instruction_metadata(instruction_type_t type) {
  if (type < 0 || type >= INST_UNKNOWN) {
    return NULL;
  }
  return &instruction_table[type];
}

instruction_type_t parse_instruction(const char *mnemonic) {
  if (instruction_hash_table) {
    void *result = vm_hash_table_lookup(instruction_hash_table, mnemonic);
    if (result) {
      return (instruction_type_t)(intptr_t)result;
    }
  }

  for (int i = 0; i < INST_UNKNOWN; i++) {
    if (instruction_table[i].mnemonic &&
        strcmp(mnemonic, instruction_table[i].mnemonic) == 0) {
      return (instruction_type_t)i;
    }
  }
  return INST_UNKNOWN;
}

operand_t parse_operand(const char *str) {
  operand_t op = {0};

  while (isspace(*str))
    str++;

  if (str[0] == 'R' && isdigit(str[1])) {
    op.type = OP_REGISTER;
    op.reg = str[1] - '0';
  } else if (str[0] == '[') {
    op.type = OP_MEMORY;
    const char *end = strchr(str, ']');
    if (end && end > str + 1) {
      size_t len = end - str - 1;
      if (str[1] == 'R' && isdigit(str[2]) && len == 2) {
        op.reg = str[2] - '0';
      } else {
        char temp[16];
        strncpy(temp, str + 1, len);
        temp[len] = '\0';
        op.value = atoi(temp);
      }
    }
  } else if (str[0] == '#') {
    op.type = OP_IMMEDIATE;
    op.value = atoi(str + 1);
  } else if (str[0] == '"' && str[strlen(str) - 1] == '"') {
    op.type = OP_STRING;
    size_t len = strlen(str) - 2;
    if (len >= MAX_LINE_LENGTH)
      len = MAX_LINE_LENGTH - 1;
    strncpy(op.string, str + 1, len);
    op.string[len] = '\0';
  } else if (isdigit(str[0]) || str[0] == '-') {
    op.type = OP_IMMEDIATE;
    op.value = atoi(str);
  } else {
    // Only treat as label if the string is not empty
    if (strlen(str) > 0) {
      op.type = OP_LABEL;
      strncpy(op.label, str, MAX_LABEL_LENGTH - 1);
      op.label[MAX_LABEL_LENGTH - 1] = '\0';
    } else {
      op.type = OP_IMMEDIATE;
      op.value = 0;
    }
  }

  return op;
}

// Helper function to tokenize assembly line
static int tokenize_line(const char *line, char *tokens[], int max_tokens) {
  char line_copy[MAX_LINE_LENGTH];
  strncpy(line_copy, line, MAX_LINE_LENGTH - 1);
  line_copy[MAX_LINE_LENGTH - 1] = '\0';

  // Remove comments and trim whitespace
  vm_str_remove_comments(line_copy);
  char *start = vm_str_trim(line_copy);

  if (vm_str_is_empty(start)) {
    return 0; // Empty line
  }

  // Check if this is a label (ends with ':')
  if (vm_str_ends_with(start, ":")) {
    return 0; // This is a label, not an instruction
  }

  // Tokenize with proper string handling
  int token_count = 0;
  char *pos = start;
  static char token_storage[4][MAX_LINE_LENGTH]; // Storage for token copies

  while (*pos && token_count < max_tokens) {
    // Skip whitespace
    while (*pos && isspace(*pos))
      pos++;
    if (!*pos)
      break;

    if (*pos == '"') {
      // Handle quoted string
      char *token_start = pos;
      pos++; // Skip opening quote
      while (*pos && *pos != '"')
        pos++;
      if (*pos == '"')
        pos++; // Skip closing quote

      // Copy the token (including quotes)
      size_t len = pos - token_start;
      if (len < MAX_LINE_LENGTH) {
        strncpy(token_storage[token_count], token_start, len);
        token_storage[token_count][len] = '\0';
        tokens[token_count] = token_storage[token_count];
        token_count++;
      }
    } else {
      // Handle regular token
      char *token_start = pos;
      while (*pos && !isspace(*pos) && *pos != ',')
        pos++;

      // Copy the token
      size_t len = pos - token_start;
      if (len < MAX_LINE_LENGTH) {
        strncpy(token_storage[token_count], token_start, len);
        token_storage[token_count][len] = '\0';
        tokens[token_count] = token_storage[token_count];
        token_count++;
      }

      if (*pos == ',') {
        pos++; // Skip comma
      }
    }
  }

  return token_count;
}

// Helper function to parse operands based on instruction type
static int parse_instruction_operands(instruction_t *inst, char *tokens[],
                                      int token_count) {
  inst->num_operands = 0;

  switch (inst->type) {
  case INST_MOV:
    if (token_count >= 3) {
      inst->operands[0] = parse_operand(tokens[1]);
      inst->operands[1] = parse_operand(tokens[2]);
      inst->num_operands = 2;
    }
    break;

  case INST_ADD:
  case INST_SUB:
  case INST_MUL:
  case INST_DIV:
    if (token_count >= 4) {
      inst->operands[0] = parse_operand(tokens[1]);
      inst->operands[1] = parse_operand(tokens[2]);
      inst->operands[2] = parse_operand(tokens[3]);
      inst->num_operands = 3;
    }
    break;

  case INST_LOAD:
  case INST_STORE:
    if (token_count >= 3) {
      inst->operands[0] = parse_operand(tokens[1]);
      inst->operands[1] = parse_operand(tokens[2]);
      inst->num_operands = 2;
    }
    break;

  case INST_JMP:
  case INST_JZ:
  case INST_JNZ:
  case INST_CALL:
    if (token_count >= 2) {
      inst->operands[0] = parse_operand(tokens[1]);
      inst->num_operands = 1;
    }
    break;

  case INST_PUSH:
  case INST_POP:
  case INST_PRINT:
  case INST_PRINTS:
  case INST_INPUT:
    if (token_count >= 2) {
      inst->operands[0] = parse_operand(tokens[1]);
      inst->num_operands = 1;
    }
    break;

  case INST_CMP:
    if (token_count >= 3) {
      inst->operands[0] = parse_operand(tokens[1]);
      inst->operands[1] = parse_operand(tokens[2]);
      inst->num_operands = 2;
    }
    break;

  case INST_RET:
  case INST_HALT:
  case INST_NOP:
    inst->num_operands = 0;
    break;

  default:
    return 0;
  }

  return 1;
}

// Parse a single assembly line
int parse_assembly_line(const char *line, instruction_t *inst) {
  char *tokens[4];
  int token_count = tokenize_line(line, tokens, 4);

  if (token_count == 0) {
    return 0; // Empty line or label
  }

  // Parse instruction
  inst->type = parse_instruction(tokens[0]);
  if (inst->type == INST_UNKNOWN) {
    printf("Unknown instruction: %s\n", tokens[0]);
    return 0;
  }

  // Parse operands based on instruction type
  return parse_instruction_operands(inst, tokens, token_count);
}

int vm_execute_instruction(vm_t *vm, instruction_t *inst) {
  // Pre-execution validation
  vm_validation_result_t validation =
      vm_validate_instruction(inst, vm->program_counter);
  if (!validation.valid) {
    vm->last_error = validation.error;
    return 0;
  }

  switch (inst->type) {
  case INST_MOV:
    return handle_mov(vm, inst);

  case INST_ADD:
    return handle_arithmetic(vm, inst, add_op);

  case INST_SUB:
    return handle_arithmetic(vm, inst, sub_op);

  case INST_MUL:
    return handle_arithmetic(vm, inst, mul_op);

  case INST_DIV:
    return handle_arithmetic(vm, inst, div_op);

  case INST_LOAD:
    return handle_load(vm, inst);

  case INST_STORE:
    return handle_store(vm, inst);

  case INST_JMP:
    return handle_jump(vm, inst, 1); // Always jump

  case INST_JZ:
    return handle_jump(vm, inst, vm->status_flags & FLAG_ZERO);

  case INST_JNZ:
    return handle_jump(vm, inst, !(vm->status_flags & FLAG_ZERO));

  case INST_PUSH:
    return handle_push(vm, inst);

  case INST_POP:
    return handle_pop(vm, inst);

  case INST_PRINT:
    return handle_print(vm, inst);

  case INST_PRINTS:
    return handle_prints(vm, inst);

  case INST_INPUT:
    return handle_input(vm, inst);

  case INST_CMP:
    return handle_cmp(vm, inst);

  case INST_CALL:
    return handle_call(vm, inst);

  case INST_RET:
    return handle_ret(vm, inst);

  case INST_HALT:
    vm->running = 0;
    return 1;

  case INST_NOP:
    return 1; // Do nothing

  default:
    return 0;
  }

  return 1;
}

void vm_run(vm_t *vm) {
  vm->running = 1;         // Mark VM as running
  vm->program_counter = 0; // Start at first instruction

  // Main execution loop - continues until HALT or error
  while (vm->running && vm->program_counter < vm->program_size) {
    instruction_t *inst =
        &vm->program[vm->program_counter]; // Fetch instruction
    int old_pc = vm->program_counter;      // Remember PC before execution

    // Execute the instruction
    if (vm_execute_instruction(vm, inst)) {
      // Only increment PC if it wasn't changed by a jump instruction
      if (vm->program_counter == old_pc) {
        vm->program_counter++; // Move to next instruction
      }
    } else {
      // Instruction execution failed - error already set in
      // vm_execute_instruction
      vm_print_error(&vm->last_error);
      vm->running = 0; // Stop execution
    }
  }
}

// Execute one instruction (step mode)
void vm_step(vm_t *vm) {
  if (vm->program_counter >= vm->program_size) {
    vm->running = 0;
    return;
  }

  instruction_t *inst = &vm->program[vm->program_counter];

  // Use the new instruction printer
  vm_print_context_t context = {.mode = VM_PRINT_MODE_STEP,
                                .instruction_number = vm->program_counter};
  vm_print_instruction(inst, &context);
  printf("\n");

  if (vm_execute_instruction(vm, inst)) {
    vm->program_counter++;
  } else {
    printf("Error executing instruction at PC=%d\n", vm->program_counter);
    vm->running = 0;
  }

  vm_print_registers(vm);
  printf("\n");
}

// Print VM state
void vm_print_state(vm_t *vm) {
  printf("=== Virtual Machine State ===\n");
  printf("Program Counter: %d\n", vm->program_counter);
  printf("Stack Pointer: %d\n", vm->stack_pointer);
  printf("Status Flags: 0x%02X\n", vm->status_flags);
  printf("Running: %s\n", vm->running ? "Yes" : "No");
  vm_print_registers(vm);
  printf("\n");
}

void vm_print_registers(vm_t *vm) {
  printf("Registers: ");
  for (int i = 0; i < NUM_REGISTERS; i++) {
    printf("R%d=%d ", i, vm->registers[i]);
  }
  printf("\n");
}

void vm_print_memory(vm_t *vm, int start, int end) {
  printf("Memory [%d-%d]:\n", start, end);
  for (int i = start; i <= end && i < MEMORY_SIZE; i += 4) {
    printf("[%d]: %d\n", i,
           (vm->memory[i] << 24) | (vm->memory[i + 1] << 16) |
               (vm->memory[i + 2] << 8) | vm->memory[i + 3]);
  }
}

void vm_print_program(vm_t *vm) {
  printf("\n=== Program Disassembly ===\n");
  printf("Address  Instruction\n");
  printf("-------- -----------\n");

  for (int i = 0; i < vm->program_size; i++) {
    instruction_t *inst = &vm->program[i];
    printf("0x%04X   ", i);

    // Use the new instruction printer
    vm_print_context_t context = {.mode = VM_PRINT_MODE_SIMPLE,
                                  .instruction_number = i};
    vm_print_instruction(inst, &context);
    printf("\n");
  }
  printf("\n");
}

// Add a label to the label table
int add_label(vm_t *vm, const char *name, int address) {
  if (vm->num_labels >= MAX_LABELS) {
    printf("Error: Too many labels (max %d)\n", MAX_LABELS);
    return 0;
  }

  // Check for duplicate labels
  for (int i = 0; i < vm->num_labels; i++) {
    if (strcmp(vm->labels[i].name, name) == 0) {
      printf("Error: Duplicate label '%s'\n", name);
      return 0;
    }
  }

  strncpy(vm->labels[vm->num_labels].name, name, MAX_LABEL_LENGTH - 1);
  vm->labels[vm->num_labels].name[MAX_LABEL_LENGTH - 1] = '\0';
  vm->labels[vm->num_labels].address = address;
  vm->num_labels++;
  return 1;
}

// Find a label by name
int find_label(vm_t *vm, const char *name) {
  for (int i = 0; i < vm->num_labels; i++) {
    if (strcmp(vm->labels[i].name, name) == 0) {
      return vm->labels[i].address;
    }
  }
  return -1; // Label not found
}

// Resolve all labels in the program
int resolve_labels(vm_t *vm) {
  for (int i = 0; i < vm->program_size; i++) {
    instruction_t *inst = &vm->program[i];

    // Check all operands for labels
    for (int j = 0; j < inst->num_operands; j++) {
      if (inst->operands[j].type == OP_LABEL) {
        int address = find_label(vm, inst->operands[j].label);
        if (address == -1) {
          printf("Error: Undefined label '%s' at instruction %d\n",
                 inst->operands[j].label, i);
          return 0;
        }
        // Convert label to immediate value
        inst->operands[j].type = OP_IMMEDIATE;
        inst->operands[j].value = address;
      }
    }
  }
  return 1;
}

// Helper function to process a single line for label collection
static int process_line_for_labels(const char *line, vm_t *vm,
                                   int *instruction_count) {
  char line_copy[MAX_LINE_LENGTH];
  strncpy(line_copy, line, MAX_LINE_LENGTH - 1);
  line_copy[MAX_LINE_LENGTH - 1] = '\0';

  // Remove comments and trim whitespace
  vm_str_remove_comments(line_copy);
  char *start = vm_str_trim(line_copy);

  if (vm_str_is_empty(start)) {
    return 1; // Empty line, continue
  }

  // Check if this is a label (ends with ':')
  if (vm_str_ends_with(start, ":")) {
    // Remove the ':' and add to label table
    start[strlen(start) - 1] = '\0';
    // Check if label name is not empty after removing colon
    if (strlen(start) > 0 && !vm_str_is_empty(start)) {
      if (!add_label(vm, start, *instruction_count)) {
        return 0;
      }
    }
  } else {
    // This is an instruction
    (*instruction_count)++;
  }

  return 1;
}

// Load program from string
int vm_load_program_from_string(vm_t *vm, const char *program_string) {
  if (!program_string) {
    printf("Error: Null program string\n");
    return 0;
  }

  char line[MAX_LINE_LENGTH];
  vm->program_size = 0;
  int instruction_count = 0;

  // First pass: collect labels
  const char *current = program_string;
  while (*current) {
    // Find end of line
    const char *line_end = current;
    while (*line_end && *line_end != '\n') {
      line_end++;
    }

    // Copy line
    size_t line_len = line_end - current;
    if (line_len >= MAX_LINE_LENGTH) {
      line_len = MAX_LINE_LENGTH - 1;
    }
    strncpy(line, current, line_len);
    line[line_len] = '\0';

    if (!process_line_for_labels(line, vm, &instruction_count)) {
      return 0;
    }

    // Move to next line
    current = line_end + (*line_end == '\n' ? 1 : 0);
  }

  // Second pass: parse instructions
  current = program_string;
  vm->program_size = 0;

  while (*current && vm->program_size < MAX_INSTRUCTIONS) {
    // Find end of line
    const char *line_end = current;
    while (*line_end && *line_end != '\n') {
      line_end++;
    }

    // Copy line
    size_t line_len = line_end - current;
    if (line_len >= MAX_LINE_LENGTH) {
      line_len = MAX_LINE_LENGTH - 1;
    }
    strncpy(line, current, line_len);
    line[line_len] = '\0';

    instruction_t inst;
    if (parse_assembly_line(line, &inst)) {
      vm->program[vm->program_size++] = inst;
    }

    // Move to next line
    current = line_end + (*line_end == '\n' ? 1 : 0);
  }

  // Resolve all labels
  if (!resolve_labels(vm)) {
    return 0;
  }

  // Only show detailed info in verbose mode
  if (vm->verbose) {
    printf("Loaded %d instructions and %d labels\n", vm->program_size,
           vm->num_labels);
  }
  return 1;
}

// Load program from file
int vm_load_program(vm_t *vm, const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("Error: Cannot open file %s\n", filename);
    return 0;
  }

  char line[MAX_LINE_LENGTH];
  vm->program_size = 0;
  int instruction_count = 0;

  // First pass: collect labels
  while (fgets(line, sizeof(line), file)) {
    if (!process_line_for_labels(line, vm, &instruction_count)) {
      fclose(file);
      return 0;
    }
  }

  // Reset file pointer for second pass
  rewind(file);
  vm->program_size = 0;

  // Second pass: parse instructions
  while (fgets(line, sizeof(line), file) &&
         vm->program_size < MAX_INSTRUCTIONS) {
    instruction_t inst;
    if (parse_assembly_line(line, &inst)) {
      vm->program[vm->program_size++] = inst;
    }
  }

  fclose(file);

  // Resolve all labels
  if (!resolve_labels(vm)) {
    return 0;
  }

  // Only show detailed info in verbose mode
  if (vm->verbose) {
    printf("Loaded %d instructions and %d labels\n", vm->program_size,
           vm->num_labels);
  }
  return 1;
}

void vm_set_verbose(vm_t *vm, int verbose) { vm->verbose = verbose; }

int32_t get_operand_value(vm_t *vm, const operand_t *op) {
  if (op->type == OP_REGISTER) {
    return vm->registers[op->reg];
  } else {
    return op->value;
  }
}

int32_t get_memory_address(vm_t *vm, const operand_t *op) {
  if (op->type == OP_REGISTER) {
    return vm->registers[op->reg];
  } else {
    return op->value;
  }
}

int validate_memory_access(int32_t address) {
  return (address >= 0 && address < MEMORY_SIZE - 3);
}

int validate_stack_operation(vm_t *vm, int is_push) {
  if (is_push) {
    return vm->stack_pointer >= 3; // Need 4 bytes for 32-bit value
  } else {
    return vm->stack_pointer < MEMORY_SIZE - 3; // Can't pop from empty stack
  }
}

void set_status_flags(vm_t *vm, int32_t result) {
  // Zero flag
  if (result == 0) {
    vm->status_flags |= FLAG_ZERO;
  } else {
    vm->status_flags &= ~FLAG_ZERO;
  }

  // Note: Carry and overflow flags are set by specific instructions
  // (like CMP) that perform comparisons or detect overflow conditions
}

int32_t read_memory_32(vm_t *vm, int32_t address) {
  return (vm->memory[address] << 24) | (vm->memory[address + 1] << 16) |
         (vm->memory[address + 2] << 8) | vm->memory[address + 3];
}

void write_memory_32(vm_t *vm, int32_t address, int32_t value) {
  vm->memory[address] = (value >> 24) & 0xFF;
  vm->memory[address + 1] = (value >> 16) & 0xFF;
  vm->memory[address + 2] = (value >> 8) & 0xFF;
  vm->memory[address + 3] = value & 0xFF;
}

/**
 * Handle MOV instruction
 */
static int handle_memory_access(vm_t *vm, const instruction_t *inst,
                                int is_load) {
  int32_t address = get_memory_address(vm, &inst->operands[1]);
  if (!validate_memory_access(address)) {
    vm_set_error(&vm->last_error, VM_ERROR_MEMORY_ACCESS_VIOLATION,
                 "Memory access violation", vm->program_counter, address, NULL,
                 instruction_table[inst->type].mnemonic);
    return 0;
  }

  if (is_load) {
    vm->registers[inst->operands[0].reg] = read_memory_32(vm, address);
  } else {
    write_memory_32(vm, address, vm->registers[inst->operands[0].reg]);
  }
  return 1;
}

int handle_mov(vm_t *vm, const instruction_t *inst) {
  int32_t value = get_operand_value(vm, &inst->operands[1]);
  vm->registers[inst->operands[0].reg] = value;
  return 1;
}

/**
 * Handle arithmetic instructions (ADD, SUB, MUL, DIV)
 */
int handle_arithmetic(vm_t *vm, const instruction_t *inst,
                      int32_t (*op)(int32_t, int32_t)) {
  int32_t a = get_operand_value(vm, &inst->operands[1]);
  int32_t b = get_operand_value(vm, &inst->operands[2]);

  if (op == div_op && b == 0) {
    vm_set_error(&vm->last_error, VM_ERROR_DIVISION_BY_ZERO, "Division by zero",
                 vm->program_counter, -1, NULL,
                 instruction_table[inst->type].mnemonic);
    return 0;
  }

  int32_t result = op(a, b);
  vm->registers[inst->operands[0].reg] = result;
  set_status_flags(vm, result);
  return 1;
}

/**
 * Handle LOAD instruction
 */
int handle_load(vm_t *vm, const instruction_t *inst) {
  return handle_memory_access(vm, inst, 1);
}

/**
 * Handle STORE instruction
 */
int handle_store(vm_t *vm, const instruction_t *inst) {
  return handle_memory_access(vm, inst, 0);
}

/**
 * Handle jump instructions (JMP, JZ, JNZ)
 */
int handle_jump(vm_t *vm, const instruction_t *inst, int condition) {
  if (condition) {
    vm->program_counter = inst->operands[0].value;
  }
  return 1;
}

/**
 * Handle PUSH instruction
 */
static int handle_stack_operation(vm_t *vm, const instruction_t *inst,
                                  int is_push) {
  if (!validate_stack_operation(vm, is_push)) {
    vm_error_code_t error =
        is_push ? VM_ERROR_STACK_OVERFLOW : VM_ERROR_STACK_UNDERFLOW;
    const char *message = is_push ? "Stack overflow" : "Stack underflow";
    vm_set_error(&vm->last_error, error, message, vm->program_counter, -1, NULL,
                 instruction_table[inst->type].mnemonic);
    return 0;
  }

  if (is_push) {
    int32_t value = vm->registers[inst->operands[0].reg];
    write_memory_32(vm, vm->stack_pointer - 3, value);
    vm->stack_pointer -= 4;
  } else {
    vm->stack_pointer += 4;
    vm->registers[inst->operands[0].reg] =
        read_memory_32(vm, vm->stack_pointer - 3);
  }
  return 1;
}

int handle_push(vm_t *vm, const instruction_t *inst) {
  return handle_stack_operation(vm, inst, 1);
}

/**
 * Handle POP instruction
 */
int handle_pop(vm_t *vm, const instruction_t *inst) {
  return handle_stack_operation(vm, inst, 0);
}

/**
 * Handle PRINT instruction
 */
int handle_print(vm_t *vm, const instruction_t *inst) {
  int32_t value = get_operand_value(vm, &inst->operands[0]);
  printf("%d\n", value);
  return 1;
}

int handle_prints(vm_t *vm, const instruction_t *inst) {
  if (inst->operands[0].type == OP_STRING) {
    printf("%s\n", inst->operands[0].string);
  } else if (inst->operands[0].type == OP_REGISTER) {
    // Treat register value as memory address pointing to string
    int32_t address = vm->registers[inst->operands[0].reg];
    if (validate_memory_access(address)) {
      printf("%s\n", (char *)&vm->memory[address]);
    } else {
      vm_set_error(&vm->last_error, VM_ERROR_MEMORY_ACCESS_VIOLATION,
                   "Invalid string address", vm->program_counter, address, NULL,
                   "PRINTS");
      return 0;
    }
  } else {
    vm_set_error(&vm->last_error, VM_ERROR_INVALID_OPERAND,
                 "PRINTS requires string literal or register",
                 vm->program_counter, -1, NULL, "PRINTS");
    return 0;
  }
  return 1;
}

/**
 * Handle INPUT instruction
 */
int handle_input(vm_t *vm, const instruction_t *inst) {
  int32_t value;
  if (vm->verbose) {
    printf("Input: ");
  }
  if (scanf("%d", &value) == 1) {
    vm->registers[inst->operands[0].reg] = value;
    return 1;
  } else {
    vm_set_error(&vm->last_error, VM_ERROR_INVALID_INPUT, "Invalid input",
                 vm->program_counter, -1, NULL,
                 instruction_table[inst->type].mnemonic);
    return 0;
  }
}

int handle_cmp(vm_t *vm, const instruction_t *inst) {
  int32_t a = get_operand_value(vm, &inst->operands[0]);
  int32_t b = get_operand_value(vm, &inst->operands[1]);
  int32_t result = a - b;

  // Set zero flag
  if (result == 0) {
    vm->status_flags |= FLAG_ZERO;
  } else {
    vm->status_flags &= ~FLAG_ZERO;
  }

  // Set carry flag (a < b)
  if (a < b) {
    vm->status_flags |= FLAG_CARRY;
  } else {
    vm->status_flags &= ~FLAG_CARRY;
  }

  // Set overflow flag (signed overflow in subtraction)
  if ((a > 0 && b < 0 && result < 0) || (a < 0 && b > 0 && result > 0)) {
    vm->status_flags |= FLAG_OVERFLOW;
  } else {
    vm->status_flags &= ~FLAG_OVERFLOW;
  }

  return 1;
}

/**
 * Handle CALL instruction - call subroutine
 */
int handle_call(vm_t *vm, const instruction_t *inst) {
  // Get target address
  int32_t target_address = get_operand_value(vm, &inst->operands[0]);

  // Validate target address
  if (target_address < 0 || target_address >= vm->program_size) {
    vm_set_error(&vm->last_error, VM_ERROR_INVALID_MEMORY_ADDRESS,
                 "Invalid call target address", vm->program_counter,
                 target_address, NULL, "CALL");
    return 0;
  }

  // Check if we have space on stack for return address
  if (!validate_stack_operation(vm, 1)) { // 1 = push operation
    vm_set_error(&vm->last_error, VM_ERROR_STACK_OVERFLOW,
                 "Stack overflow on CALL", vm->program_counter, -1, NULL,
                 "CALL");
    return 0;
  }

  // Push return address (current PC + 1) onto stack
  int32_t return_address = vm->program_counter + 1;
  write_memory_32(vm, vm->stack_pointer - 3, return_address);
  vm->stack_pointer -= 4;

  // Jump to target address
  vm->program_counter = target_address;
  return 1;
}

/**
 * Handle RET instruction - return from subroutine
 */
int handle_ret(vm_t *vm, const instruction_t *inst) {
  (void)inst; // Unused parameter
  // Check if stack has a return address
  if (!validate_stack_operation(vm, 0)) { // 0 = pop operation
    vm_set_error(&vm->last_error, VM_ERROR_STACK_UNDERFLOW,
                 "Stack underflow on RET", vm->program_counter, -1, NULL,
                 "RET");
    return 0;
  }

  // Pop return address from stack
  vm->stack_pointer += 4;
  int32_t return_address = read_memory_32(vm, vm->stack_pointer - 3);

  // Validate return address
  if (return_address < 0 || return_address >= vm->program_size) {
    vm_set_error(&vm->last_error, VM_ERROR_INVALID_MEMORY_ADDRESS,
                 "Invalid return address", vm->program_counter, return_address,
                 NULL, "RET");
    return 0;
  }

  // Set program counter to return address
  vm->program_counter = return_address;
  return 1;
}

// Binary format constants
#define DEZ_MAGIC 0xDEADBEEF
#define DEZ_VERSION 1

// Binary instruction encoding
typedef struct {
  uint32_t magic;        // Magic number: 0xDEADBEEF
  uint32_t version;      // Version number
  uint32_t program_size; // Number of instructions
  uint32_t num_labels;   // Number of labels
} dez_header_t;

typedef struct {
  uint8_t opcode;        // Instruction opcode
  uint8_t operand_count; // Number of operands
  uint8_t operand_types; // Bit field for operand types (2 bits per operand)
  uint8_t reserved;      // Reserved for alignment
  union {
    struct {
      uint8_t reg1, reg2, reg3; // Register operands
      uint8_t padding;
    } regs;
    struct {
      int32_t value1, value2, value3; // Immediate values
    } values;
    struct {
      uint32_t label_offset; // Offset into string table
      uint32_t padding;
    } label;
    struct {
      uint32_t string_offset; // Offset into string table
      uint32_t string_length;
    } string;
  } operands;
} dez_instruction_t;

// Helper function to encode operand types
static uint8_t encode_operand_types(const instruction_t *inst) {
  uint8_t types = 0;
  for (int i = 0; i < inst->num_operands && i < 3; i++) {
    // Map operand types to 2-bit values
    uint8_t type_code = 0;
    switch (inst->operands[i].type) {
    case OP_REGISTER:
      type_code = 0;
      break;
    case OP_IMMEDIATE:
      type_code = 1;
      break;
    case OP_MEMORY:
      type_code = 2;
      break;
    case OP_LABEL:
      type_code = 3;
      break;
    case OP_STRING:
      type_code = 3;
      break; // Use same code as label for now
    default:
      type_code = 0;
      break;
    }
    types |= (type_code & 0x3) << (i * 2);
  }
  return types;
}

int assemble_file(const char *input_file, const char *output_file) {
  // Create VM to parse the assembly
  vm_t *vm = vm_create();
  if (!vm) {
    printf("Error: Failed to create VM for assembly\n");
    return 0;
  }

  // Load the assembly program
  if (!vm_load_program(vm, input_file)) {
    printf("Error: Failed to load assembly file %s\n", input_file);
    vm_destroy(vm);
    return 0;
  }

  // Open output file
  FILE *out = fopen(output_file, "wb");
  if (!out) {
    printf("Error: Cannot create output file %s\n", output_file);
    vm_destroy(vm);
    return 0;
  }

  // Write header
  dez_header_t header = {.magic = DEZ_MAGIC,
                         .version = DEZ_VERSION,
                         .program_size = vm->program_size,
                         .num_labels = vm->num_labels};

  if (fwrite(&header, sizeof(header), 1, out) != 1) {
    printf("Error: Failed to write header\n");
    fclose(out);
    vm_destroy(vm);
    return 0;
  }

  // Write labels
  for (int i = 0; i < vm->num_labels; i++) {
    uint32_t name_length = strlen(vm->labels[i].name);
    if (fwrite(&vm->labels[i].address, sizeof(uint32_t), 1, out) != 1 ||
        fwrite(&name_length, sizeof(uint32_t), 1, out) != 1 ||
        fwrite(vm->labels[i].name, 1, name_length, out) != name_length) {
      printf("Error: Failed to write label %s\n", vm->labels[i].name);
      fclose(out);
      vm_destroy(vm);
      return 0;
    }
  }

  // Write instructions
  for (int i = 0; i < vm->program_size; i++) {
    instruction_t *inst = &vm->program[i];
    dez_instruction_t dez_inst = {0};

    dez_inst.opcode = (uint8_t)inst->type;
    dez_inst.operand_count = inst->num_operands;
    dez_inst.operand_types = encode_operand_types(inst);

    // Encode operands based on their types
    for (int j = 0; j < inst->num_operands && j < 3; j++) {
      switch (inst->operands[j].type) {
      case OP_REGISTER:
        if (j == 0)
          dez_inst.operands.regs.reg1 = inst->operands[j].reg;
        else if (j == 1)
          dez_inst.operands.regs.reg2 = inst->operands[j].reg;
        else if (j == 2)
          dez_inst.operands.regs.reg3 = inst->operands[j].reg;
        break;
      case OP_IMMEDIATE:
        if (j == 0)
          dez_inst.operands.values.value1 = inst->operands[j].value;
        else if (j == 1)
          dez_inst.operands.values.value2 = inst->operands[j].value;
        else if (j == 2)
          dez_inst.operands.values.value3 = inst->operands[j].value;
        break;
      case OP_LABEL:
        // For labels, we'll store the address after resolving
        if (j == 0)
          dez_inst.operands.values.value1 =
              find_label(vm, inst->operands[j].label);
        else if (j == 1)
          dez_inst.operands.values.value2 =
              find_label(vm, inst->operands[j].label);
        else if (j == 2)
          dez_inst.operands.values.value3 =
              find_label(vm, inst->operands[j].label);
        break;
      case OP_STRING:
        // For strings, we'll store the length and then the string data
        if (j == 0) {
          dez_inst.operands.string.string_length =
              strlen(inst->operands[j].string);
          // We'll write the string data after the instruction
        }
        break;
      default:
        break;
      }
    }

    // Write instruction
    if (fwrite(&dez_inst, sizeof(dez_instruction_t), 1, out) != 1) {
      printf("Error: Failed to write instruction %d\n", i);
      fclose(out);
      vm_destroy(vm);
      return 0;
    }

    // Write string data if present
    for (int j = 0; j < inst->num_operands; j++) {
      if (inst->operands[j].type == OP_STRING) {
        if (fwrite(inst->operands[j].string, 1,
                   strlen(inst->operands[j].string),
                   out) != strlen(inst->operands[j].string)) {
          printf("Error: Failed to write string data for instruction %d\n", i);
          fclose(out);
          vm_destroy(vm);
          return 0;
        }
      }
    }
  }

  fclose(out);
  vm_destroy(vm);

  printf("Successfully assembled %s to %s\n", input_file, output_file);
  printf("Program size: %d instructions\n", vm->program_size);
  printf("Labels: %d\n", vm->num_labels);

  return 1;
}
