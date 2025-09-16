#include "dez_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize parser
void parser_init(parser_t *parser, const char *input,
                 symbol_table_t *symbol_table, uint32_t *output, int capacity) {
  lexer_init(&parser->lexer, input);
  parser->symbol_table = symbol_table;
  parser->output = output;
  parser->output_size = 0;
  parser->output_capacity = capacity;
  parser->current_address = 0;
  parser->pass = 1;
  parser->error = false;
}

// Parse entire input
bool parser_parse(parser_t *parser) {
  // First pass: collect labels and constants
  parser->pass = 1;
  parser->symbol_table->pass = 1;
  while (!parser->error && parser_current_token(parser).type != TOKEN_EOF) {
    if (!parser_parse_line(parser)) {
      break;
    }
  }

  if (parser->error) {
    return false;
  }

  // Second pass: generate code
  parser->pass = 2;
  parser->symbol_table->pass = 2;
  parser->current_address = 0;
  parser->output_size = 0;
  lexer_init(&parser->lexer, parser->lexer.input); // Reset lexer

  while (!parser->error && parser_current_token(parser).type != TOKEN_EOF) {
    if (!parser_parse_line(parser)) {
      break;
    }
  }

  return !parser->error;
}

// Parse a single line
bool parser_parse_line(parser_t *parser) {
  token_t token = parser_current_token(parser);

  // Skip empty lines
  if (token.type == TOKEN_NEWLINE) {
    parser_advance(parser);
    return true;
  }

  // Parse label
  if (token.type == TOKEN_IDENTIFIER) {
    token_t next = parser_peek_token(parser);
    if (next.type == TOKEN_COLON) {
      return parser_parse_label(parser);
    }
  }

  // Parse constant definition
  if (token.type == TOKEN_IDENTIFIER) {
    token_t next = parser_peek_token(parser);
    if (next.type == TOKEN_IDENTIFIER && strcmp(next.value, "EQU") == 0) {
      return parser_parse_constant(parser);
    }
  }

  // Parse instruction
  parsed_instruction_t inst;
  if (parser_parse_instruction(parser, &inst)) {
    if (parser->pass == 2) {
      // Generate binary code
      uint32_t encoded = parser_encode_instruction(&inst);
      if (parser->output_size < parser->output_capacity) {
        parser->output[parser->output_size++] = encoded;
      } else {
        parser_error(parser, "Output buffer overflow");
        return false;
      }
    }
    parser->current_address++;
    return true;
  }

  return false;
}

// Parse a label
bool parser_parse_label(parser_t *parser) {
  token_t name_token = parser_current_token(parser);
  parser_advance(parser); // Consume identifier

  if (!parser_consume_token(parser, TOKEN_COLON)) {
    return false;
  }

  if (parser->pass == 1) {
    // Define label
    symbol_table_define(parser->symbol_table, name_token.value,
                        parser->current_address, name_token.line);
  }

  return true;
}

// Parse a constant definition
bool parser_parse_constant(parser_t *parser) {
  token_t name_token = parser_current_token(parser);
  parser_advance(parser); // Consume identifier

  if (!parser_consume_token(parser, TOKEN_IDENTIFIER) ||
      strcmp(parser_current_token(parser).value, "EQU") != 0) {
    parser_expected(parser, "EQU");
    return false;
  }
  parser_advance(parser); // Consume EQU

  // Parse value
  token_t value_token = parser_current_token(parser);
  if (value_token.type != TOKEN_NUMBER) {
    parser_expected(parser, "number");
    return false;
  }
  parser_advance(parser); // Consume number

  if (parser->pass == 1) {
    symbol_table_define_constant(parser->symbol_table, name_token.value,
                                 value_token.num_value, name_token.line);
  }

  return true;
}

// Parse an instruction
bool parser_parse_instruction(parser_t *parser, parsed_instruction_t *inst) {
  token_t token = parser_current_token(parser);

  if (token.type != TOKEN_IDENTIFIER) {
    return false;
  }

  // Parse instruction type
  if (strcmp(token.value, "MOV") == 0) {
    inst->type = INST_MOV;
    inst->num_operands = 2;
  } else if (strcmp(token.value, "STORE") == 0) {
    inst->type = INST_STORE;
    inst->num_operands = 2;
  } else if (strcmp(token.value, "ADD") == 0) {
    inst->type = INST_ADD;
    inst->num_operands = 3;
  } else if (strcmp(token.value, "SUB") == 0) {
    inst->type = INST_SUB;
    inst->num_operands = 3;
  } else if (strcmp(token.value, "MUL") == 0) {
    inst->type = INST_MUL;
    inst->num_operands = 3;
  } else if (strcmp(token.value, "DIV") == 0) {
    inst->type = INST_DIV;
    inst->num_operands = 3;
  } else if (strcmp(token.value, "JMP") == 0) {
    inst->type = INST_JMP;
    inst->num_operands = 1;
  } else if (strcmp(token.value, "JZ") == 0) {
    inst->type = INST_JZ;
    inst->num_operands = 1;
  } else if (strcmp(token.value, "JNZ") == 0) {
    inst->type = INST_JNZ;
    inst->num_operands = 1;
  } else if (strcmp(token.value, "CMP") == 0) {
    inst->type = INST_CMP;
    inst->num_operands = 2;
  } else if (strcmp(token.value, "SYS") == 0) {
    inst->type = INST_SYS;
    inst->num_operands = 2;
  } else if (strcmp(token.value, "HALT") == 0) {
    inst->type = INST_HALT;
    inst->num_operands = 0;
  } else if (strcmp(token.value, "NOP") == 0) {
    inst->type = INST_NOP;
    inst->num_operands = 0;
  } else {
    parser_error(parser, "Unknown instruction");
    return false;
  }

  parser_advance(parser); // Consume instruction name

  // Parse operands
  for (int i = 0; i < inst->num_operands; i++) {
    if (i > 0 && !parser_consume_token(parser, TOKEN_COMMA)) {
      parser_expected(parser, "comma");
      return false;
    }

    if (!parser_parse_operand(parser, &inst->operands[i])) {
      return false;
    }
  }

  inst->address = parser->current_address;
  inst->resolved = true;
  inst->symbol_table = parser->symbol_table;

  return true;
}

// Parse an operand
bool parser_parse_operand(parser_t *parser, dez_operand_t *operand) {
  token_t token = parser_current_token(parser);

  if (token.type == TOKEN_REGISTER) {
    operand->type = OP_REGISTER;
    operand->reg = register_name_to_number(token.value);
    parser_advance(parser);
    return true;
  }

  if (token.type == TOKEN_STRING) {
    operand->type = OP_STRING;
    strncpy(operand->string, token.value, sizeof(operand->string) - 1);
    operand->string[sizeof(operand->string) - 1] = '\0';
    parser_advance(parser);
    return true;
  }

  if (token.type == TOKEN_NUMBER) {
    operand->type = OP_IMMEDIATE;
    operand->value = token.num_value;
    parser_advance(parser);
    return true;
  }

  if (token.type == TOKEN_HASH) {
    parser_advance(parser); // Consume #
    token = parser_current_token(parser);

    if (token.type == TOKEN_NUMBER) {
      operand->type = OP_IMMEDIATE;
      operand->value = token.num_value;
      parser_advance(parser);
      return true;
    } else if (token.type == TOKEN_IDENTIFIER) {
      // Check if it's a system call name
      if (strcmp(token.value, "PRINT") == 0) {
        operand->type = OP_IMMEDIATE;
        operand->value = SYS_PRINT;
      } else if (strcmp(token.value, "PRINT_STR") == 0) {
        operand->type = OP_IMMEDIATE;
        operand->value = SYS_PRINT_STR;
      } else if (strcmp(token.value, "PRINT_CHAR") == 0) {
        operand->type = OP_IMMEDIATE;
        operand->value = SYS_PRINT_CHAR;
      } else if (strcmp(token.value, "EXIT") == 0) {
        operand->type = OP_IMMEDIATE;
        operand->value = SYS_EXIT;
      } else {
        operand->type = OP_LABEL;
        strncpy(operand->label, token.value, sizeof(operand->label) - 1);
        operand->label[sizeof(operand->label) - 1] = '\0';
      }
      parser_advance(parser);
      return true;
    }
  }

  if (token.type == TOKEN_LBRACKET) {
    parser_advance(parser); // Consume [
    token = parser_current_token(parser);

    if (token.type == TOKEN_NUMBER) {
      operand->type = OP_MEMORY;
      operand->address = token.num_value;
      parser_advance(parser);
    } else if (token.type == TOKEN_IDENTIFIER) {
      operand->type = OP_MEMORY;
      strncpy(operand->label, token.value, sizeof(operand->label) - 1);
      operand->label[sizeof(operand->label) - 1] = '\0';
      parser_advance(parser);
    } else {
      parser_expected(parser, "number or label");
      return false;
    }

    if (!parser_consume_token(parser, TOKEN_RBRACKET)) {
      parser_expected(parser, "]");
      return false;
    }
    return true;
  }

  if (token.type == TOKEN_IDENTIFIER) {
    // Check if it's a system call name
    if (strcmp(token.value, "PRINT") == 0) {
      operand->type = OP_IMMEDIATE;
      operand->value = SYS_PRINT;
    } else if (strcmp(token.value, "PRINT_STR") == 0) {
      operand->type = OP_IMMEDIATE;
      operand->value = SYS_PRINT_STR;
    } else if (strcmp(token.value, "PRINT_CHAR") == 0) {
      operand->type = OP_IMMEDIATE;
      operand->value = SYS_PRINT_CHAR;
    } else if (strcmp(token.value, "EXIT") == 0) {
      operand->type = OP_IMMEDIATE;
      operand->value = SYS_EXIT;
    } else {
      operand->type = OP_LABEL;
      strncpy(operand->label, token.value, sizeof(operand->label) - 1);
      operand->label[sizeof(operand->label) - 1] = '\0';
    }
    parser_advance(parser);
    return true;
  }

  parser_expected(parser, "operand");
  return false;
}

// Encode instruction to binary
uint32_t parser_encode_instruction(const parsed_instruction_t *inst) {
  switch (inst->type) {
  case INST_MOV:
    if (inst->operands[1].type == OP_STRING) {
      // Handle string literal - load address of string
      char symbol_name[64];
      snprintf(symbol_name, sizeof(symbol_name), "__str_%s",
               inst->operands[1].string);
      symbol_table_define_string(inst->symbol_table, symbol_name,
                                 inst->operands[1].string, 0);
      // Find the symbol to get its address
      symbol_t *sym = symbol_table_find(inst->symbol_table, symbol_name);
      uint32_t string_addr = sym ? sym->address : 0;
      return parser_encode_mov(inst->operands[0].reg, string_addr);
    } else {
      return parser_encode_mov(inst->operands[0].reg, inst->operands[1].value);
    }
  case INST_STORE:
    return parser_encode_store(inst->operands[0].reg,
                               inst->operands[1].address);
  case INST_ADD:
  case INST_SUB:
  case INST_MUL:
  case INST_DIV:
    return parser_encode_arithmetic(inst->type, inst->operands[0].reg,
                                    inst->operands[1].reg,
                                    inst->operands[2].reg);
  case INST_JMP:
    return parser_encode_jump(inst->type, 0, inst->operands[0].value);
  case INST_JZ:
  case INST_JNZ:
    return parser_encode_jump(inst->type, 0, inst->operands[0].value);
  case INST_CMP:
    return parser_encode_arithmetic(inst->type, inst->operands[0].reg,
                                    inst->operands[1].reg, 0);
  case INST_SYS:
    return parser_encode_sys(inst->operands[0].reg, inst->operands[1].value);
  case INST_HALT:
  case INST_NOP:
    return parser_encode_single(inst->type);
  default:
    return 0;
  }
}

// Encode MOV instruction
uint32_t parser_encode_mov(uint8_t reg, uint32_t immediate) {
  return (INST_MOV << 24) | (reg << 20) | (immediate & 0x0FFF);
}

// Encode STORE instruction
uint32_t parser_encode_store(uint8_t reg, uint32_t address) {
  return (INST_STORE << 24) | (reg << 20) | (address & 0x0FFF);
}

// Encode arithmetic instruction
uint32_t parser_encode_arithmetic(dez_instruction_type_t type, uint8_t reg1,
                                  uint8_t reg2, uint8_t reg3) {
  return (type << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12);
}

// Encode jump instruction
uint32_t parser_encode_jump(dez_instruction_type_t type, uint8_t reg,
                            uint32_t address) {
  return (type << 24) | (reg << 20) | (address & 0x0FFF);
}

// Encode system call
uint32_t parser_encode_sys(uint8_t reg, uint32_t syscall) {
  return (INST_SYS << 24) | (reg << 20) | (syscall & 0x0FFF);
}

// Encode single instruction
uint32_t parser_encode_single(dez_instruction_type_t type) {
  return type << 24;
}

// Error handling
void parser_error(parser_t *parser, const char *message) {
  printf("Error at line %d, column %d: %s\n", parser_current_token(parser).line,
         parser_current_token(parser).column, message);
  parser->error = true;
}

void parser_expected(parser_t *parser, const char *expected) {
  printf("Error at line %d, column %d: Expected %s, got %s\n",
         parser_current_token(parser).line, parser_current_token(parser).column,
         expected, token_type_to_string(parser_current_token(parser).type));
  parser->error = true;
}

// Token handling
bool parser_expect_token(parser_t *parser, token_type_t expected) {
  return parser_current_token(parser).type == expected;
}

bool parser_consume_token(parser_t *parser, token_type_t expected) {
  if (parser_expect_token(parser, expected)) {
    parser_advance(parser);
    return true;
  }
  return false;
}

token_t parser_current_token(parser_t *parser) {
  return lexer_peek_token(&parser->lexer);
}

token_t parser_peek_token(parser_t *parser) {
  return lexer_peek_token(&parser->lexer);
}

void parser_advance(parser_t *parser) { lexer_next_token(&parser->lexer); }
