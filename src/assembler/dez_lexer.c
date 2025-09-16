#include "dez_lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Compiler optimization hints
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Initialize lexer
void lexer_init(lexer_t *lexer, const char *input) {
  lexer->input = input;
  lexer->position = 0;
  lexer->line = 1;
  lexer->column = 1;
  lexer->has_token = false;
  memset(&lexer->current_token, 0, sizeof(token_t));
}

// Check if character is alphabetic (optimized with lookup table)
static const bool alpha_table[256] = {
    [0] = false,   [1] = false,   [2] = false,  [3] = false,   [4] = false,
    [5] = false,   [6] = false,   [7] = false,  [8] = false,   [9] = false,
    [10] = false,  [11] = false,  [12] = false, [13] = false,  [14] = false,
    [15] = false,  [16] = false,  [17] = false, [18] = false,  [19] = false,
    [20] = false,  [21] = false,  [22] = false, [23] = false,  [24] = false,
    [25] = false,  [26] = false,  [27] = false, [28] = false,  [29] = false,
    [30] = false,  [31] = false,  [32] = false, [33] = false,  [34] = false,
    [35] = false,  [36] = false,  [37] = false, [38] = false,  [39] = false,
    [40] = false,  [41] = false,  [42] = false, [43] = false,  [44] = false,
    [45] = false,  [46] = false,  [47] = false, [48] = false,  [49] = false,
    [50] = false,  [51] = false,  [52] = false, [53] = false,  [54] = false,
    [55] = false,  [56] = false,  [57] = false, [58] = false,  [59] = false,
    [60] = false,  [61] = false,  [62] = false, [63] = false,  [64] = false,
    [65] = true,   [66] = true,   [67] = true,  [68] = true,   [69] = true,
    [70] = true,   [71] = true,   [72] = true,  [73] = true,   [74] = true,
    [75] = true,   [76] = true,   [77] = true,  [78] = true,   [79] = true,
    [80] = true,   [81] = true,   [82] = true,  [83] = true,   [84] = true,
    [85] = true,   [86] = true,   [87] = true,  [88] = true,   [89] = true,
    [90] = true,   [91] = false,  [92] = false, [93] = false,  [94] = false,
    [95] = true,   [96] = false,  [97] = true,  [98] = true,   [99] = true,
    [100] = true,  [101] = true,  [102] = true, [103] = true,  [104] = true,
    [105] = true,  [106] = true,  [107] = true, [108] = true,  [109] = true,
    [110] = true,  [111] = true,  [112] = true, [113] = true,  [114] = true,
    [115] = true,  [116] = true,  [117] = true, [118] = true,  [119] = true,
    [120] = true,  [121] = true,  [122] = true, [123] = false, [124] = false,
    [125] = false, [126] = false, [127] = false};

bool lexer_is_alpha(char c) { return alpha_table[(unsigned char)c]; }

// Check if character is digit (optimized)
bool lexer_is_digit(char c) { return c >= '0' && c <= '9'; }

// Check if character is alphanumeric
bool lexer_is_alnum(char c) { return lexer_is_alpha(c) || lexer_is_digit(c); }

// Check if character is whitespace (optimized)
bool lexer_is_whitespace(char c) { return c == ' ' || c == '\t' || c == '\r'; }

// Skip whitespace and comments
void lexer_skip_whitespace(lexer_t *lexer) {
  // Optimized: bulk skip whitespace characters
  while (lexer->input[lexer->position] != '\0') {
    char c = lexer->input[lexer->position];

    if (LIKELY(lexer_is_whitespace(c))) {
      lexer->position++;
      lexer->column++;
    } else if (c == ';') {
      lexer_skip_comment(lexer);
    } else {
      break;
    }
  }
}

// Skip comment until end of line
void lexer_skip_comment(lexer_t *lexer) {
  while (lexer->input[lexer->position] != '\0' &&
         lexer->input[lexer->position] != '\n') {
    lexer->position++;
    lexer->column++;
  }
}

// Get next token
token_t lexer_next_token(lexer_t *lexer) {
  if (lexer->has_token) {
    lexer->has_token = false;
    return lexer->current_token;
  }

  lexer_skip_whitespace(lexer);

  token_t token;
  memset(&token, 0, sizeof(token_t));
  token.line = lexer->line;
  token.column = lexer->column;

  char c = lexer->input[lexer->position];

  // End of input
  if (c == '\0') {
    token.type = TOKEN_EOF;
    return token;
  }

  // Newline
  if (c == '\n') {
    token.type = TOKEN_NEWLINE;
    token.value[0] = c;
    token.value[1] = '\0';
    lexer->position++;
    lexer->line++;
    lexer->column = 1;
    return token;
  }

  // Single character tokens
  if (c == ',') {
    token.type = TOKEN_COMMA;
    token.value[0] = c;
    token.value[1] = '\0';
    lexer->position++;
    lexer->column++;
    return token;
  }

  if (c == ':') {
    token.type = TOKEN_COLON;
    token.value[0] = c;
    token.value[1] = '\0';
    lexer->position++;
    lexer->column++;
    return token;
  }

  if (c == '#') {
    token.type = TOKEN_HASH;
    token.value[0] = c;
    token.value[1] = '\0';
    lexer->position++;
    lexer->column++;
    return token;
  }

  if (c == '[') {
    token.type = TOKEN_LBRACKET;
    token.value[0] = c;
    token.value[1] = '\0';
    lexer->position++;
    lexer->column++;
    return token;
  }

  if (c == ']') {
    token.type = TOKEN_RBRACKET;
    token.value[0] = c;
    token.value[1] = '\0';
    lexer->position++;
    lexer->column++;
    return token;
  }

  // Comment (semicolon)
  if (c == ';') {
    token.type = TOKEN_COMMENT;
    token.value[0] = c;
    token.value[1] = '\0';
    lexer->position++;
    lexer->column++;

    // Skip the rest of the comment until end of line
    while (lexer->input[lexer->position] != '\0' &&
           lexer->input[lexer->position] != '\n') {
      lexer->position++;
      lexer->column++;
    }

    return token;
  }

  // String literal
  if (c == '"') {
    token.type = TOKEN_STRING;
    int i = 0;
    lexer->position++; // Skip opening quote
    lexer->column++;

    while (lexer->input[lexer->position] != '\0' &&
           lexer->input[lexer->position] != '"' && i < 255) {
      token.value[i++] = lexer->input[lexer->position++];
      lexer->column++;
    }

    if (lexer->input[lexer->position] == '"') {
      lexer->position++; // Skip closing quote
      lexer->column++;
    } else {
      // Unterminated string
      token.type = TOKEN_UNKNOWN;
    }

    token.value[i] = '\0';
    return token;
  }

  // Number
  if (lexer_is_digit(c)) {
    token.type = TOKEN_NUMBER;
    int i = 0;

    while (lexer_is_digit(lexer->input[lexer->position]) && i < 255) {
      token.value[i++] = lexer->input[lexer->position++];
      lexer->column++;
    }

    token.value[i] = '\0';
    token.num_value = (uint32_t)strtoul(token.value, NULL, 10);
    return token;
  }

  // Identifier or register
  if (lexer_is_alpha(c)) {
    int i = 0;

    while (lexer_is_alnum(lexer->input[lexer->position]) && i < 255) {
      token.value[i++] = lexer->input[lexer->position++];
      lexer->column++;
    }

    token.value[i] = '\0';

    // Check if it's a register
    if (is_register_token(token.value)) {
      token.type = TOKEN_REGISTER;
    } else {
      token.type = TOKEN_IDENTIFIER;
    }

    return token;
  }

  // Unknown character
  token.type = TOKEN_UNKNOWN;
  token.value[0] = c;
  token.value[1] = '\0';
  lexer->position++;
  lexer->column++;
  return token;
}

// Peek at next token without consuming it
token_t lexer_peek_token(lexer_t *lexer) {
  if (!lexer->has_token) {
    lexer->current_token = lexer_next_token(lexer);
    lexer->has_token = true;
  }
  return lexer->current_token;
}

// Check if token is a register
bool is_register_token(const char *value) {
  if (strlen(value) < 2 || strlen(value) > 3)
    return false;
  if (value[0] != 'R')
    return false;

  // Check if it's R0-R15
  if (strlen(value) == 2) {
    return value[1] >= '0' && value[1] <= '9';
  } else if (strlen(value) == 3) {
    if (value[1] == '1') {
      return value[2] >= '0' && value[2] <= '5'; // R10-R15
    }
  }
  return false;
}

// Convert register name to number
uint8_t register_name_to_number(const char *name) {
  if (!is_register_token(name))
    return 0;

  if (strlen(name) == 2) {
    return name[1] - '0';
  } else {
    return 10 + (name[2] - '0');
  }
}

// Convert token type to string for debugging
const char *token_type_to_string(token_type_t type) {
  switch (type) {
  case TOKEN_EOF:
    return "EOF";
  case TOKEN_NEWLINE:
    return "NEWLINE";
  case TOKEN_IDENTIFIER:
    return "IDENTIFIER";
  case TOKEN_NUMBER:
    return "NUMBER";
  case TOKEN_STRING:
    return "STRING";
  case TOKEN_REGISTER:
    return "REGISTER";
  case TOKEN_COMMA:
    return "COMMA";
  case TOKEN_COLON:
    return "COLON";
  case TOKEN_HASH:
    return "HASH";
  case TOKEN_LBRACKET:
    return "LBRACKET";
  case TOKEN_RBRACKET:
    return "RBRACKET";
  case TOKEN_COMMENT:
    return "COMMENT";
  case TOKEN_UNKNOWN:
    return "UNKNOWN";
  default:
    return "INVALID";
  }
}
