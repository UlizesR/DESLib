#ifndef DEZ_PARSER_H
#define DEZ_PARSER_H

#include "../include/dez_vm_types.h"
#include "dez_lexer.h"
#include "dez_symbol_table.h"
#include <stdbool.h>
#include <stdint.h>

// Parser state
typedef struct {
  lexer_t lexer;
  symbol_table_t *symbol_table;
  uint32_t *output;         // Output binary code
  int output_size;          // Current output size
  int output_capacity;      // Maximum output capacity
  uint32_t current_address; // Current instruction address
  int pass;                 // Pass number (1 or 2)
  bool error;               // Error flag
} parser_t;

// Parsed instruction
typedef struct {
  dez_instruction_type_t type;
  dez_operand_t operands[3];
  int num_operands;
  uint32_t address;
  bool resolved;                // Whether all operands are resolved
  symbol_table_t *symbol_table; // Reference to symbol table
} parsed_instruction_t;

// Parser functions
void parser_init(parser_t *parser, const char *input,
                 symbol_table_t *symbol_table, uint32_t *output, int capacity);
bool parser_parse(parser_t *parser);
bool parser_parse_line(parser_t *parser);
bool parser_parse_line_with_labels(parser_t *parser);
bool parser_parse_instruction(parser_t *parser, parsed_instruction_t *inst);
bool parser_parse_operand(parser_t *parser, dez_operand_t *operand);
bool parser_parse_label(parser_t *parser);
bool parser_parse_constant(parser_t *parser);

// Instruction encoding
uint32_t parser_encode_instruction(const parsed_instruction_t *inst);
uint32_t parser_encode_mov(uint8_t reg, uint32_t immediate);
uint32_t parser_encode_store(uint8_t reg, uint32_t address);
uint32_t parser_encode_arithmetic(dez_instruction_type_t type, uint8_t reg1,
                                  uint8_t reg2, uint8_t reg3);
uint32_t parser_encode_jump(dez_instruction_type_t type, uint8_t reg,
                            uint32_t address);
uint32_t parser_encode_sys(uint8_t reg, uint32_t syscall);
uint32_t parser_encode_single(dez_instruction_type_t type);

// Operand resolution
bool parser_resolve_operand(parser_t *parser, dez_operand_t *operand);
bool parser_resolve_label(parser_t *parser, const char *label,
                          uint32_t *address);
bool parser_resolve_constant(parser_t *parser, const char *name,
                             uint32_t *value);

// Error handling
void parser_error(parser_t *parser, const char *message);
void parser_expected(parser_t *parser, const char *expected);

// Utility functions
bool parser_expect_token(parser_t *parser, token_type_t expected);
bool parser_consume_token(parser_t *parser, token_type_t expected);
token_t parser_current_token(parser_t *parser);
token_t parser_peek_token(parser_t *parser);
void parser_advance(parser_t *parser);

#endif // DEZ_PARSER_H
