#ifndef DEZ_LEXER_H
#define DEZ_LEXER_H

#include <stdbool.h>
#include <stdint.h>

// Token types
typedef enum {
  TOKEN_EOF = 0,
  TOKEN_NEWLINE,
  TOKEN_IDENTIFIER,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_REGISTER,
  TOKEN_COMMA,
  TOKEN_COLON,
  TOKEN_HASH,     // # for immediate values
  TOKEN_LBRACKET, // [ for memory addressing
  TOKEN_RBRACKET, // ] for memory addressing
  TOKEN_COMMENT,
  TOKEN_UNKNOWN
} token_type_t;

// Token structure
typedef struct {
  token_type_t type;
  char value[256];    // Token value as string
  uint32_t num_value; // Numeric value for numbers
  int line;           // Line number for error reporting
  int column;         // Column number for error reporting
} token_t;

// Lexer state
typedef struct {
  const char *input;     // Input string
  int position;          // Current position
  int line;              // Current line
  int column;            // Current column
  token_t current_token; // Current token
  bool has_token;        // Whether we have a current token
} lexer_t;

// Lexer functions
void lexer_init(lexer_t *lexer, const char *input);
token_t lexer_next_token(lexer_t *lexer);
token_t lexer_peek_token(lexer_t *lexer);
void lexer_skip_whitespace(lexer_t *lexer);
void lexer_skip_comment(lexer_t *lexer);
bool lexer_is_alpha(char c);
bool lexer_is_digit(char c);
bool lexer_is_alnum(char c);
bool lexer_is_whitespace(char c);

// Token utility functions
const char *token_type_to_string(token_type_t type);
bool is_register_token(const char *value);
uint8_t register_name_to_number(const char *name);

#endif // DEZ_LEXER_H
